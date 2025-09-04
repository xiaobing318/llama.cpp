/*
 * C++头文件包含顺序的重要性：
 * 
 * 为什么要按特定顺序包含头文件？
 * 1. 验证头文件自包含性 - 确保每个.h文件都包含了所需的依赖
 * 2. 避免隐式依赖 - 防止系统头文件意外提供你需要的声明
 * 3. 早期发现编译错误 - 如果头文件有问题，立即暴露而不是隐藏
 * 
 * 示例：如果先包含<iostream>，它可能间接包含<string>，
 * 导致你的头文件看似正常但实际缺少#include <string>
 */

// 1. 相关项目头文件
#include "qcopilot_utils.h"
#include "qcopilot_executor.h"

// 2. llama.cpp项目头文件
#include "common.h"

// 3. 第三方库头文件
#include "httplib.h"
#include "json.hpp"

// 4. 生成的资源头文件
#include "index.html.gz.hpp"
#include "loading.html.hpp"

// 5. C++标准库头文件
#include <atomic>
#include <thread>
#include <chrono>
#include <fstream>
#include <memory>
#include <cstring>
#include <iostream>
#include <sstream>
#include <ctime>
#include <deque>

// 6. C标准库头文件
#include <signal.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/wait.h>
    #include <cerrno>
    #include <unistd.h>
#endif

using json = nlohmann::ordered_json;

// 生成当前 SSE 会话里稳定不变的 id（整条流复用）
static std::string make_stream_id() {
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    char buf[64];
    snprintf(buf, sizeof(buf), "chatcmpl-qcopilot-%llx", (unsigned long long) now);
    return std::string(buf);
}

// 统一构造 OpenAI 形状的 streaming chunk（chat.completion.chunk）
static std::string build_delta_chunk(const std::string &stream_id,
                                     const std::string &model_name,
                                     const nlohmann::ordered_json &delta) {
    nlohmann::ordered_json chunk = {
        {"id",      stream_id},
        {"object",  "chat.completion.chunk"},
        {"created", (long long) time(nullptr)},
        {"model",   model_name},
        {"choices", nlohmann::ordered_json::array({
            nlohmann::ordered_json{
                {"index", 0},
                {"delta", delta},
                {"finish_reason", nullptr}
            }
        })}
    };
    return std::string("data: ") + chunk.dump() + "\n\n";
}


struct SSEBridge {
    // SSEBridge（阻塞队列）

    std::mutex m;
    std::condition_variable cv;
    std::deque<std::string> q;
    bool closed = false;

    void push(std::string s) {
        {
          std::lock_guard<std::mutex> lk(m);
          q.emplace_back(std::move(s));
        }
        cv.notify_one();
    }

    // pop 一条；若 closed 且队空，则返回 false
    bool pop(std::string &out) {
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [&]{ return !q.empty() || closed; });
        if (q.empty()) return false;
        out = std::move(q.front());
        q.pop_front();
        return true;
    }

    void close() {
        {
          std::lock_guard<std::mutex> lk(m);
          closed = true;
        }
        cv.notify_all();
    }
};

struct QCopilotConfig {
    std::string qcopilot_host = "127.0.0.1";
    int qcopilot_port = 8081;
    std::string base_server_host = "127.0.0.1";
    int base_server_port = 8080;
    std::string base_server_path = "./llama-server";
    std::string model_path = "";
    int n_ctx = 2048;
    int n_gpu_layers = -1;
    bool auto_start_base_server = true;
    json tools;
};

struct CommandLineArgs {
    std::string config_file_path = "./QCopilotConfig.json";
    bool show_help = false;
    bool show_version = false;
};

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
                    /*
                    1、fix:{"content", ""}--->{"content", nullptr}
                    2、TODO：针对这里的 content 字段或许可以提一个 PR ，因为 jinja(minja)模板在解析 content 的时候如果 content 为空
                    指针则会崩溃，因此需要弄清楚 server.cpp 中为什么返回 content 这个字段的时候返回的是空指针。
                    */
                    {"content", ""},
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

