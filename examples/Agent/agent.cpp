#include "common.h"
#include "httplib.h"
#include "json.hpp"
#include "agent_utils.h"
#include "tool_executor.h"

#include "index.html.gz.hpp"
#include "loading.html.hpp"

#include <atomic>
#include <thread>
#include <chrono>
#include <signal.h>
#include <fstream>
#include <memory>
#include <cstring>
#include <iostream>
#include <sstream>
#include <ctime>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/wait.h>
    #include <cerrno>
    #include <unistd.h>
#endif

using json = nlohmann::ordered_json;

// SSE 流解析工具函数
class SSEParser {
public:
    static std::vector<json> parseSSEResponse(const std::string& sse_data) {
        std::vector<json> result;
        std::istringstream stream(sse_data);
        std::string line;
        
        while (std::getline(stream, line)) {
            // 移除行尾的回车符
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            
            // 跳过空行和注释行
            if (line.empty() || line[0] == ':') {
                continue;
            }
            
            // 解析 data: 行
            if (line.substr(0, 5) == "data:") {
                std::string json_data = line.substr(5);
                
                // 移除前导空格
                size_t start = json_data.find_first_not_of(" \t");
                if (start != std::string::npos) {
                    json_data = json_data.substr(start);
                }
                
                // 检查是否是结束标记
                if (json_data == "[DONE]") {
                    break;
                }
                
                // 尝试解析JSON
                try {
                    if (!json_data.empty()) {
                        json parsed = json::parse(json_data);
                        result.push_back(parsed);
                    }
                } catch (const json::parse_error& e) {
                    LOG_WRN("SSE JSON 解析错误: %s, 数据: %s\n", e.what(), json_data.c_str());
                    continue;
                }
            }
        }
        
        return result;
    }
    
    static json combineStreamingResponse(const std::vector<json>& chunks) {
        if (chunks.empty()) {
            LOG_WRN("SSE chunks 为空\n");
            return json{};
        }
        
        LOG_INF("开始合并 %zu 个SSE chunks\n", chunks.size());
        
        // 找到有效的基础chunk（包含choices的chunk）
        json combined;
        bool found_base = false;
        
        for (const auto& chunk : chunks) {
            if (chunk.contains("choices") && !chunk["choices"].empty()) {
                combined = chunk;
                found_base = true;
                break;
            }
        }
        
        if (!found_base) {
            LOG_ERR("在SSE chunks中未找到有效的choices数据\n");
            return json{};
        }
        
        // 如果只有一个chunk，特殊处理
        if (chunks.size() == 1) {
            LOG_INF("单个SSE chunk，直接处理\n");
            if (combined.contains("choices") && !combined["choices"].empty()) {
                auto& choice = combined["choices"][0];
                
                // 如果已经是完整message格式，直接返回
                if (choice.contains("message")) {
                    return combined;
                }
                
                // 如果是delta格式，转换为message格式
                if (choice.contains("delta")) {
                    const auto& delta = choice["delta"];
                    choice["message"] = json{};
                    choice["message"]["role"] = "assistant";
                    
                    if (delta.contains("content") && !delta["content"].is_null()) {
                        choice["message"]["content"] = delta["content"];
                    } else {
                        choice["message"]["content"] = nullptr;
                    }
                    
                    if (delta.contains("tool_calls")) {
                        choice["message"]["tool_calls"] = delta["tool_calls"];
                        choice["finish_reason"] = "tool_calls";
                    } else {
                        choice["finish_reason"] = "stop";
                    }
                    
                    choice.erase("delta");
                }
            }
            return combined;
        }
        
        // 多个chunks，需要合并
        std::string combined_content = "";
        json combined_tool_calls = json::array();
        std::string last_finish_reason = "stop";
        
        for (const auto& chunk : chunks) {
            if (chunk.contains("choices") && !chunk["choices"].empty()) {
                const auto& choice = chunk["choices"][0];
                
                // 处理finish_reason
                if (choice.contains("finish_reason") && !choice["finish_reason"].is_null()) {
                    last_finish_reason = choice["finish_reason"].get<std::string>();
                }
                
                if (choice.contains("delta")) {
                    const auto& delta = choice["delta"];
                    
                    // 合并content
                    if (delta.contains("content") && !delta["content"].is_null()) {
                        combined_content += delta["content"].get<std::string>();
                    }
                    
                    // 合并tool_calls
                    if (delta.contains("tool_calls")) {
                        for (const auto& tool_call : delta["tool_calls"]) {
                            combined_tool_calls.push_back(tool_call);
                        }
                    }
                }
            }
        }
        
        LOG_INF("合并完成：content长度=%zu, tool_calls数量=%zu\n", 
               combined_content.length(), combined_tool_calls.size());
        
        // 构造最终响应
        if (combined.contains("choices") && !combined["choices"].empty()) {
            auto& choice = combined["choices"][0];
            
            // 创建完整的message而不是delta
            choice["message"] = json{};
            choice["message"]["role"] = "assistant";
            
            if (!combined_content.empty()) {
                choice["message"]["content"] = combined_content;
            } else {
                choice["message"]["content"] = nullptr;
            }
            
            if (!combined_tool_calls.empty()) {
                choice["message"]["tool_calls"] = combined_tool_calls;
                choice["finish_reason"] = "tool_calls";
            } else {
                choice["finish_reason"] = last_finish_reason;
            }
            
            // 移除delta字段
            choice.erase("delta");
        }
        
        return combined;
    }
};

