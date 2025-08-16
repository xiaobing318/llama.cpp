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

// 改进的SSE解析器
class SSEParser {
public:
    // 解析单个SSE数据块
    static json parseSSEChunk(const std::string& chunk) {
        if (chunk.empty() || chunk == "[DONE]") {
            return json{};
        }
        
        try {
            return json::parse(chunk);
        } catch (const json::parse_error& e) {
            LOG_WRN("SSE chunk解析失败: %s\n", e.what());
            return json{};
        }
    }
    
    // 从SSE流中提取所有数据块
    static std::vector<std::string> extractSSEChunks(const std::string& sse_data) {
        std::vector<std::string> chunks;
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
            
            // 解析data:行
            if (line.substr(0, 5) == "data:") {
                std::string data = line.substr(5);
                
                // 移除前导空格
                size_t start = data.find_first_not_of(" \t");
                if (start != std::string::npos) {
                    data = data.substr(start);
                }
                
                if (!data.empty()) {
                    chunks.push_back(data);
                }
            }
        }
        
        return chunks;
    }
    
    // 合并流式tool calling响应
    static json mergeToolCallChunks(const std::vector<json>& json_chunks) {
        std::map<int, json> tool_calls_map;
        json result;
        
        for (const auto& chunk : json_chunks) {
            if (!chunk.contains("choices") || chunk["choices"].empty()) {
                continue;
            }
            
            // 保存第一个chunk的基础信息
            if (result.empty()) {
                result = chunk;
                result["choices"][0].erase("delta");
                result["choices"][0]["message"] = json{
                    {"role", "assistant"},
                    {"content", nullptr},
                    {"tool_calls", json::array()}
                };
            }
            
            const auto& choice = chunk["choices"][0];
            
            // 更新finish_reason
            if (choice.contains("finish_reason") && !choice["finish_reason"].is_null()) {
                result["choices"][0]["finish_reason"] = choice["finish_reason"];
            }
            
            // 处理delta中的tool_calls
            if (choice.contains("delta") && choice["delta"].contains("tool_calls")) {
                for (const auto& delta_tool : choice["delta"]["tool_calls"]) {
                    if (!delta_tool.contains("index")) continue;
                    
                    int idx = delta_tool["index"];
                    
                    // 初始化tool call
                    if (tool_calls_map.find(idx) == tool_calls_map.end()) {
                        tool_calls_map[idx] = json{
                            {"type", "function"},
                            {"id", ""},
                            {"function", json{
                                {"name", ""},
                                {"arguments", ""}
                            }}
                        };
                    }
                    
                    auto& tool_call = tool_calls_map[idx];
                    
                    // 更新id
                    if (delta_tool.contains("id")) {
                        tool_call["id"] = delta_tool["id"];
                    }
                    
                    // 更新type
                    if (delta_tool.contains("type")) {
                        tool_call["type"] = delta_tool["type"];
                    }
                    
                    // 更新function信息
                    if (delta_tool.contains("function")) {
                        const auto& func = delta_tool["function"];
                        if (func.contains("name") && !func["name"].get<std::string>().empty()) {
                            tool_call["function"]["name"] = func["name"];
                        }
                        if (func.contains("arguments")) {
                            std::string current_args = tool_call["function"]["arguments"];
                            tool_call["function"]["arguments"] = current_args + func["arguments"].get<std::string>();
                        }
                    }
                }
            }
        }
        
        // 将map转换为数组
        for (const auto& [idx, tool_call] : tool_calls_map) {
            result["choices"][0]["message"]["tool_calls"].push_back(tool_call);
        }
        
        return result;
    }
    
    // 合并流式content响应
    static json mergeContentChunks(const std::vector<json>& json_chunks) {
        json result;
        std::string combined_content;
        
        for (const auto& chunk : json_chunks) {
            if (!chunk.contains("choices") || chunk["choices"].empty()) {
                continue;
            }
            
            // 保存第一个chunk的基础信息
            if (result.empty()) {
                result = chunk;
                result["choices"][0].erase("delta");
                result["choices"][0]["message"] = json{
                    {"role", "assistant"},
                    {"content", ""}
                };
            }
            
            const auto& choice = chunk["choices"][0];
            
            // 更新finish_reason
            if (choice.contains("finish_reason") && !choice["finish_reason"].is_null()) {
                result["choices"][0]["finish_reason"] = choice["finish_reason"];
            }
            
            // 累积content
            if (choice.contains("delta") && choice["delta"].contains("content") && 
                !choice["delta"]["content"].is_null()) {
                combined_content += choice["delta"]["content"].get<std::string>();
            }
        }
        
        if (!result.empty()) {
            result["choices"][0]["message"]["content"] = combined_content;
        }
        
        return result;
    }
};