static json executeToolCalls(ToolExecutor* tool_executor, const json& tool_calls, json& messages) {
    //  在单个推理过程中可能会存在多个 tool calling/function calling，因此这里最好是循环处理每一个  tool calling/function calling。
    for (const auto& tool_call : tool_calls) {
        //  从当前 function calling 中提取 id 字段，如果没有则置空。
        std::string tool_id = tool_call.value("id", "");
        //  从当前 function calling 中提取 name 字段。
        std::string tool_name = tool_call["function"]["name"];
        //  从当前 function calling 中提取 arguments 字段。
        std::string args_str = tool_call["function"]["arguments"];

#ifdef NDEBUG
        /*
        1、Release 模式：不输出完整的工具调用命令到终端。
        2、TODO:需要完善日志功能。
        */
#else
        /*
        1、Debug 模式：输出完整的工具调用命令到终端
        2、TODO:需要完善日志功能。
        */
        LOG_INF("<=== 工具调用 ===>\n");
        LOG_INF("工具名称: %s\n", tool_name.c_str());
        LOG_INF("工具ID: %s\n", tool_id.c_str());
        LOG_INF("调用参数: %s\n", args_str.c_str());
        LOG_INF("<===============>\n");          
#endif
        //  创建一个临时变量用来存储调用工具所需要的参数。
        json arguments;
        try {
            //  从 JSON 数据中解析调用工具所需要的参数。
            arguments = json::parse(args_str);
        } catch (const json::parse_error& e) {
            //  输出错误日志说明解析调用工具所需参数失败。
            LOG_ERR("解析工具参数失败: %s\n", e.what());
            //  将调用工具所需参数置空，这里不应该直接返回结果，因为有些工具的确是不需要参数的。
            arguments = json::object();
        }

        //  创建两个临时变量用来保存工具调用处理结果。
        json result;
        std::string result_content;
        try {
            //  使用特定参数调用指定工具。
            result = tool_executor->execute(tool_name, arguments);
            LOG_INF("名为[%s]工具执行成功！\n", tool_name.c_str());
            //  将工具执行的 JSON 结果序列化为字符串。
            result_content = result.dump();
        } catch (const std::exception& e) {
            LOG_ERR("名为[%s]工具执行失败，错误日志：%s\n", tool_name.c_str(), e.what());
            result = json{
                {"error", "Tool execution failed"},
                {"details", e.what()}
            };
            result_content = result.dump();
        }
        //  判断序列话的工具执行结果是否为空。
        if (result_content.empty()) {
            result_content = "{}";
        }

#ifdef NDEBUG
        /*
        1、Release 模式：不输出完整的工具调用命令到终端。
        2、TODO:需要完善日志功能。
        */
#else
        /*
        1、Debug 模式：输出完整的工具调用命令到终端
        2、TODO:需要完善日志功能。
        */
        LOG_INF("<=== 工具执行结果 ===>\n");
        LOG_INF("工具名称: %s\n", tool_name.c_str());
        LOG_INF("执行结果: %s\n", result_content.c_str());
        LOG_INF("<===================>\n");       
#endif


        // 确保工具消息格式完整，包含所有必要字段
        json tool_message = {
            {"role", "tool"},
            {"tool_call_id", tool_id},
            {"name", tool_name},
            {"content", result_content},
            {"reasoning_content", ""}
        };
        //  将工具执行结果保存到消息中用来再次给到推理引擎。
        messages.push_back(tool_message);
    }

    return messages;
}

/*
1、对 llama-server 返回的响应体中的 messages 字段进行修正。
2、TODO:需要对 llama-server 返回响应体机制进行理解，最新分支可能已经解决了该问题。
*/
static void normalize_messages_for_llama(json& messages) {
    for (auto &m : messages) {
        // 确保所有消息都有content字段且为字符串
        if (!m.contains("content")) {
            m["content"] = "";
        } else if (m["content"].is_null()) {
            m["content"] = "";
        } else if (!m["content"].is_string()) {
            if (m["content"].is_object() || m["content"].is_array()) {
                m["content"] = m["content"].dump();
            } else {
                m["content"] = "";
            }
        }

        // 确保所有消息都有reasoning_content字段且为字符串
        if (!m.contains("reasoning_content")) {
            m["reasoning_content"] = "";
        } else if (m["reasoning_content"].is_null()) {
            m["reasoning_content"] = "";
        } else if (!m["reasoning_content"].is_string()) {
            m["reasoning_content"] = "";
        }

        // 特别处理tool角色的消息
        if (m.contains("role") && m["role"] == "tool") {
            // tool消息必须有content
            if (m["content"].get<std::string>().empty()) {
                m["content"] = "Tool executed successfully";
            }
            // 确保有tool_call_id
            if (!m.contains("tool_call_id")) {
                m["tool_call_id"] = "";
            }
            // 确保有name
            if (!m.contains("name")) {
                m["name"] = "unknown_tool";
            }
        }

        // 处理assistant角色带tool_calls的情况
        if (m.contains("role") && m["role"] == "assistant") {
            if (m.contains("tool_calls") && m["tool_calls"].is_array()) {
                // 确保content不为null
                if (!m.contains("content") || !m["content"].is_string()) {
                    m["content"] = "";
                }
            }
        }
    }
}

// no streaming mode merge：补上 reasoning_content
static nlohmann::ordered_json merge_with_reasoning(const std::vector<nlohmann::ordered_json> &chunks){
    json result;
    std::string content, reasoning;

    for (const auto &chunk : chunks) {
      if (!chunk.contains("choices") || chunk["choices"].empty()) continue;
      if (result.empty()) {
        result = chunk;
        result["choices"][0].erase("delta");
        result["choices"][0]["message"] = json{
          {"role","assistant"},
          {"content",""},
          {"reasoning_content",""}
        };
      }
      const auto &choice = chunk["choices"][0];
      if (choice.contains("finish_reason") && !choice["finish_reason"].is_null()) {
        result["choices"][0]["finish_reason"] = choice["finish_reason"];
      }
      if (choice.contains("delta")) {
        const auto &d = choice["delta"];
        if (d.contains("content") && !d["content"].is_null()) {
          content += d["content"].get<std::string>();
        }
        if (d.contains("reasoning_content") && !d["reasoning_content"].is_null()) {
          reasoning += d["reasoning_content"].get<std::string>();
        }
      }
    }
    if (!result.empty()) {
      result["choices"][0]["message"]["content"] = content;
      result["choices"][0]["message"]["reasoning_content"] = reasoning;
    }
    return result;
}