struct AgentConfig {
    std::string agent_host = "127.0.0.1";
    int agent_port = 8081;
    std::string llama_server_host = "127.0.0.1";
    int llama_server_port = 8080;
    std::string llama_server_path = "./llama-server";
    std::string model_path = "";
    int n_ctx = 2048;
    int n_gpu_layers = -1;
    bool auto_start_server = true;
    json tools;
};

class LlamaAgent {
private:
    AgentConfig AgentConfig;
    std::unique_ptr<httplib::Server> server;
    std::unique_ptr<httplib::Client> llama_client;
    std::unique_ptr<ToolExecutor> tool_executor;
    std::atomic<bool> running{false};
    std::thread server_thread;

#ifdef _WIN32
    PROCESS_INFORMATION llama_process{};
#else
    pid_t llama_pid = -1;
#endif

public:
    LlamaAgent() {
        server = std::make_unique<httplib::Server>();
        tool_executor = std::make_unique<ToolExecutor>();
    }

    ~LlamaAgent() {
        stop();
    }

    bool loadConfig(const std::string& config_file) {
        try {
            std::ifstream file(config_file);
            if (!file.is_open()) {
                // TODO:使用 spdlog 日志库记录错误，可以使用 __FUNCTION__ 定位函数。
                LOG_ERR("打开配置文件失败：%s \n", config_file.c_str());
                return false;
            }
            // 如果打开配置文件成功，则使用 nlohmann::json 库解析 JSON 格式的配置文件，解析的过程中出现问题将会抛出异常。
            json j;
            file >> j;
            // 使用 nlohmann::json 库的 value 方法获取配置项的值，如果配置项不存在，则使用默认值。
            AgentConfig.agent_host = j.value("agent_host", AgentConfig.agent_host);
            AgentConfig.agent_port = j.value("agent_port", AgentConfig.agent_port);
            AgentConfig.llama_server_host = j.value("llama_server_host", AgentConfig.llama_server_host);
            AgentConfig.llama_server_port = j.value("llama_server_port", AgentConfig.llama_server_port);
            AgentConfig.llama_server_path = j.value("llama_server_path", AgentConfig.llama_server_path);
            AgentConfig.model_path = j.value("model_path", AgentConfig.model_path);
            AgentConfig.n_ctx = j.value("n_ctx", AgentConfig.n_ctx);
            AgentConfig.n_gpu_layers = j.value("n_gpu_layers", AgentConfig.n_gpu_layers);
            AgentConfig.auto_start_server = j.value("auto_start_server", AgentConfig.auto_start_server);
            AgentConfig.tools = j.value("tools", json::array());

            // 将配置文件中的配置的工具注册到 ToolExecutor 中，设置一个标志用来判断配置中的工具定义是否有效。
            bool areToolsDefinitionValid = true;
            for (const auto& tool : AgentConfig.tools) {
                if (!(tool_executor->registerExternalTools(tool))) {
                    areToolsDefinitionValid = false;
                }
            }
            if (!areToolsDefinitionValid) {
                LOG_ERR("配置文件中的工具定义无效，请检查 tools 字段。\n");
                return false;
            }
            LOG_INF("配置加载成功！\n");
            return true;
        } catch (const std::exception& e) {
            LOG_ERR("加载配置失败：%s\n", e.what());
            return false;
        }
    }

