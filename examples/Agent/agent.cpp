#include "common.h"
#include "httplib.h"
#include "json.hpp"
#include "agent_utils.h"
#include "tool_executor.h"

#include <atomic>
#include <thread>
#include <chrono>
#include <signal.h>
#include <fstream>
#include <memory>
#include <cstring>
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/wait.h>
    #include <cerrno>
    #include <unistd.h>
#endif

using json = nlohmann::ordered_json;

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
    AgentConfig config;
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
            // 如果打开配置文件成功，则使用 nlohmann::json 库解析 JSON 格式的配置文件。
            json j;
            file >> j;
            // 使用 nlohmann::json 库的 value 方法获取配置项的值，如果配置项不存在，则使用默认值。
            config.agent_host = j.value("agent_host", config.agent_host);
            config.agent_port = j.value("agent_port", config.agent_port);
            config.llama_server_host = j.value("llama_server_host", config.llama_server_host);
            config.llama_server_port = j.value("llama_server_port", config.llama_server_port);
            config.llama_server_path = j.value("llama_server_path", config.llama_server_path);
            config.model_path = j.value("model_path", config.model_path);
            config.n_ctx = j.value("n_ctx", config.n_ctx);
            config.n_gpu_layers = j.value("n_gpu_layers", config.n_gpu_layers);
            config.auto_start_server = j.value("auto_start_server", config.auto_start_server);
            config.tools = j.value("tools", json::array());

            // 将配置文件中的配置的工具注册到 ToolExecutor 中
            for (const auto& tool : config.tools) {
                tool_executor->registerTool(tool);
            }

            LOG_INF("配置加载成功！\n");
            return true;
        } catch (const std::exception& e) {
            LOG_ERR("加载配置失败： %s\n", e.what());
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
        if (!config.auto_start_server) {
            LOG_INF(" auto_start_server 已禁用，这里假设 llama-server 已在运行！\n");
            return true;
        }
        // 拼接 llama-server 的命令行参数。
        std::string cmd = config.llama_server_path;
        cmd += " -m " + config.model_path;
        cmd += " --host " + config.llama_server_host;
        cmd += " --port " + std::to_string(config.llama_server_port);
        cmd += " -c " + std::to_string(config.n_ctx);
        cmd += " --jinja";
        if (config.n_gpu_layers >= 0) {
            cmd += " -ngl " + std::to_string(config.n_gpu_layers);
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
        llama_client = std::make_unique<httplib::Client>(config.llama_server_host, config.llama_server_port);

        // 等待并检测 llama-server 启动状态
        return waitForServerStartup();
    }

    void stopLlamaServer() {
        // 如果自动启动 llama-server 服务器被禁用，则不需要停止服务器。
        if (!config.auto_start_server) {
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

        // Chat completion with tool support
        server->Post("/v1/chat/completions", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                // 解析请求体中的 JSON 数据
                json request_body = json::parse(req.body);

                /*
                1、如果请求体中不存在 "tools" 字段，则需要将工具执行器中的工具（包含内置工具定义、配置中的外部工具定义）添加到请求体中。
                2、TODO：应该不考虑请求体中的工具定义，因为没有具体的实现。
                */
                json all_tools = tool_executor->getTools();
                if (!request_body.contains("tools") && !all_tools.empty()) {
                    request_body["tools"] = all_tools;
                }

                // Forward to llama-server
                auto llama_res = llama_client->Post("/v1/chat/completions",
                    request_body.dump(), "application/json");

                // 如果 llama-server 的响应体为空，则设置 llama-agent 的响应体。
                if (!llama_res) {
                    json error = {{"error", "Failed to connect to llama-server"}};
                    res.set_content(error.dump(), "application/json");
                    res.status = 500;
                    return;
                }

                /*
                1、解析 llama-server 的响应
                2、下列是 llama-server 响应的一个 JSON 格式的例子。
                {
                    "choices": [
                        {
                            "finish_reason": "tool_calls",
                            "index": 0,
                            "message": {
                                "role": "assistant",
                                "content": null,
                                "tool_calls": [
                                    {
                                        "type": "function",
                                        "function": {
                                            "name": "encoding_detection",
                                            "arguments": "{\"layer_path\":\"F:/llama.cpp-data/models/DN12.shp\"}"
                                        },
                                        "id": "Hd6Adx1ePqjQearhRcp9Wlnadd36iyXQ"
                                    }
                                ]
                            }
                        }
                    ],
                    "created": 1754833006,
                    "model": "Qwen3-4B-Q8_0",
                    "system_fingerprint": "b5215-5f5e39e1",
                    "object": "chat.completion",
                    "usage": {
                        "completion_tokens": 33,
                        "prompt_tokens": 607,
                        "total_tokens": 640
                    },
                    "id": "chatcmpl-Xd5V9Yp5nQud9zxLvEzB6md7c9FeC3vQ",
                    "timings": {
                        "prompt_n": 607,
                        "prompt_ms": 368.11,
                        "prompt_per_token_ms": 0.6064415156507413,
                        "prompt_per_second": 1648.9636250033957,
                        "predicted_n": 33,
                        "predicted_ms": 675.522,
                        "predicted_per_token_ms": 20.470363636363636,
                        "predicted_per_second": 48.851110696616836
                    }
                }
                */
                json response = json::parse(llama_res->body);

                // 如果 llama-server 响应体中包含 "choices" 字段，并且该字段不为空，则执行代码块中的内容。
                if (response.contains("choices") && !response["choices"].empty()) {
                    // 获取得到 llama-server 响应体中的第一个 choice 的内容。
                    auto& choice = response["choices"][0];
                    // 如果 choice 中包含 "message" 字段，并且该字段中包含 "tool_calls" 字段，则执行工具调用。
                    if (choice.contains("message") && choice["message"].contains("tool_calls")) {
                        // 创建一个 JSON 数组来存储工具调用的结果。
                        json tool_results = json::array();
                        // 循环遍历每个工具调用，并执行相应的工具。
                        for (const auto& tool_call : choice["message"]["tool_calls"]) {
                            // 获取当前工具调用的名称和参数，并将其解析为 JSON 对象。
                            std::string function_name = tool_call["function"]["name"];
                            json arguments = json::parse(tool_call["function"]["arguments"].get<std::string>());

                            LOG_INF("执行工具: %s\n", function_name.c_str());
                            // 调用 ToolExecutor 执行工具，并获取结果。
                            json result = tool_executor->execute(function_name, arguments);
                            // 将调用工具结果以及一些额外信息包装成 JSON 保存到 tool_results 数组中，这里假设会执行多个工具调用。
                            tool_results.push_back({
                                {"tool_call_id", tool_call["id"]},
                                {"role", "tool"},
                                {"name", function_name},
                                {"content", result.dump()}
                            });
                        }

                        // 获取请求体中的 "messages" 字段。
                        auto messages = request_body["messages"];
                        // 将 llama-server 响应体中的 choice 的 message 添加到 messages 中，相当于将模型的响应添加到消息列表中。
                        messages.push_back(choice["message"]);
                        // 循环将工具调用的结果添加到消息列表中。
                        for (const auto& result : tool_results) {
                            messages.push_back(result);
                        }

                        // 支持多次工具调用循环，只要响应中包含 tool_calls 就继续执行
                        json all_tool_results = json::array();
                        for (const auto& result : tool_results) {
                            all_tool_results.push_back(result);
                        }

                        // 防止无限循环
                        int max_iterations = 10;
                        int iteration = 0;

                        while (iteration < max_iterations) {
                            json continue_request = request_body;
                            continue_request["messages"] = messages;
                            // 第一轮之后的请求中不需要 tools 字段，减少 token 消耗
                            if (iteration > 0) {
                                continue_request.erase("tools");
                            }

                            auto continue_res = llama_client->Post("/v1/chat/completions",
                                continue_request.dump(), "application/json");

                            if (!continue_res) {
                                break;
                            }

                            response = json::parse(continue_res->body);

                            // 检查是否还有工具调用
                            bool has_tool_calls = false;
                            if (response.contains("choices") && !response["choices"].empty()) {
                                auto& choice = response["choices"][0];
                                if (choice.contains("message") && choice["message"].contains("tool_calls")) {
                                    has_tool_calls = true;

                                    // 将当前助手响应添加到消息历史
                                    messages.push_back(choice["message"]);

                                    // 执行新的工具调用
                                    for (const auto& tool_call : choice["message"]["tool_calls"]) {
                                        std::string function_name = tool_call["function"]["name"];
                                        json arguments = json::parse(tool_call["function"]["arguments"].get<std::string>());

                                        LOG_INF("执行工具 (第%d轮): %s\n", iteration + 1, function_name.c_str());
                                        json result = tool_executor->execute(function_name, arguments);

                                        json tool_result = {
                                            {"tool_call_id", tool_call["id"]},
                                            {"role", "tool"},
                                            {"name", function_name},
                                            {"content", result.dump()}
                                        };

                                        messages.push_back(tool_result);
                                        all_tool_results.push_back(tool_result);
                                    }
                                }
                            }

                            // 如果没有更多工具调用，结束循环
                            if (!has_tool_calls) {
                                break;
                            }

                            iteration++;
                        }

                        if (iteration >= max_iterations) {
                            LOG_WRN("工具调用循环达到最大次数限制 (%d)，强制结束\n", max_iterations);
                        }

                        // 向最终响应中添加所有工具调用的结果
                        response["tool_results"] = all_tool_results;
                    }
                }
                // 将最终的响应体设置到 HTTP 响应中。
                res.set_content(response.dump(), "application/json");
                res.status = llama_res->status;

            } catch (const std::exception& e) {
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
    }

    bool start() {
        // 初始化一个指向 llama-server 服务的客户端。
        llama_client = std::make_unique<httplib::Client>(
            config.llama_server_host, config.llama_server_port);

        // 如果启动 llama-server 失败的话直接返回。
        if (!startLlamaServer()) {
            LOG_ERR("无法启动 llama-server，Agent 启动失败\n");
            LOG_ERR("请检查：\n");
            LOG_ERR("  1. llama-server 路径是否正确: %s\n", config.llama_server_path.c_str());
            LOG_ERR("  2. 模型文件路径是否正确: %s\n", config.model_path.c_str());
            LOG_ERR("  3. 端口 %d 是否被占用\n", config.llama_server_port);
            LOG_ERR("  4. 系统资源是否充足（内存、GPU等）\n");
            return false;
        }

        // 设置 llama-agent 服务的 endpoints 。
        setupRoutes();

        // 启动 llama-agent 服务。
        running = true;
        server_thread = std::thread([this]() {
            LOG_INF("代理服务器正在监听 http://%s:%d\n",
                config.agent_host.c_str(), config.agent_port);
            server->listen(config.agent_host, config.agent_port);
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
        std::string arg = argv[i];

        if (arg == "--help") {
            args.show_help = true;
            return true;
        }
        else if (arg == "--version") {
            args.show_version = true;
            return true;
        }
        else if (arg == "--config-file-path") {
            if (i + 1 >= argc) {
                std::cerr << "错误: " << arg << " 选项需要一个参数\n";
                return false;
            }
            args.config_file_path = argv[++i];
        }
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