// 工具执行辅助函数（优化版）
json executeToolCalls(ToolExecutor* tool_executor, const json& tool_calls, json& messages) {
    for (const auto& tool_call : tool_calls) {
        // 提取工具调用信息
        std::string tool_id = tool_call.value("id", "");
        std::string tool_name = tool_call["function"]["name"];
        std::string args_str = tool_call["function"]["arguments"];
        
        LOG_INF("执行工具: %s (id: %s)\n", tool_name.c_str(), tool_id.c_str());
        
        // 解析参数
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
            result = tool_executor->execute(tool_name, arguments);
            LOG_INF("工具执行成功: %s\n", tool_name.c_str());
        } catch (const std::exception& e) {
            LOG_ERR("工具执行失败: %s: %s\n", tool_name.c_str(), e.what());
            result = json{
                {"error", "Tool execution failed"},
                {"details", e.what()}
            };
        }
        
        // 添加工具结果消息
        json tool_message = {
            {"role", "tool"},
            {"tool_call_id", tool_id},
            {"name", tool_name},
            {"content", result.dump()}
        };
        
        messages.push_back(tool_message);
    }
    
    return messages;
}

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

        // 主要的/v1/chat/completions端点实现
        server->Post("/v1/chat/completions", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                // 1. 验证请求
                if (req.body.empty()) {
                    json error = {{"error", {{"message", "Empty request body"}}}};
                    res.set_content(error.dump(), "application/json");
                    res.status = 400;
                    return;
                }
        
                // 2. 解析请求
                json request_body;
                try {
                    request_body = json::parse(req.body);
                } catch (const json::parse_error& e) {
                    json error = {{"error", {{"message", "Invalid JSON: " + std::string(e.what())}}}};
                    res.set_content(error.dump(), "application/json");
                    res.status = 400;
                    return;
                }
        
                // 3. 获取配置
                bool stream_mode = request_body.value("stream", false);
                json messages = request_body["messages"];
        
                // 4. 添加工具定义（如果需要）
                json all_tools = tool_executor->getTools();
                if (!request_body.contains("tools") && !all_tools.empty()) {
                    request_body["tools"] = all_tools;
                }
        
                // 5. 设置HTTP客户端
                llama_client->set_read_timeout(120);
                llama_client->set_write_timeout(60);
        
                // 6. 多轮对话循环（支持连续的tool calling）
                bool continue_conversation = true;
                std::string accumulated_sse_response;  // 累积的SSE响应（用于流模式）
                json final_response;                   // 最终响应（用于非流模式）
        
                while (continue_conversation) {
                    // 准备当前请求
                    json current_request = request_body;
                    current_request["messages"] = messages;
            
                    // 发送请求到llama-server
                    LOG_INF("发送请求到llama-server...\n");
                    auto llama_res = llama_client->Post(
                        "/v1/chat/completions",
                        current_request.dump(),
                        "application/json"
                    );
            
                    // 检查响应
                    if (!llama_res || llama_res->status != 200) {
                        LOG_ERR("llama-server响应错误: status=%d\n", 
                                llama_res ? llama_res->status : 0);
                        json error = {{"error", {{"message", "Failed to connect to llama-server"}}}};
                        res.set_content(error.dump(), "application/json");
                        res.status = 503;
                        return;
                    }
            
                    // 判断响应类型
                    bool is_sse_response = llama_res->body.find("data:") != std::string::npos;
            
                    if (is_sse_response) {
                        // 处理SSE流式响应
                        LOG_INF("处理SSE流式响应\n");
                
                        // 提取所有SSE chunks
                        auto chunks = SSEParser::extractSSEChunks(llama_res->body);
                        std::vector<json> json_chunks;
                        bool has_tool_calls = false;
                
                        // 解析每个chunk
                        for (const auto& chunk_str : chunks) {
                            if (chunk_str == "[DONE]") break;
                    
                            auto chunk_json = SSEParser::parseSSEChunk(chunk_str);
                            if (!chunk_json.empty()) {
                                json_chunks.push_back(chunk_json);
                        
                                // 检测tool calls
                                if (chunk_json.contains("choices") && !chunk_json["choices"].empty()) {
                                    const auto& choice = chunk_json["choices"][0];
                                    if (choice.contains("delta") && choice["delta"].contains("tool_calls")) {
                                        has_tool_calls = true;
                                    }
                                    if (choice.contains("finish_reason") && 
                                        choice["finish_reason"] == "tool_calls") {
                                        has_tool_calls = true;
                                    }
                                }
                            }
                        }
                
                        if (has_tool_calls) {
                            // 合并tool calling chunks
                            json merged = SSEParser::mergeToolCallChunks(json_chunks);
                    
                            if (merged.contains("choices") && !merged["choices"].empty() &&
                                merged["choices"][0].contains("message")) {
                                const auto& message = merged["choices"][0]["message"];
                        
                                // 添加assistant消息到历史
                                messages.push_back(message);
                        
                                // 执行工具调用
                                if (message.contains("tool_calls") && !message["tool_calls"].empty()) {
                                    LOG_INF("执行%zu个工具调用\n", message["tool_calls"].size());
                                    executeToolCalls(tool_executor.get(), message["tool_calls"], messages);
                            
                                    // 如果是流模式，累积响应（移除最后的[DONE]）
                                    if (stream_mode) {
                                        size_t done_pos = llama_res->body.rfind("data: [DONE]");
                                        if (done_pos != std::string::npos) {
                                            accumulated_sse_response += llama_res->body.substr(0, done_pos);
                                        } else {
                                            accumulated_sse_response += llama_res->body;
                                        }
                                    }
                            
                                    continue; // 继续下一轮对话
                                }
                            }
                        } else {
                            // 普通content响应
                            if (stream_mode) {
                                // 流模式：累积SSE响应
                                accumulated_sse_response += llama_res->body;
                            } else {
                                // 非流模式：合并为完整响应
                                final_response = SSEParser::mergeContentChunks(json_chunks);
                            }
                            continue_conversation = false;
                        }
                
                    } else {
                        // 处理JSON响应
                        LOG_INF("处理JSON响应\n");
                
                        try {
                            json response = json::parse(llama_res->body);
                    
                            // 检查是否有tool calls
                            bool has_tool_calls = false;
                            if (response.contains("choices") && !response["choices"].empty()) {
                                const auto& choice = response["choices"][0];
                                if (choice.contains("message") && choice["message"].contains("tool_calls") &&
                                    choice.value("finish_reason", "") == "tool_calls") {
                                    has_tool_calls = true;
                                }
                            }
                    
                            if (has_tool_calls) {
                                const auto& message = response["choices"][0]["message"];
                        
                                // 添加assistant消息到历史
                                messages.push_back(message);
                        
                                // 执行工具调用
                                LOG_INF("执行工具调用\n");
                                executeToolCalls(tool_executor.get(), message["tool_calls"], messages);
                        
                                continue; // 继续下一轮对话
                            } else {
                                // 普通响应，结束对话
                                final_response = response;
                                continue_conversation = false;
                            }
                    
                        } catch (const json::parse_error& e) {
                            LOG_ERR("解析llama-server响应失败: %s\n", e.what());
                            // 直接转发原始响应
                            res.set_content(llama_res->body, llama_res->get_header_value("Content-Type"));
                            res.status = llama_res->status;
                            return;
                        }
                    }
                }
        
                // 7. 返回最终响应
                if (stream_mode) {
                    // 流模式响应
                    res.set_header("Content-Type", "text/event-stream");
                    res.set_header("Cache-Control", "no-cache");
                    res.set_header("Connection", "keep-alive");
                    res.set_header("Access-Control-Allow-Origin", "*");
            
                    // 确保响应以data: [DONE]结尾
                    if (accumulated_sse_response.find("data: [DONE]") == std::string::npos) {
                        accumulated_sse_response += "data: [DONE]\n\n";
                    }
            
                    res.set_content(accumulated_sse_response, "text/event-stream");
                } else {
                    // 非流模式响应
                    res.set_header("Content-Type", "application/json");
                    res.set_content(final_response.dump(), "application/json");
                }
        
                res.status = 200;
        
            } catch (const std::exception& e) {
                LOG_ERR("处理请求异常: %s\n", e.what());
                json error = {{"error", {{"message", e.what()}}}};
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