    bool waitForServerStartup() {
        // 最多尝试60次
        const int max_attempts = 60;
        // 每次间隔1秒
        const int retry_interval_ms = 1000;

        LOG_INF("正在等待 llama-server 启动...\n");

        for (int attempt = 1; attempt <= max_attempts; ++attempt) {
            // 检查进程是否还在运行（避免无谓的等待）
#ifdef _WIN32
            if (llama_process.hProcess) {
                DWORD exit_code;
                if (GetExitCodeProcess(llama_process.hProcess, &exit_code) && exit_code != STILL_ACTIVE) {
                    LOG_ERR("llama-server 进程已退出，退出码: %lu\n", exit_code);
                    return false;
                }
            }
#else
            if (llama_pid > 0) {
                int status;
                pid_t result = waitpid(llama_pid, &status, WNOHANG);
                if (result > 0) {
                    if (WIFEXITED(status)) {
                        LOG_ERR("llama-server 进程已退出，退出码: %d\n", WEXITSTATUS(status));
                    } else if (WIFSIGNALED(status)) {
                        LOG_ERR("llama-server 进程被信号终止: %d\n", WTERMSIG(status));
                    }
                    return false;
                } else if (result < 0 && errno != ECHILD) {
                    LOG_ERR("检查进程状态失败: %s\n", strerror(errno));
                    return false;
                }
            }
#endif

            // 尝试连接健康检查端点
            auto res = llama_client->Get("/health");
            if (res && res->status == 200) {
                LOG_INF("llama-server 启动成功！(尝试 %d/%d 次)\n", attempt, max_attempts);
                return true;
            }

            // 输出等待进度
            if (attempt % 10 == 0) {
                LOG_INF("等待 llama-server 启动中... (%d/%d)\n", attempt, max_attempts);
            }

            // 等待后重试
            std::this_thread::sleep_for(std::chrono::milliseconds(retry_interval_ms));
        }

        LOG_ERR("llama-server 启动超时！已尝试 %d 次，总计等待时间: %d 秒\n",
                max_attempts, max_attempts * retry_interval_ms / 1000);
        return false;
    }

