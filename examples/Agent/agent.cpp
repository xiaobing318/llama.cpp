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
                LOG_ERR("Failed to open config file: %s\n", config_file.c_str());
                return false;
            }

            json j;
            file >> j;

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

            // Register tools
            for (const auto& tool : config.tools) {
                tool_executor->registerTool(tool);
            }

            LOG_INF("Configuration loaded successfully\n");
            return true;
        } catch (const std::exception& e) {
            LOG_ERR("Failed to load config: %s\n", e.what());
            return false;
        }
    }

    bool startLlamaServer() {
        if (!config.auto_start_server) {
            LOG_INF("Auto-start disabled, assuming llama-server is already running\n");
            return true;
        }

        std::string cmd = config.llama_server_path;
        cmd += " -m " + config.model_path;
        cmd += " --host " + config.llama_server_host;
        cmd += " --port " + std::to_string(config.llama_server_port);
        cmd += " -c " + std::to_string(config.n_ctx);
        cmd += " --jinja";
        if (config.n_gpu_layers >= 0) {
            cmd += " -ngl " + std::to_string(config.n_gpu_layers);
        }

        LOG_INF("Starting llama-server: %s\n", cmd.c_str());

#ifdef _WIN32
        STARTUPINFOA si = {sizeof(si)};
        if (!CreateProcessA(NULL, const_cast<char*>(cmd.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &si, &llama_process)) {
            LOG_ERR("Failed to start llama-server\n");
            return false;
        }
#else
        llama_pid = fork();
        if (llama_pid == 0) {
            // Child process
            system(cmd.c_str());
            exit(0);
        } else if (llama_pid < 0) {
            LOG_ERR("Failed to fork process\n");
            return false;
        }
#endif

        // Wait for server to start
        std::this_thread::sleep_for(std::chrono::seconds(5));

        // Check if server is running
        llama_client = std::make_unique<httplib::Client>(config.llama_server_host, config.llama_server_port);
        auto res = llama_client->Get("/health");
        if (res && res->status == 200) {
            LOG_INF("llama-server started successfully\n");
            return true;
        }

        LOG_ERR("llama-server failed to start\n");
        return false;
    }

    void stopLlamaServer() {
        if (!config.auto_start_server) {
            return;
        }

#ifdef _WIN32
        if (llama_process.hProcess) {
            TerminateProcess(llama_process.hProcess, 0);
            CloseHandle(llama_process.hProcess);
            CloseHandle(llama_process.hThread);
        }
#else
        if (llama_pid > 0) {
            kill(llama_pid, SIGTERM);
        }
#endif
        LOG_INF("llama-server stopped\n");
    }

    void setupRoutes() {
        // Health check
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
        // Initialize llama client
        llama_client = std::make_unique<httplib::Client>(
            config.llama_server_host, config.llama_server_port);

        // Start llama-server if needed
        if (!startLlamaServer()) {
            return false;
        }

        // Setup routes
        setupRoutes();

        // Start agent server
        running = true;
        server_thread = std::thread([this]() {
            LOG_INF("Agent server listening on http://%s:%d\n",
                config.agent_host.c_str(), config.agent_port);
            server->listen(config.agent_host, config.agent_port);
        });

        return true;
    }

    void stop() {
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
    // Initialize
    common_init();

    // Parse arguments
    std::string config_file = "config.json";
    if (argc > 1) {
        config_file = argv[1];
    }

    // Setup signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Create and start agent
    LlamaAgent agent;

    if (!agent.loadConfig(config_file)) {
        LOG_ERR("Failed to load configuration\n");
        return 1;
    }

    if (!agent.start()) {
        LOG_ERR("Failed to start agent\n");
        return 1;
    }

    LOG_INF("Agent is running. Press Ctrl+C to stop.\n");

    // Wait for shutdown
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    LOG_INF("Shutting down...\n");
    agent.stop();

    return 0;
}
