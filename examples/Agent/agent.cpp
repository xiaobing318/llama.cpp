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
            // 
            LOG_ERR("加载配置失败： %s\n", e.what());
            return false;
        }
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

        // TODO：等待服务器启动，更好的实现是自动检测启动是否成功。
        std::this_thread::sleep_for(std::chrono::seconds(5));

        // 创建一个 HTTP 客户端，可以理解成是一个 HTTP 请求的客户端，用于向 llama-server 发送请求。
        llama_client = std::make_unique<httplib::Client>(config.llama_server_host, config.llama_server_port);
        // 向 llama-server 服务器 /health 端口发送 GET 请求，检查服务器是否启动成功。
        auto res = llama_client->Get("/health");
        // 如果响应不为空且状态码为 200，则表示服务器启动成功。
        if (res && res->status == 200) {
            LOG_INF("llama-server 启动成功！\n");
            return true;
        }
        
        LOG_ERR("llama-server 启动失败！\n");
        return false;
    }

    void stopLlamaServer() {
        // 如果自动启动 llama-server 服务器被禁用，则不需要停止服务器。
        if (!config.auto_start_server) {
            return;
        }
        // 根据不同的平台，使用不同的方法停止 llama-server 进程。
#ifdef _WIN32
        // 如果 llama_process.hProcess 有效，则使用 TerminateProcess 终止进程，并关闭句柄。
        if (llama_process.hProcess) {
            TerminateProcess(llama_process.hProcess, 0);
            CloseHandle(llama_process.hProcess);
            CloseHandle(llama_process.hThread);
        }
#else
        // 如果 llama_pid 大于 0，则使用 kill 函数发送 SIGTERM 信号终止进程。
        if (llama_pid > 0) {
            kill(llama_pid, SIGTERM);
        }
#endif
        LOG_INF("llama-server 已停止。\n");
    }

    void setupRoutes() {
        // Health check（应该间接的检查 llama-server 的 /health 端口）
        server->Get("/health", [](const httplib::Request&, httplib::Response& res) {
            json response = {{"status", "ok"}};
            res.set_content(response.dump(), "application/json");
        });

        // List available tools
        server->Get("/tools", [this](const httplib::Request&, httplib::Response& res) {
            json response = {{"tools", config.tools}};
            res.set_content(response.dump(), "application/json");
        });

        // Chat completion with tool support
        server->Post("/v1/chat/completions", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                json request_body = json::parse(req.body);

                // Add tools to request if not present
                if (!request_body.contains("tools") && !config.tools.empty()) {
                    request_body["tools"] = config.tools;
                }

                // Forward to llama-server
                auto llama_res = llama_client->Post("/v1/chat/completions",
                    request_body.dump(), "application/json");

                if (!llama_res) {
                    json error = {{"error", "Failed to connect to llama-server"}};
                    res.set_content(error.dump(), "application/json");
                    res.status = 500;
                    return;
                }

                json response = json::parse(llama_res->body);

                // Check for tool calls in response
                if (response.contains("choices") && !response["choices"].empty()) {
                    auto& choice = response["choices"][0];
                    if (choice.contains("message") && choice["message"].contains("tool_calls")) {
                        // Execute tool calls
                        json tool_results = json::array();
                        for (const auto& tool_call : choice["message"]["tool_calls"]) {
                            std::string function_name = tool_call["function"]["name"];
                            json arguments = json::parse(tool_call["function"]["arguments"].get<std::string>());

                            LOG_INF("Executing tool: %s\n", function_name.c_str());
                            json result = tool_executor->execute(function_name, arguments);

                            tool_results.push_back({
                                {"tool_call_id", tool_call["id"]},
                                {"role", "tool"},
                                {"name", function_name},
                                {"content", result.dump()}
                            });
                        }

                        // Add tool results to conversation and get final response
                        auto messages = request_body["messages"];
                        messages.push_back(choice["message"]);
                        for (const auto& result : tool_results) {
                            messages.push_back(result);
                        }

                        json final_request = request_body;
                        final_request["messages"] = messages;
                        final_request.erase("tools");  // Remove tools for final call

                        auto final_res = llama_client->Post("/v1/chat/completions",
                            final_request.dump(), "application/json");

                        if (final_res) {
                            response = json::parse(final_res->body);
                            response["tool_results"] = tool_results;
                        }
                    }
                }

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

        /*
        // Proxy other requests to llama-server
        server->set_default_handler([this](const httplib::Request& req, httplib::Response& res) {
            httplib::Client client(config.llama_server_host, config.llama_server_port);

            httplib::Result result;
            if (req.method == "GET") {
                result = client.Get(req.path);
            } else if (req.method == "POST") {
                result = client.Post(req.path, req.body, req.get_header_value("Content-Type"));
            } else {
                res.status = 405;
                return;
            }

            if (result) {
                res.set_content(result->body, result->get_header_value("Content-Type"));
                res.status = result->status;
            } else {
                res.status = 502;
            }
        });
        */
    }

    bool start() {
        // 初始化一个指向 llama-server 服务的客户端。
        llama_client = std::make_unique<httplib::Client>(
            config.llama_server_host, config.llama_server_port);

        // 如果启动 llama-server 失败的话直接返回。
        if (!startLlamaServer()) {
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

void signal_handler(int) {
    g_running = false;
}

int main(int argc, char** argv) {
    // 初始化程序环境，包括设置 UTF-8 区域和在 Windows 上启用 UTF-8 控制台输出。
    common_init();

    // 解析命令行参数，获取配置文件的路径
    std::string config_file = "config.json";
    if (argc > 1) {
        config_file = argv[1];
    }

    // 设置信号处理器以捕获终止信号
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 创建 LlamaAgent 实例
    LlamaAgent agent;

    // 如果代理实例加载配置文件失败，则输出错误信息并退出程序。
    if (!agent.loadConfig(config_file)) {
        LOG_ERR("加载配置失败！\n");
        return 1;
    }

    // 如果代理实例启动失败，则输出错误信息并退出程序。
    if (!agent.start()) {
        LOG_ERR("启动 Agent 失败！\n");
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

    return 0;
}