    bool startLlamaServer() {
        // 如果自动启动 llama-server 服务器被禁用，则假设 llama-server 已经在运行。
        if (!AgentConfig.auto_start_server) {
            LOG_INF(" auto_start_server 已禁用，这里假设 llama-server 已在运行！\n");
            return true;
        }
        // 拼接 llama-server 的命令行参数。
        std::string cmd = AgentConfig.llama_server_path;
        cmd += " -m " + AgentConfig.model_path;
        cmd += " --host " + AgentConfig.llama_server_host;
        cmd += " --port " + std::to_string(AgentConfig.llama_server_port);
        cmd += " -c " + std::to_string(AgentConfig.n_ctx);
        cmd += " --jinja";
        if (AgentConfig.n_gpu_layers >= 0) {
            cmd += " -ngl " + std::to_string(AgentConfig.n_gpu_layers);
        }

        LOG_INF("正在启动 llama-server: %s\n", cmd.c_str());
        // 在 Windows 上使用 CreateProcess 启动 llama-server 进程，在其他平台上使用 fork 和 system 调用。
#ifdef _WIN32
        STARTUPINFOA si = {sizeof(si)};
        if (!CreateProcessA(NULL, const_cast<char*>(cmd.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &si, &llama_process)) {
            LOG_ERR("启动 llama-server 失败\n");
            return false;
        }
#else
        llama_pid = fork();
        if (llama_pid == 0) {
            // Child process
            system(cmd.c_str());
            exit(0);
        } else if (llama_pid < 0) {
            LOG_ERR("fork 进程失败，即启动 llama-server 失败\n");
            return false;
        }
#endif

        // 创建 HTTP 客户端用于健康检查
        llama_client = std::make_unique<httplib::Client>(AgentConfig.llama_server_host, AgentConfig.llama_server_port);

        // 等待并检测 llama-server 启动状态
        return waitForServerStartup();
    }

    void stopLlamaServer() {
        // 如果自动启动 llama-server 服务器被禁用，则不需要停止服务器。
        if (!AgentConfig.auto_start_server) {
            return;
        }

        LOG_INF("正在停止 llama-server...\n");

        // 根据不同的平台，使用不同的方法停止 llama-server 进程。
#ifdef _WIN32
        // 如果 llama_process.hProcess 有效，则优雅地终止进程
        if (llama_process.hProcess) {
            // 首先尝试优雅终止
            if (TerminateProcess(llama_process.hProcess, 0)) {
                // 等待进程结束，最多等待5秒
                DWORD waitResult = WaitForSingleObject(llama_process.hProcess, 5000);
                if (waitResult == WAIT_TIMEOUT) {
                    LOG_WRN("llama-server 进程在5秒内未响应，强制终止\n");
                }
            }
            CloseHandle(llama_process.hProcess);
            CloseHandle(llama_process.hThread);
            // 清理进程信息
            memset(&llama_process, 0, sizeof(llama_process));
        }
#else
        // 如果 llama_pid 大于 0，则优雅地终止进程
        if (llama_pid > 0) {
            // 首先发送 SIGTERM 信号进行优雅关闭
            if (kill(llama_pid, SIGTERM) == 0) {
                // 等待进程结束，最多等待5秒
                int wait_count = 0;
                while (wait_count < 50 && kill(llama_pid, 0) == 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    wait_count++;
                }

                // 如果进程仍在运行，强制终止
                if (kill(llama_pid, 0) == 0) {
                    LOG_WRN("llama-server 进程在5秒内未响应，强制终止\n");
                    kill(llama_pid, SIGKILL);
                    // 再等待1秒确保进程被终止
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }
            // 清理进程ID
            llama_pid = -1;
        }
#endif
        LOG_INF("llama-server 已停止。\n");
    }

    void setupRoutes() {
        // Health check（应该间接的检查 llama-server 的 /health 端口）
        server->Get("/health", [this](const httplib::Request&, httplib::Response& res) {
            auto llama_res = llama_client->Get("/health");
            if (llama_res && llama_res->status == 200) {
                res.set_content(llama_res->body, "application/json");
                res.status = llama_res->status;
            } else {
                json error_response = {{"status", "error"}, {"message", "llama-server unavailable"}};
                res.set_content(error_response.dump(), "application/json");
                res.status = 503;
            }
        });

        // List available tools
        server->Get("/tools", [this](const httplib::Request&, httplib::Response& res) {
            json response = {{"tools", tool_executor->getTools()}};
            res.set_content(response.dump(), "application/json");
        });

        // Chat completion with tool support - 简化透传版本
        server->Post("/v1/chat/completions", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                // 验证请求体
                if (req.body.empty()) {
                    json error = {{"error", "请求体为空"}};
                    res.set_content(error.dump(), "application/json");
                    res.status = 400;
                    return;
                }
        
                // 解析请求体
                json request_body;
                try {
                    request_body = json::parse(req.body);
                } catch (const json::parse_error& e) {
                    LOG_ERR("请求体JSON解析失败: %s\n", e.what());
                    json error = {{"error", "请求体格式错误: " + std::string(e.what())}};
                    res.set_content(error.dump(), "application/json");
                    res.status = 400;
                    return;
                }

                // 添加工具定义
                json all_tools = tool_executor->getTools();
                if (!request_body.contains("tools") && !all_tools.empty()) {
                    request_body["tools"] = all_tools;
                }

                // 检查是否需要流式响应
                bool stream_mode = request_body.value("stream", false);
        
                // 设置超时
                llama_client->set_read_timeout(60);
                llama_client->set_write_timeout(60);

                // 第一次请求：发送到llama-server
                auto llama_res = llama_client->Post("/v1/chat/completions",
                    request_body.dump(), "application/json");

                if (!llama_res || llama_res->status != 200 || llama_res->body.empty()) {
                    LOG_ERR("连接 llama-server 失败\n");
                    json error = {{"error", "连接 llama-server 失败"}};
                    res.set_content(error.dump(), "application/json");
                    res.status = 503;
                    return;
                }

                // 检查响应中是否有工具调用
                bool has_tool_calls = false;
                json parsed_response;
                json tool_messages;
        
                // 检查是否为SSE格式
                bool is_sse = llama_res->body.find("data:") != std::string::npos;
        
                if (is_sse) {
                    // 解析SSE检查工具调用
                    auto chunks = SSEParser::parseSSEResponse(llama_res->body);
                    if (!chunks.empty()) {
                        parsed_response = SSEParser::combineStreamingResponse(chunks);
                    }
                } else {
                    // 解析JSON检查工具调用
                    try {
                        parsed_response = json::parse(llama_res->body);
                    } catch (const json::parse_error& e) {
                        // 如果解析失败，直接透传
                        if (stream_mode) {
                            res.set_header("Content-Type", "text/event-stream");
                            res.set_header("Cache-Control", "no-cache");
                            res.set_header("Connection", "keep-alive");
                        }
                        res.set_content(llama_res->body, llama_res->get_header_value("Content-Type"));
                        res.status = llama_res->status;
                        return;
                    }
                }
        
                // 检查是否有工具调用
                if (parsed_response.contains("choices") && !parsed_response["choices"].empty()) {
                    const auto& choice = parsed_response["choices"][0];
                    if (choice.contains("message") && choice["message"].contains("tool_calls")) {
                        has_tool_calls = true;
                        LOG_INF("检测到工具调用\n");
                
                        // 保存工具调用消息
                        tool_messages = request_body["messages"];
                        tool_messages.push_back(choice["message"]);
                
                        // 执行所有工具调用
                        for (const auto& tool_call : choice["message"]["tool_calls"]) {
                            std::string function_name = tool_call["function"]["name"];
                            std::string args_str = tool_call["function"]["arguments"].get<std::string>();
                    
                            LOG_INF("执行工具: %s\n", function_name.c_str());
                    
                            json arguments;
                            try {
                                arguments = json::parse(args_str);
                            } catch (const json::parse_error& e) {
                                LOG_ERR("解析工具参数失败: %s\n", e.what());
                                arguments = json::object();
                            }
                    
                            // 执行工具
                            json result;
                            try {
                                result = tool_executor->execute(function_name, arguments);
                            } catch (const std::exception& e) {
                                LOG_ERR("工具执行失败 %s: %s\n", function_name.c_str(), e.what());
                                result = json{
                                    {"error", "工具执行失败"},
                                    {"details", e.what()}
                                };
                            }
                    
                            // 添加工具结果到消息
                            tool_messages.push_back({
                                {"tool_call_id", tool_call["id"]},
                                {"role", "tool"},
                                {"name", function_name},
                                {"content", result.dump()}
                            });
                        }
                    }
                }
        
                // 如果没有工具调用，直接透传响应
                if (!has_tool_calls) {
                    LOG_INF("无工具调用，直接透传响应\n");
            
                    // 设置响应头
                    if (stream_mode) {
                        res.set_header("Content-Type", "text/event-stream");
                        res.set_header("Cache-Control", "no-cache");
                        res.set_header("Connection", "keep-alive");
                        res.set_header("Access-Control-Allow-Origin", "*");
                    } else {
                        res.set_header("Content-Type", "application/json");
                    }
            
                    // 直接返回llama-server的响应
                    res.set_content(llama_res->body, res.get_header_value("Content-Type"));
                    res.status = llama_res->status;
                    return;
                }
        
                // 有工具调用，需要继续对话
                LOG_INF("工具调用执行完成，继续对话\n");
        
                // 准备包含工具结果的新请求
                json continue_request = request_body;
                continue_request["messages"] = tool_messages;
        
                // 发送继续请求
                auto continue_res = llama_client->Post("/v1/chat/completions",
                    continue_request.dump(), "application/json");
        
                if (!continue_res || continue_res->status != 200) {
                    LOG_ERR("工具调用后续请求失败\n");
                    json error = {{"error", "工具调用后续请求失败"}};
                    res.set_content(error.dump(), "application/json");
                    res.status = 503;
                    return;
                }
        
                // 合并响应：如果是流式，需要合并两个SSE流
                if (stream_mode) {
                    res.set_header("Content-Type", "text/event-stream");
                    res.set_header("Cache-Control", "no-cache");
                    res.set_header("Connection", "keep-alive");
                    res.set_header("Access-Control-Allow-Origin", "*");
            
                    std::string combined_response;
            
                    // 添加第一个响应（工具调用）
                    if (is_sse) {
                        // 移除第一个响应的 [DONE] 标记
                        std::string first_response = llama_res->body;
                        size_t done_pos = first_response.find("data: [DONE]");
                        if (done_pos != std::string::npos) {
                            first_response = first_response.substr(0, done_pos);
                        }
                        combined_response += first_response;
                    } else {
                        // 转换为SSE格式
                        json sse_chunk = {
                            {"id", parsed_response.value("id", "chatcmpl-" + std::to_string(std::time(nullptr)))},
                            {"object", "chat.completion.chunk"},
                            {"created", parsed_response.value("created", std::time(nullptr))},
                            {"model", parsed_response.value("model", "unknown")},
                            {"choices", json::array({{
                                {"index", 0},
                                {"delta", parsed_response["choices"][0]["message"]},
                                {"finish_reason", "tool_calls"}
                            }})}
                        };
                        combined_response += "data: " + sse_chunk.dump() + "\n\n";
                    }
            
                    // 添加第二个响应（工具执行后的回复）
                    combined_response += continue_res->body;
            
                    res.set_content(combined_response, "text/event-stream");
                } else {
                    // 非流式模式，返回最终响应
                    res.set_content(continue_res->body, "application/json");
                }
        
                res.status = 200;
        
            } catch (const std::exception& e) {
                LOG_ERR("处理请求时发生异常: %s\n", e.what());
                json error = {{"error", e.what()}};
                res.set_content(error.dump(), "application/json");
                res.status = 500;
            }
        });

        // Direct tool execution endpoint
        server->Post("/execute_tool", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                json request = json::parse(req.body);
                std::string tool_name = request["name"];
                json arguments = request["arguments"];

                json result = tool_executor->execute(tool_name, arguments);
                res.set_content(result.dump(), "application/json");

            } catch (const std::exception& e) {
                json error = {{"error", e.what()}};
                res.set_content(error.dump(), "application/json");
                res.status = 400;
            }
        });

        // WebUI routes - Static files
        server->Get("/", [](const httplib::Request& req, httplib::Response& res) {
            if (req.get_header_value("Accept-Encoding").find("gzip") == std::string::npos) {
                res.set_content("Error: gzip is not supported by this browser", "text/plain");
            } else {
                res.set_header("Content-Encoding", "gzip");
                res.set_header("Cross-Origin-Embedder-Policy", "require-corp");
                res.set_header("Cross-Origin-Opener-Policy", "same-origin");
                res.set_content(reinterpret_cast<const char*>(index_html_gz), index_html_gz_len, "text/html; charset=utf-8");
            }
        });

        // Loading page for when server is starting up
        server->Get("/loading", [](const httplib::Request&, httplib::Response& res) {
            res.set_content(reinterpret_cast<const char*>(loading_html), loading_html_len, "text/html; charset=utf-8");
        });
    }

    bool start() {
        // 初始化一个指向 llama-server 服务的客户端。
        llama_client = std::make_unique<httplib::Client>(
            AgentConfig.llama_server_host, AgentConfig.llama_server_port);

        // 如果启动 llama-server 失败的话直接返回。
        if (!startLlamaServer()) {
            LOG_ERR("无法启动 llama-server，Agent 启动失败\n");
            LOG_ERR("请检查：\n");
            LOG_ERR("  1. llama-server 路径是否正确: %s\n", AgentConfig.llama_server_path.c_str());
            LOG_ERR("  2. 模型文件路径是否正确: %s\n", AgentConfig.model_path.c_str());
            LOG_ERR("  3. 端口 %d 是否被占用\n", AgentConfig.llama_server_port);
            LOG_ERR("  4. 系统资源是否充足（内存、GPU等）\n");
            return false;
        }

        // 设置 llama-agent 服务的 endpoints 。
        setupRoutes();

        // 启动 llama-agent 服务。
        running = true;
        server_thread = std::thread([this]() {
            LOG_INF("代理服务器正在监听 http://%s:%d\n", AgentConfig.agent_host.c_str(), AgentConfig.agent_port);
            server->listen(AgentConfig.agent_host, AgentConfig.agent_port);
        });

        return true;
    }

    void stop() {
        // 如果 llama-agent 服务正在运行的话，则需要停止 llama-agent 服务。
        if (running) {
            running = false;
            server->stop();
            if (server_thread.joinable()) {
                server_thread.join();
            }
            stopLlamaServer();
        }
    }

    void wait() {
        if (server_thread.joinable()) {
            server_thread.join();
        }
    }
};