// 帮助函数：POST + streaming（使用 httplib 的 send + content_receiver）
static bool post_stream(
    httplib::Client &cli,
    const std::string &path,
    const std::string &json_body,
    const std::function<bool(const httplib::Response&)> &on_response,
    const std::function<bool(const char*, size_t, uint64_t, uint64_t)> &on_chunk,
    httplib::Response &out_resp,
    httplib::Error &out_err) {

    httplib::Request req;
    req.method = "POST";
    req.path   = path;
    req.headers.emplace("Content-Type", "application/json");
    // 接受 SSE 或 JSON
    req.headers.emplace("Accept", "text/event-stream, application/json");
    req.body = json_body;

    req.response_handler = on_response;
    req.content_receiver = on_chunk;

    return cli.send(req, out_resp, out_err);
}

/*
一轮：把 llama-server 的 SSE 原样转到 bridge，但：
- “吞掉”本轮末尾的 data: [DONE]，避免前端提前关流
- 收集 JSON 块以判断是否出现 tool_calls，并把分片 tool_calls 合并回来
*/
static bool forward_llama_sse_once(
    httplib::Client &cli,
    const nlohmann::ordered_json &request_body,
    SSEBridge &bridge,
    nlohmann::ordered_json &out_merged_tool_message,
    bool &out_saw_done_marker,
    bool &out_has_tool_calls,
    const std::string &stream_id,
    const std::string &model_name,
    int round_number) {

    std::atomic<bool> saw_tool_calls{false};
    out_saw_done_marker = false;
    out_has_tool_calls = false;
    out_merged_tool_message = nlohmann::ordered_json();

    std::string buf;
    std::vector<nlohmann::ordered_json> json_chunks;

    httplib::Response resp;
    httplib::Error err;

    bool ok = post_stream(
        cli, "/v1/chat/completions", request_body.dump(),
        [&](const httplib::Response &r) { return r.status == 200; },
        [&](const char *data, size_t len, uint64_t, uint64_t) {
            buf.append(data, len);
            size_t pos;
            while ((pos = buf.find('\n')) != std::string::npos) {
                std::string line = buf.substr(0, pos);
                buf.erase(0, pos + 1);
                if (!line.empty() && line.back() == '\r') line.pop_back();

                if (line.empty() || line[0] == ':') continue;
                if (line.rfind("data:", 0) != 0) continue;

                std::string payload = line.substr(5);
                size_t st = payload.find_first_not_of(" \t");
                if (st != std::string::npos) payload = payload.substr(st);

                // [DONE] 只记录，不转发（中间轮一定不能把 DONE 发到前端）
                if (payload == "[DONE]") {
                    out_saw_done_marker = true;
                    continue;
                }

                auto j = nlohmann::ordered_json::parse(payload, nullptr, false);
                if (j.is_discarded()) {
                    // 异常块：原样透传，避免丢信息
                    bridge.push(std::string("data: ") + payload + "\n\n");
                    continue;
                }

                // 归一化 id/object/model：同一 HTTP 响应内保持稳定
                j["id"]     = stream_id;
                j["object"] = "chat.completion.chunk";
                if (!model_name.empty()) j["model"] = model_name;

                // —— 在这里先判断是否“带结束语义的 tool_calls 块” —— //
                bool this_is_finish_tool_calls = false;
                if (j.contains("choices") && !j["choices"].empty()) {
                    auto &choice = j["choices"][0];

                    // 记录是否出现工具调用（用于会后合并、驱动下一轮）
                    if ((choice.contains("delta")   && choice["delta"].contains("tool_calls")) ||
                        (choice.contains("message") && choice["message"].contains("tool_calls"))) {
                        saw_tool_calls = true;
                    }

                    // “结束语义”：finish_reason == "tool_calls"
                    if (choice.contains("finish_reason") && choice["finish_reason"].is_string()
                        && choice["finish_reason"] == "tool_calls") {
                        saw_tool_calls = true;
                        this_is_finish_tool_calls = true;
                    }

                    // 多轮 role 处理：避免新开一条“消息”
                    if (round_number > 1 && choice.contains("delta") && choice["delta"].is_object()) {
                        auto &delta = choice["delta"];
                        // 若本分片仅含 role，跳过（UI 不需要）
                        if (delta.contains("role") && delta.size() == 1) {
                            // 即使跳过，也要把原始块参与到 tool_calls 合并
                            json_chunks.push_back(j);
                            continue;
                        }
                        // 若 role 与其他键并存，仅移除 role
                        if (delta.contains("role")) {
                            delta.erase("role");
                        }
                    }

                    // 字段兜底：有 reasoning_content 就补上 content = ""
                    if (choice.contains("delta") && choice["delta"].is_object()) {
                        auto &delta = choice["delta"];
                        if (!delta.contains("content") && delta.contains("reasoning_content")) {
                            delta["content"] = "";
                        }
                    }
                }

                // 参与工具合并判断（必须在可能 continue 之前做）
                json_chunks.push_back(j);

                // 关键：**屏蔽**带 finish_reason:"tool_calls" 的块，避免前端误以为一轮已结束
                if (this_is_finish_tool_calls) {
                    continue;
                }

                // 其他块正常转发给前端
                bridge.push(std::string("data: ") + j.dump() + "\n\n");
            }
            return true;
        },
        resp, err
    );

    if (!ok || resp.status != 200) return false;

    out_has_tool_calls = saw_tool_calls.load();
    if (saw_tool_calls) {
        out_merged_tool_message = SSEParser::mergeToolCallChunks(json_chunks);
    }
    // 返回true表示成功处理
    return true;
}