std::atomic<bool> g_running{true};
LlamaAgent* g_agent_instance = nullptr;

struct CommandLineArgs {
    std::string config_file_path = "config.json";
    bool show_help = false;
    bool show_version = false;
};

void printHelp(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n"
              << "\n"
              << "Options:\n"
              << "  --config-file-path PATH        配置文件路径 (默认: config.json)\n"
              << "  --help                         显示帮助信息\n"
              << "  --version                      显示版本信息\n"
              << "\n"
              << "Examples:\n"
              << "  " << program_name << "\n"
              << "  " << program_name << " --config-file-path /path/to/agent-config.json\n"
              << "  " << program_name << " --config-file-path \"path/to/agent-config.json\"\n"
              << std::endl;
}

void printVersion() {
    std::cout << "LlamaAgent v1.0.0\n";
}

bool parseCommandLine(int argc, char** argv, CommandLineArgs& args) {
    for (int i = 1; i < argc; ++i) {
        // 获取当前命令行参数
        std::string arg = argv[i];
        // 如果当前命令行参数为 --help 的话则需要修改命令行结构体状态。
        if (arg == "--help") {
            args.show_help = true;
            return true;
        }
        // 如果当前命令行参数为 --version 的话则需要修改命令行结构体状态。
        else if (arg == "--version") {
            args.show_version = true;
            return true;
        }
        // 如果当前命令行参数为 --config-file-path 的话则需要修改命令行结构体状态。
        else if (arg == "--config-file-path") {
            if (i + 1 >= argc) {
                std::cerr << "错误: " << arg << " 选项需要一个参数\n";
                return false;
            }
            args.config_file_path = argv[++i];
        }
        // 如果当前命令行参数为 - 的话则需要修改命令行结构体状态。
        else if (arg[0] == '-') {
            std::cerr << "错误: 未知选项 '" << arg << "'\n";
            std::cerr << "使用 --help 查看帮助信息\n";
            return false;
        }
        else {
            args.config_file_path = arg;
        }
    }
    return true;
}

void signal_handler(int signal_num) {
    const char* signal_name = (signal_num == SIGINT) ? "SIGINT" :
                             (signal_num == SIGTERM) ? "SIGTERM" : "UNKNOWN";
    LOG_INF("收到信号 %s，正在关闭服务...\n", signal_name);

    g_running = false;

    // 如果有代理实例的引用，直接调用停止方法以确保立即停止
    if (g_agent_instance) {
        g_agent_instance->stop();
    }
}

int main(int argc, char** argv) {
    // 初始化程序环境，包括设置 UTF-8 区域和在 Windows 上启用 UTF-8 控制台输出。
    common_init();

    // 解析命令行参数
    CommandLineArgs args;
    if (!parseCommandLine(argc, argv, args)) {
        return 1;
    }

    // 处理帮助和版本信息
    if (args.show_help) {
        printHelp(argv[0]);
        return 0;
    }

    if (args.show_version) {
        printVersion();
        return 0;
    }

    // 获取配置文件路径
    std::string config_file = args.config_file_path;

    // 设置信号处理器以捕获终止信号
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 创建 LlamaAgent 实例
    LlamaAgent agent;
    g_agent_instance = &agent;  // 设置全局引用以便信号处理器使用

    // 如果代理实例加载配置文件失败，则输出错误信息并退出程序。
    if (!agent.loadConfig(config_file)) {
        LOG_ERR("加载配置失败！\n");
        g_agent_instance = nullptr;
        return 1;
    }

    // 如果代理实例启动失败，则输出错误信息并退出程序。
    if (!agent.start()) {
        LOG_ERR("启动 Agent 失败！\n");
        g_agent_instance = nullptr;
        return 1;
    }

    // 输出代理正在运行的信息，并提示用户按 Ctrl+C 停止。
    LOG_INF("Agent 正在运行，如果想要停止运行 Agent 请按下 Ctrl+C 。\n");

    // 执行循环，直到收到终止信号。
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // 输出代理正在关闭的信息，并停止代理实例。
    LOG_INF("正在停止运行 Agent 。\n");
    // 停止运行 Agent 。
    agent.stop();

    // 清理全局引用
    g_agent_instance = nullptr;
    LOG_INF("Agent 已完全停止。\n");

    return 0;
}