class QCopilot {
private:
    struct QCopilotConfig QCopilotConfig;
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
    QCopilot() {
        server = std::make_unique<httplib::Server>();
        tool_executor = std::make_unique<ToolExecutor>();
    }

    ~QCopilot() {
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
            QCopilotConfig.qcopilot_host = j.value("qcopilot_host", QCopilotConfig.qcopilot_host);
            QCopilotConfig.qcopilot_port = j.value("qcopilot_port", QCopilotConfig.qcopilot_port);
            QCopilotConfig.base_server_host = j.value("base_server_host", QCopilotConfig.base_server_host);
            QCopilotConfig.base_server_port = j.value("base_server_port", QCopilotConfig.base_server_port);
            QCopilotConfig.base_server_path = j.value("base_server_path", QCopilotConfig.base_server_path);
            QCopilotConfig.model_path = j.value("model_path", QCopilotConfig.model_path);
            QCopilotConfig.n_ctx = j.value("n_ctx", QCopilotConfig.n_ctx);
            QCopilotConfig.n_gpu_layers = j.value("n_gpu_layers", QCopilotConfig.n_gpu_layers);
            QCopilotConfig.auto_start_base_server = j.value("auto_start_base_server", QCopilotConfig.auto_start_base_server);
            QCopilotConfig.tools = j.value("tools", json::array());

            // 将配置文件中的配置的工具注册到 ToolExecutor 中，设置一个标志用来判断配置中的工具定义是否有效。
            bool areToolsDefinitionValid = true;
            for (const auto& tool : QCopilotConfig.tools) {
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
        // 最多尝试 300 次检查。
        const int max_attempts = 300;
        // 每次检查间隔 1 秒，总共可以预留 300 秒（5 mins）的时间让 llama-server 进行启动。
        const int retry_interval_ms = 1000;

        LOG_INF("正在等待 llama-server 启动...\n");

        for (int attempt = 1; attempt <= max_attempts; ++attempt) {
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

            // 尝试连接健康检查端点。
            auto res = llama_client->Get("/health");
            if (res && res->status == 200) {
                LOG_INF(" llama-server 启动成功！(等待次数 %d/%d 次/秒)\n", attempt, max_attempts);
                return true;
            }

            // 输出等待进度，每两次检查输出一次进度。
            if (attempt % 2 == 0) {
                LOG_INF("等待 llama-server 启动中... (%d/%d)\n", attempt, max_attempts);
            }

            // 等待后重试。
            std::this_thread::sleep_for(std::chrono::milliseconds(retry_interval_ms));
        }

        LOG_ERR(" llama-server 启动超时！已尝试 %d 次，总计等待时间: %d 秒\n",
                max_attempts, max_attempts * retry_interval_ms / 1000);
        return false;
    }

    bool startLlamaServer() {
        // 如果自动启动 llama-server 服务器选项被禁用，则假设 llama-server 已经在运行。
        if (!QCopilotConfig.auto_start_base_server) {
            LOG_INF(" auto_start_base_server 已禁用，这里假设 base_server 已在运行！\n");
            return true;
        }
        // 拼接 llama-server 的命令行参数。
        std::string cmd = QCopilotConfig.base_server_path;
        cmd += " -m " + QCopilotConfig.model_path;
        cmd += " --host " + QCopilotConfig.base_server_host;
        cmd += " --port " + std::to_string(QCopilotConfig.base_server_port);
        cmd += " -c " + std::to_string(QCopilotConfig.n_ctx);
        // 禁止 llama-server 启动其自身的 Web UI。
        cmd += " --no-webui";
        cmd += " --jinja";
        if (QCopilotConfig.n_gpu_layers >= 0) {
            cmd += " -ngl " + std::to_string(QCopilotConfig.n_gpu_layers);
        }


#ifdef NDEBUG
        // Release 模式
        LOG_INF("正在启动 base-server......\n");
#else
        // Debug 模式
        LOG_INF("正在启动 base-server: %s\n", cmd.c_str());
#endif

#ifdef _WIN32
        STARTUPINFOA si = {sizeof(si)};
        if (!CreateProcessA(NULL, const_cast<char*>(cmd.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &si, &llama_process)) {
            LOG_ERR("启动 base-server 失败\n");
            return false;
        }
#else
        llama_pid = fork();
        if (llama_pid == 0) {
            // Child process
            system(cmd.c_str());
            exit(0);
        } else if (llama_pid < 0) {
            LOG_ERR("fork 进程失败，即启动 base-server 失败\n");
            return false;
        }
#endif

        // 上述代码已经在启动 llama-server ，这时候创建一个 HTTP 客户端用于健康检查，即检查 llama-server 是否启动成功。
        llama_client = std::make_unique<httplib::Client>(QCopilotConfig.base_server_host, QCopilotConfig.base_server_port);

        // 等待并检测 llama-server 启动状态
        return waitForServerStartup();
    }

    void stopLlamaServer() {
        // 如果自动启动 llama-server 服务器被禁用，则不需要停止服务器。
        if (!QCopilotConfig.auto_start_base_server) {
            return;
        }

        LOG_INF("正在停止 base-server...\n");

#ifdef _WIN32
        if (llama_process.hProcess) {
            if (TerminateProcess(llama_process.hProcess, 0)) {
                DWORD waitResult = WaitForSingleObject(llama_process.hProcess, 5000);
                if (waitResult == WAIT_TIMEOUT) {
                    LOG_WRN("base-server 进程在5秒内未响应，强制终止\n");
                }
            }
            CloseHandle(llama_process.hProcess);
            CloseHandle(llama_process.hThread);
            memset(&llama_process, 0, sizeof(llama_process));
        }
#else
        if (llama_pid > 0) {
            if (kill(llama_pid, SIGTERM) == 0) {
                int wait_count = 0;
                while (wait_count < 50 && kill(llama_pid, 0) == 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    wait_count++;
                }
                if (kill(llama_pid, 0) == 0) {
                    LOG_WRN("base-server 进程在5秒内未响应，强制终止\n");
                    kill(llama_pid, SIGKILL);
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }
            llama_pid = -1;
        }
#endif
        LOG_INF(" base-server 已停止。\n");
    }

    void setupRoutes() {
        /*
         * 步骤1: 设置预路由处理器处理CORS(跨域资源共享)请求
         * 此处理器在所有路由匹配之前执行，主要用于：
         * 1. 处理浏览器发送的CORS预检请求(OPTIONS)
         * 2. 为所有响应添加必要的CORS头部
         * 3. 允许前端网页从不同域名访问QCopilot服务
         */
        server->set_pre_routing_handler([](const httplib::Request & req, httplib::Response & res) {
            /*
             * 设置Access-Control-Allow-Origin头部允许跨域访问
             * 获取请求中的Origin头部值，并将其设置为允许的源
             * 这样可以动态允许任何发起请求的域名访问此服务
             */
            res.set_header("Access-Control-Allow-Origin", req.get_header_value("Origin"));

            /*
             * 处理浏览器CORS预检请求(OPTIONS方法)
             * 当浏览器发送跨域请求时，会先发送OPTIONS请求询问服务器
             * 是否允许跨域访问以及允许哪些方法和头部
             */
            if (req.method == "OPTIONS") {
                /*
                 * 告诉浏览器此服务允许携带认证信息的跨域请求
                 * 如cookies、Authorization头部等敏感信息
                 */
                res.set_header("Access-Control-Allow-Credentials", "true");

                /*
                 * 指定允许的HTTP方法
                 * QCopilot服务主要使用GET和POST方法
                 * GET用于健康检查、获取工具列表等
                 * POST用于聊天完成、工具执行等
                 */
                res.set_header("Access-Control-Allow-Methods", "GET, POST");

                /*
                 * 允许请求携带任意头部
                 * 通配符*表示不限制请求头部类型
                 * 这对于灵活的API调用非常重要
                 */
                res.set_header("Access-Control-Allow-Headers", "*");

                /*
                 * 为OPTIONS请求返回空内容
                 * OPTIONS请求只需要响应头部信息，不需要实际数据
                 */
                res.set_content("", "text/html");

                /*
                 * 返回Handled状态表示此请求已完全处理完成
                 * 跳过后续的路由匹配和处理逻辑
                 * 直接向浏览器返回响应
                 */
                return httplib::Server::HandlerResponse::Handled;
            }

            /*
             * 对于非OPTIONS请求，返回Unhandled状态
             * 让请求继续进入正常的路由处理流程
             * 但CORS头部已经设置完成
             */
            return httplib::Server::HandlerResponse::Unhandled;
        });

        // Health check endpoint
        server->Get("/health", [this](const httplib::Request&, httplib::Response& res) {
            auto llama_res = llama_client->Get("/health");
            if (llama_res && llama_res->status == 200) {
                res.set_content(llama_res->body, "application/json");
                res.status = llama_res->status;
            } else {
                json error_response = {{"status", "error"}, {"message", "base-server unavailable"}};
                res.set_content(error_response.dump(), "application/json");
                res.status = 503;
            }
        });

        // List available tools endpoint
        server->Get("/tools", [this](const httplib::Request&, httplib::Response& res) {
            json response = {{"tools", tool_executor->getTools()}};
            res.set_content(response.dump(), "application/json");
        });

        // chat completion/tool calling endpoint
        server->Post("/v1/chat/completions", [this](const httplib::Request& req, httplib::Response& res) {
          try {
            // 检查向该 endpoint 发起请求的请求体是否为空。
            if (req.body.empty()) {
              json err = {{"error", {{"message", "Empty request body"}}}};
              res.set_content(err.dump(), "application/json");
              res.status = 400;
              return;
            }
            // 反序列化过程即将 JSON 数据从网络读取到 main memory 中待使用。
            json request = json::parse(req.body);
            bool stream = request.value("stream", false);
            json messages = request["messages"];

            /*
            1、不论向该 endpoint 发送请求的请求体中是否存在 tools 字段，都需要将配置工具添加到 tools 字段中。
            2、在这里只能保证配置工具是符合规范的，对于请求体中的 tools 字段则是没有办法保证的，可能需要进行一些检查工作。
            3、如果向该 endpoint 发送请求的请求体中不存在 tools 字段，则将会在请求体中添加一个名为 tools 的字段，将配置工具添加到这个
            字段中；如果该 endpoint 发送请求的请求体中存在 tools 字段，则将会使用配置工具将该 tools 字段内容覆盖掉。
            4、TODO：后续会对该部分进行逻辑优化。
            */
            json all_tools = tool_executor->getTools();
            request["tools"] = all_tools;

            // 读操作：客户端在等待服务器回应时，如果 300（单位往往是秒）内没拿到回应，就触发一个读超时（read timeout）。
            llama_client->set_read_timeout(300);
            // 写操作：客户端在等待服务器回应时，如果 300（单位往往是秒）内没拿到回应，就触发一个写超时（write timeout）。
            llama_client->set_write_timeout(120);

            // no streaming mode：跑多轮，最后合并（含 reasoning）后一次性返回
            if (!stream) {
              bool go = true;
              json final_resp;
              while (go) {
                json one = request;
                one["messages"] = messages;
                // TODO：发送前对 messages 字段进行修正，可能是暂时的。
                normalize_messages_for_llama(one["messages"]);
                auto llama_res = llama_client->Post("/v1/chat/completions", one.dump(), "application/json");
                if (!llama_res || llama_res->status != 200) {
                  json err = {{"error", {{"message", "Failed to connect to base-server"}}}};
                  res.set_content(err.dump(), "application/json"); res.status = 503; return;
                }

                // SSE 还是 JSON？
                if (llama_res->get_header_value("Content-Type").find("text/event-stream") != std::string::npos
                    || llama_res->body.find("data:") != std::string::npos) {
                  auto chunks = SSEParser::extractSSEChunks(llama_res->body);
                  std::vector<json> jchunks; jchunks.reserve(chunks.size());
                  bool has_tool = false;
                  for (auto &s : chunks) {
                    if (s == "[DONE]") break;
                    auto j = SSEParser::parseSSEChunk(s);
                    if (!j.empty()) {
                      jchunks.push_back(j);
                      if (j.contains("choices") && !j["choices"].empty()) {
                        const auto &c = j["choices"][0];
                        if (c.contains("message") && c["message"].contains("tool_calls")) has_tool = true;
                        if (c.contains("delta") && c["delta"].contains("tool_calls")) has_tool = true;
                        if (c.contains("finish_reason") && c["finish_reason"] == "tool_calls") has_tool = true;
                      }
                    }
                  }
                  if (has_tool) {
                    auto merged = SSEParser::mergeToolCallChunks(jchunks);
                    if (merged.contains("choices") && !merged["choices"].empty()
                        && merged["choices"][0].contains("message")) {
                      const auto &msg = merged["choices"][0]["message"];
                      messages.push_back(msg);
                      if (msg.contains("tool_calls") && !msg["tool_calls"].empty()) {
                        executeToolCalls(tool_executor.get(), msg["tool_calls"], messages);
                        continue; // 下一轮
                      }
                    }
                  } else {
                    //  把 reasoning 也合并
                    final_resp = merge_with_reasoning(jchunks);
                    go = false;
                  }
                } else {
                  // JSON 一次性返回（可能是无流的服务器）
                  json jresp = json::parse(llama_res->body);
                  bool has_tool = false;
                  if (jresp.contains("choices") && !jresp["choices"].empty()) {
                    const auto &choice = jresp["choices"][0];
                    if (choice.contains("message") && choice["message"].contains("tool_calls")
                        && choice.value("finish_reason","") == "tool_calls") {
                      has_tool = true;
                    }
                  }
                  if (has_tool) {
                    const auto &msg = jresp["choices"][0]["message"];
                    messages.push_back(msg);
                    executeToolCalls(tool_executor.get(), msg["tool_calls"], messages);
                    continue;
                  } else {
                    final_resp = jresp; go = false;
                  }
                }
              }
              res.set_header("Content-Type", "application/json");
              res.set_content(final_resp.dump(), "application/json");
              res.status = 200;
              return;
            }
            // streaming mode：一路打通 llama-server ←→ WebUI 的 SSE
            else
            {
                /*
                 * 设置Server-Sent Events (SSE) 流式响应的标准头部
                 * Content-Type: 告诉浏览器这是一个事件流
                 * Cache-Control: 禁止浏览器缓存流式数据
                 * Connection: 保持连接活跃以持续发送数据
                 * 注意: Access-Control-Allow-Origin已由预路由处理器统一处理
                 */
                res.set_header("Content-Type", "text/event-stream");
                res.set_header("Cache-Control", "no-cache");
                res.set_header("Connection", "keep-alive");

                auto bridge = std::make_shared<SSEBridge>();

                // 生产者线程：多轮推理 + 工具调用；中间轮不发 DONE，最后一轮统一发一次
                std::thread producer([this, bridge, request, messages]() mutable {
                    try {
                        const std::string stream_id  = make_stream_id();
                        const std::string model_name = request.value("model", "");
                        json msgs = messages;

                        for (int round = 1; ; ++round) {
                            LOG_INF("开始第 %d 轮推理\n", round);

                            nlohmann::ordered_json one = request;
                            one["messages"] = msgs;
                            normalize_messages_for_llama(one["messages"]);
                            one["stream"] = true;

                            nlohmann::ordered_json merged_tool_msg;
                            bool saw_done_marker = false;
                            bool has_tool_calls = false;  // 新增变量

                            // 传递round参数
                            bool success = forward_llama_sse_once(
                                *llama_client, one, *bridge, merged_tool_msg,
                                saw_done_marker, has_tool_calls, stream_id, model_name, round);

                            if (!success) {
                                LOG_ERR("第 %d 轮推理失败\n", round);
                                bridge->push("data: [DONE]\n\n");
                                break;
                            }

                            if (!has_tool_calls) {
                                LOG_INF("第 %d 轮推理完成，无工具调用，结束会话\n", round);
                                // 发送最终的[DONE]
                                bridge->push("data: [DONE]\n\n");
                                break;
                            }

                            // 有工具调用的处理
                            if (merged_tool_msg.contains("choices") &&
                                merged_tool_msg["choices"].is_array() &&
                                !merged_tool_msg["choices"].empty() &&
                                merged_tool_msg["choices"][0].contains("message")) {

                                const auto &assist_msg = merged_tool_msg["choices"][0]["message"];

                                // 确保消息包含必要的字段
                                json normalized_msg = assist_msg;
                                if (!normalized_msg.contains("content")) {
                                    normalized_msg["content"] = "";
                                }
                                if (!normalized_msg.contains("reasoning_content")) {
                                    normalized_msg["reasoning_content"] = "";
                                }
                                msgs.push_back(normalized_msg);

                                // 在执行工具前，发送一个表示工具正在执行的消息
                                std::string tool_names_str;
                                for (size_t i = 0; i < assist_msg["tool_calls"].size(); ++i) {
                                    if (i > 0) tool_names_str += ", ";
                                    tool_names_str += assist_msg["tool_calls"][i]["function"]["name"];
                                }

                                json tool_executing_msg = {
                                    {"id", stream_id},
                                    {"object", "chat.completion.chunk"},
                                    {"model", model_name},
                                    {"choices", json::array({
                                        json{
                                            {"index", 0},
                                            {"delta", json{
                                                {"content", " QCopilot 正在调用工具[" + tool_names_str + "]执行操作 ......\n"}
                                            }},
                                            {"finish_reason", nullptr}
                                        }
                                    })}
                                };
                                bridge->push(std::string("data: ") + tool_executing_msg.dump() + "\n\n");

                                // 执行工具
                                size_t before = msgs.size();
                                executeToolCalls(tool_executor.get(), assist_msg["tool_calls"], msgs);
                                size_t added = msgs.size() - before;

                                // 发送工具执行结果的提示
                                for (size_t i = msgs.size() - added; i < msgs.size(); ++i) {
                                    if (!msgs[i].contains("role") || msgs[i]["role"] != "tool") continue;
                                    std::string tool_name = msgs[i].value("name", "unknown");

                                    // 发送工具执行完成的消息
                                    json tool_result_msg = {
                                        {"id", stream_id},
                                        {"object", "chat.completion.chunk"},
                                        {"model", model_name},
                                        {"choices", json::array({
                                            json{
                                                {"index", 0},
                                                {"delta", json{ { "content", " QCopilot 调用工具 [" + tool_name + "] 执行操作完成。\n" }
                                                }},
                                                {"finish_reason", nullptr}
                                            }
                                        })}
                                    };
                                    bridge->push(std::string("data: ") + tool_result_msg.dump() + "\n\n");

                                    // 发送工具处理结果的预览（150字符以内）
                                    std::string tool_content = msgs[i].value("content", "");
                                    std::string preview = tool_content.length() > 150 ?
                                        tool_content.substr(0, 147) + "..." : tool_content;

                                    json tool_preview_msg = {
                                        {"id", stream_id},
                                        {"object", "chat.completion.chunk"},
                                        {"model", model_name},
                                        {"choices", json::array({
                                            json{
                                                {"index", 0},
                                                {"delta", json{
                                                    {"content", " QCopilot 调用工具 [" + tool_name + "] 执行结果预览：" + preview + "\n"}
                                                }},
                                                {"finish_reason", nullptr}
                                            }
                                        })}
                                    };
                                    bridge->push(std::string("data: ") + tool_preview_msg.dump() + "\n\n");

                                    LOG_INF("工具 %s 执行完成\n", tool_name.c_str());
                                }

                                continue;
                            }

                            break;
                        }
                    } catch (const std::exception &e) {
                        LOG_ERR("生产者线程异常: %s\n", e.what());
                        nlohmann::ordered_json err = {
                            {"error", {{"message", std::string("处理过程中出错: ") + e.what()}}}
                        };
                        bridge->push(std::string("data: ") + err.dump() + "\n\n");
                        bridge->push("data: [DONE]\n\n");
                    } catch (...) {
                        LOG_ERR("生产者线程未知异常\n");
                        bridge->push("data: [DONE]\n\n");
                    }
                    bridge->close();
                });

                // 消费者：chunked provider 从 bridge 读并写到客户端
                res.set_chunked_content_provider("text/event-stream",
                  [bridge](size_t, httplib::DataSink &sink) {
                    std::string chunk;
                    while (bridge->pop(chunk)) {
                      sink.write(chunk.data(), chunk.size());
                    }
                    sink.done();
                    return true;
                  });

                // 注意：不要在这里 join，交给 httplib 的写线程生命周期
                producer.detach();
                res.status = 200;
            }
          } catch (const std::exception &e) {
            nlohmann::ordered_json error = {{"error", {{"message", e.what()}}}};
            res.set_content(error.dump(), "application/json"); res.status = 500;
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

        // 如果启动 llama-server 失败的话直接返回。
        if (!startLlamaServer()) {
            LOG_ERR("无法启动 base-server，QCopilot 启动失败\n");
            LOG_ERR("请检查：\n");
            LOG_ERR("  1. base-server 路径是否正确: %s\n", QCopilotConfig.base_server_path.c_str());
            LOG_ERR("  2. 模型文件路径是否正确: %s\n", QCopilotConfig.model_path.c_str());
            LOG_ERR("  3. 端口 %d 是否被占用\n", QCopilotConfig.base_server_port);
            LOG_ERR("  4. 系统资源是否充足（内存、GPU等）\n");
            return false;
        }

        // 设置 QCopilot 服务的 endpoints 。
        setupRoutes();

        // 启动 QCopilot 服务。
        running = true;
        server_thread = std::thread([this]() {
            LOG_INF(" QCopilot 正在监听 http://%s:%d\n", QCopilotConfig.qcopilot_host.c_str(), QCopilotConfig.qcopilot_port);
            server->listen(QCopilotConfig.qcopilot_host, QCopilotConfig.qcopilot_port);
        });

        return true;
    }

    void stop() {
        // 如果 QCopilot 服务正在运行的话，则需要停止 QCopilot 服务。
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

static std::atomic<bool> g_running{true};

static QCopilot* g_agent_instance = nullptr;

static void printHelp(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n"
              << "\n"
              << "Options:\n"
              << "  --config-file-path PATH        配置文件路径 (默认: QCopilotConfig.json)\n"
              << "  --help                         显示帮助信息\n"
              << "  --version                      显示版本信息\n"
              << "\n"
              << "Examples:\n"
              << "  " << program_name << "\n"
              << "  " << program_name << " --config-file-path /path/to/QCopilotConfig.json\n"
              << "  " << program_name << " --config-file-path \"path/to/QCopilotConfig.json\"\n"
              << std::endl;
}

static void printVersion() {
    std::cout << " QCopilot v1.0.0\n";
}

static bool parseCommandLine(int argc, char** argv, CommandLineArgs& args) {
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

static void signal_handler(int signal_num) {
    const char* signal_name = (signal_num == SIGINT) ? "SIGINT" :
                             (signal_num == SIGTERM) ? "SIGTERM" : "UNKNOWN";
    LOG_INF("收到信号 %s，正在关闭 QCopilot 服务...\n", signal_name);

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

    // 终端输出帮助信息
    if (args.show_help) {
        printHelp(argv[0]);
        return 0;
    }

    // 终端输出版本信息
    if (args.show_version) {
        printVersion();
        return 0;
    }

    // 获取配置文件路径
    std::string config_file = args.config_file_path;

    // 设置信号处理器以捕获终止信号
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 创建 QCopilot 实例
    QCopilot agent;
    // 设置全局引用以便信号处理器使用
    g_agent_instance = &agent;

    // 如果代理实例加载配置文件失败，则输出错误信息并退出程序。
    if (!agent.loadConfig(config_file)) {
        LOG_ERR("加载 QCopilot 配置失败！\n");
        g_agent_instance = nullptr;
        return 1;
    }

    // 如果代理实例启动失败，则输出错误信息并退出程序。
    if (!agent.start()) {
        LOG_ERR("启动 QCopilot 失败！\n");
        g_agent_instance = nullptr;
        return 1;
    }

    // 输出代理正在运行的信息，并提示用户按 Ctrl+C 停止。
    LOG_INF(" QCopilot 正在运行，如果想要停止运行 QCopilot 请按下 Ctrl+C 。\n");

    // 执行循环，直到收到终止信号。
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // 输出代理正在关闭的信息，并停止代理实例。
    LOG_INF("正在停止运行 QCopilot 服务......\n");
    // 停止运行 QCopilot 。
    agent.stop();

    // 清理全局引用
    g_agent_instance = nullptr;
    LOG_INF(" QCopilot 已完全停止。\n");

    return 0;
}
