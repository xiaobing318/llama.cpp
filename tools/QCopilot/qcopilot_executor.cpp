// 1. 本模块的头文件必须第一个包含（验证头文件自包含性）
#include "qcopilot_executor.h"

// 2. 相关项目头文件
#include "qcopilot_utils.h"
#include "qcopilot_builtin_tools.h"

// 3. C++标准库头文件（保留需要的）
#include <cctype>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>

// 4. 平台特定头文件
#ifdef _WIN32
    #include <windows.h>
    #include <shellapi.h>
#else
    // POSIX platform specific headers
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <fcntl.h>
    #include <poll.h>
    #include <signal.h>
    #include <errno.h>
#endif

#pragma region "内部辅助函数"

namespace {

// 检查字符串中是否包含路径分隔符
static inline bool has_path_separator(const std::string& s) {
    return s.find('/') != std::string::npos || s.find('\\') != std::string::npos;
}

// POSIX shell-style argument quoting
static std::string posix_quote_arg(const std::string& s) {
    if (s.find_first_of(" \t\n\v'\"$`;&|") == std::string::npos) return s;
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') out += "'\\''"; else out.push_back(c);
    }
    out += "'";
    return out;
}

// 检查 JSON 值的 truthy 状态
static inline bool is_truthy(const json &v) {
    if (v.is_null()) return false;
    if (v.is_boolean()) return v.get<bool>();
    if (v.is_number_integer()) return v.get<long long>() != 0;
    if (v.is_number_unsigned()) return v.get<unsigned long long>() != 0ULL;
    if (v.is_number_float()) return v.get<double>() != 0.0;
    if (v.is_string()) return !v.get<std::string>().empty();
    if (v.is_array()) return !v.empty();
    if (v.is_object()) return !v.empty();
    return false;
}

// URL-encode a string (for "url" modifier)
static std::string url_encode(const std::string &s) {
    static const char hex[] = "0123456789ABCDEF";
    std::string out;
    for (unsigned char c : s) {
        if ((c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c=='-' || c=='_' || c=='.' || c=='~') {
            out.push_back((char)c);
        } else {
            out.push_back('%');
            out.push_back(hex[(c >> 4) & 0xF]);
            out.push_back(hex[c & 0xF]);
        }
    }
    return out;
}

// Simple token representation preserved from template split
struct Token {
    std::string text;
    bool quoted = false; // whether the token in template was quoted
};

// Split command template into tokens respecting quotes (' and ") and backslash escapes inside quotes
static std::vector<Token> split_template_tokens(const std::string& s) {
    std::vector<Token> tokens;
    std::string cur;
    bool in_single = false, in_double = false;
    bool quoted = false;

    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (!in_single && !in_double) {
            if (std::isspace(static_cast<unsigned char>(c))) {
                if (!cur.empty()) {
                    tokens.push_back(Token{cur, quoted});
                    cur.clear();
                    quoted = false;
                }
            } else if (c == '\'' ) {
                if (cur.empty()) quoted = true;
                in_single = true;
            } else if (c == '"') {
                if (cur.empty()) quoted = true;
                in_double = true;
            } else {
                cur.push_back(c);
            }
        } else if (in_single) {
            if (c == '\'') {
                in_single = false;
            } else {
                cur.push_back(c);
            }
        } else if (in_double) {
            if (c == '"') {
                in_double = false;
            } else if (c == '\\' && i + 1 < s.size() && (s[i+1] == '"' || s[i+1] == '\\')) {
                // keep escaped quote or backslash as literal
                cur.push_back(s[i+1]);
                ++i;
            } else {
                cur.push_back(c);
            }
        }
    }
    if (!cur.empty()) tokens.push_back(Token{cur, quoted});
    return tokens;
}

// Placeholder processor inside a single token. If token is exactly a {param:join:sep} and token not quoted and sep == " ", expand to multiple tokens.
static std::vector<std::string> expand_placeholders_token(const std::string& token_in, const json& arguments, bool token_quoted) {
    std::vector<std::string> out_single; // default single token result
    std::string token = token_in;

    // Check if token is exactly one placeholder
    if (token.size() >= 2 && token.front() == '{' && token.back() == '}') {
        std::string placeholder = token.substr(1, token.size() - 2);
        size_t colon_pos = placeholder.find(':');
        std::string param_name = placeholder;
        std::string modifier;
        if (colon_pos != std::string::npos) {
            param_name = placeholder.substr(0, colon_pos);
            modifier = placeholder.substr(colon_pos + 1);
        }

        bool exists = arguments.contains(param_name);
        json val = exists ? arguments[param_name] : json();

        if (!modifier.empty() && modifier.rfind("join:", 0) == 0) {
            std::string separator = modifier.substr(5);
            if (exists && val.is_array()) {
                // If separator is a single space and token wasn't quoted, expand into multiple tokens.
                if (!token_quoted && separator == " ") {
                    std::vector<std::string> result;
                    for (const auto& item : val) {
                        if (item.is_string()) result.push_back(item.get<std::string>());
                        else result.push_back(item.dump());
                    }
                    if (!result.empty()) return result; // multi-token expansion
                } else {
                    std::string joined;
                    for (size_t i = 0; i < val.size(); ++i) {
                        std::string part = val[i].is_string() ? val[i].get<std::string>() : val[i].dump();
                        if (i > 0) joined += separator;
                        joined += part;
                    }
                    return { joined };
                }
            }
        }
    }

    // Generic replacement inside token
    std::string result;
    size_t pos = 0;
    while ((pos = token.find('{', pos)) != std::string::npos) {
        result.append(token, 0, pos);
        size_t end_pos = token.find('}', pos);
        if (end_pos == std::string::npos) {
            // unmatched brace, treat as literal rest
            result.append(token.substr(pos));
            token.clear();
            break;
        }
        std::string placeholder = token.substr(pos + 1, end_pos - pos - 1);
        size_t colon_pos = placeholder.find(':');
        std::string param_name = placeholder;
        std::string modifier;
        if (colon_pos != std::string::npos) {
            param_name = placeholder.substr(0, colon_pos);
            modifier = placeholder.substr(colon_pos + 1);
        }
        bool exists = arguments.contains(param_name);
        json val = exists ? arguments[param_name] : json();
        std::string replacement;
        if (modifier.empty()) {
            if (exists && !val.is_null()) {
                replacement = val.is_string() ? val.get<std::string>() : val.dump();
            }
        } else if (!modifier.empty() && modifier[0] == '?') {
            // truthy conditional
            if (exists && is_truthy(val)) {
                std::string text = modifier.substr(1);
                // replace {param_name} in text with value
                std::string needle = "{" + param_name + "}";
                std::string val_s = val.is_string() ? val.get<std::string>() : val.dump();
                size_t p = 0;
                while ((p = text.find(needle, p)) != std::string::npos) {
                    text.replace(p, needle.size(), val_s);
                    p += val_s.size();
                }
                replacement = text;
            }
        } else if (!modifier.empty() && modifier[0] == '!') {
            size_t q = modifier.find('?');
            if (q != std::string::npos) {
                std::string defv = modifier.substr(1, q - 1);
                std::string text = modifier.substr(q + 1);
                if (exists && !val.is_null()) {
                    std::string cur = val.is_string() ? val.get<std::string>() : val.dump();
                    if (cur != defv) replacement = text;
                }
            }
        } else if (modifier.rfind("join:", 0) == 0) {
            std::string sep = modifier.substr(5);
            if (exists && val.is_array()) {
                for (size_t i = 0; i < val.size(); ++i) {
                    std::string part = val[i].is_string() ? val[i].get<std::string>() : val[i].dump();
                    if (i > 0) replacement += sep;
                    replacement += part;
                }
            }
        } else if (modifier.rfind("or:", 0) == 0) {
            std::string defv = modifier.substr(3);
            if (!exists || !is_truthy(val)) replacement = defv;
        } else if (modifier == "upper") {
            if (exists && !val.is_null()) {
                std::string s = val.is_string() ? val.get<std::string>() : val.dump();
                std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char)std::toupper(c); });
                replacement = s;
            }
        } else if (modifier == "lower") {
            if (exists && !val.is_null()) {
                std::string s = val.is_string() ? val.get<std::string>() : val.dump();
                std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char)std::tolower(c); });
                replacement = s;
            }
        } else if (modifier == "json") {
            if (exists) replacement = val.dump();
        } else if (modifier == "url" || modifier == "urlencode") {
            if (exists && !val.is_null()) {
                std::string s = val.is_string() ? val.get<std::string>() : val.dump();
                replacement = url_encode(s);
            }
        } else if (modifier.rfind("flag:", 0) == 0) {
            std::string flag = modifier.substr(5);
            if (exists && is_truthy(val)) replacement = flag;
        }
        result += replacement;
        token.erase(0, end_pos + 1);
        pos = 0;
    }
    result += token; // append remainder
    out_single.push_back(result);
    return out_single;
}

// Build argv vector (UTF-8) from template and arguments
static std::vector<std::string> build_argv_from_template(
    const std::string& command_template,
    const json& arguments,
    const std::string& executable) {
    // 创建一个空的 argv 向量用来保存构建的命令行参数
    std::vector<std::string> argv;
    // 如果命令模板为空，则只包含可执行文件路径（如果提供了的话）
    if (command_template.empty()) {
        if (!executable.empty()) argv.push_back(executable);
        return argv;
    }
    // 解析命令模板为多个 token
    auto tokens = split_template_tokens(command_template);
    // 如果没有包含路径分隔符且提供了可执行文件路径，则将其作为第一个参数
    if (!tokens.empty()) {
        std::string cmd0 = tokens[0].text;
        if (cmd0.find('/') == std::string::npos && cmd0.find('\\') == std::string::npos && !executable.empty()) {
            argv.push_back(executable);
        } else {
            // keep as-is (may be absolute path or relative path inside template)
            argv.push_back(cmd0);
        }
        // Process remaining tokens
        for (size_t i = 1; i < tokens.size(); ++i) {
            auto expanded = expand_placeholders_token(tokens[i].text, arguments, tokens[i].quoted);
            argv.insert(argv.end(), expanded.begin(), expanded.end());
        }
    }
    return argv;
}

#ifdef _WIN32
// UTF-8 <-> UTF-16 helpers
static std::wstring utf8_to_utf16(const std::string& s) {
    if (s.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
    if (size_needed <= 0) return std::wstring();
    std::wstring ws(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &ws[0], size_needed);
    return ws;
}
static std::string utf16_to_utf8(const std::wstring& ws) {
    if (ws.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), NULL, 0, NULL, NULL);
    if (size_needed <= 0) return std::string();
    std::string s(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), &s[0], size_needed, NULL, NULL);
    return s;
}
// Build Windows command-line from argv tokens following CRT rules
static std::wstring windows_quote_arg(const std::wstring& arg) {
    if (arg.find_first_of(L" \t\"\n\v") == std::wstring::npos) {
        return arg; // no quoting needed
    }
    std::wstring result;
    result.push_back('"');
    size_t i = 0;
    while (i < arg.size()) {
        size_t num_backslashes = 0;
        while (i < arg.size() && arg[i] == L'\\') { ++i; ++num_backslashes; }
        if (i == arg.size()) {
            // Escape all backslashes at the end
            result.append(num_backslashes * 2, L'\\');
            break;
        } else if (arg[i] == L'"') {
            // Escape backslashes and the following quote
            result.append(num_backslashes * 2 + 1, L'\\');
            result.push_back(L'"');
            ++i;
        } else {
            // Keep backslashes
            result.append(num_backslashes, L'\\');
            result.push_back(arg[i]);
            ++i;
        }
    }
    result.push_back('"');
    return result;
}
static std::wstring build_windows_cmdline_w(const std::vector<std::string>& argv) {
    std::wstring cmd;
    bool first = true;
    for (const auto& a : argv) {
        if (!first) cmd.push_back(L' ');
        first = false;
        std::wstring wa = utf8_to_utf16(a);
        cmd += windows_quote_arg(wa);
    }
    return cmd;
}
#endif

} // namespace

#pragma endregion

// 构造函数
ToolExecutor::ToolExecutor() {
    //  在构造 ToolExecutor 实体的时候自动注册内置工具。
    registerBuiltinTools();
}

// 注册内置工具
void ToolExecutor::registerBuiltinTools() {
    // 获取n内置工具定义和内置工具执行函数
    auto definitions = BuiltinTools::getBuiltinToolDefinitions();
    auto functions = BuiltinTools::getBuiltinToolFunctions();

    // 注册所有内置工具
    for (const auto& definition : definitions) {
        const std::string& name = definition.name;
        try {
            // 先校验：仅校验共用的 function 子结构（Builtin 定义不要求可执行路径等外部字段）
            std::string err;
            if (!validate_tool_definition(definition.definition, ToolDefinitionKind::Builtin, err)) {
                LOG_ERR("内置工具定义校验失败，已跳过注册: %s; 错误: %s", name.c_str(), err.c_str());
                continue;
            }
            std::lock_guard<std::mutex> lock(tools_mutex);
            // 注册工具定义
            tool_definitions[name] = definition.definition;

            // 注册工具执行函数
            if (functions.find(name) != functions.end()) {
                builtinTools[name] = functions[name];
            }
        } catch (...) {
            LOG_ERR("注册内置工具时发生异常: %s", name.c_str());
            continue;
        }

        // 输出日志（尽量不抛异常）
        try {
            LOG_INF("成功注册内置工具: %s - %s",
                name.c_str(),
                tool_definitions[name]["function"]["description"].get<std::string>().c_str());
        } catch (...) {
            LOG_INF("成功注册内置工具: %s", name.c_str());
        }
    }
}

// 注册外部工具
bool ToolExecutor::registerExternalTools(const json& tool_definition) {
    // 对外部工具的定义进行全面检查，确保其符合预期的 JSON schema
    std::string error_message;
    if (!validate_tool_definition(tool_definition, ToolDefinitionKind::External, error_message)) {
        LOG_ERR("外部工具定义验证失败: %s", error_message.c_str());
        return false;
    }

    // 获取工具名称（经过验证，我们知道这些字段是存在且有效的）
    const json& function = tool_definition["function"];
    std::string name = function["name"].get<std::string>();

    // 检查是否已经注册了同名的工具，如果已经存在，则直接返回不需要进行注册。
    {
        std::lock_guard<std::mutex> lock(tools_mutex);
        if (builtinTools.find(name) != builtinTools.end() || tool_definitions.find(name) != tool_definitions.end()){
            LOG_ERR("工具注册失败：已存在名称为 %s 的工具（重复注册）", name.c_str());
            return false;
        }
    }

    // 经过上述检查后说明配置文件中的当前工具定义是有效的，将其保存到内存中的工具定义映射中。
    {
        std::lock_guard<std::mutex> lock(tools_mutex);
        tool_definitions[name] = tool_definition;
    }
    LOG_INF("成功注册外部工具: %s - %s",
        name.c_str(),
        function["description"].get<std::string>().c_str());
    return true;
}

// 执行工具
namespace {
    // 内部辅助函数：根据平台解析可执行文件路径
    static std::string resolve_executable_for_platform(const json& definition) {
        // priority: platform-specific -> generic-only（解析优先级：先解析特定于平台的可执行文件路径，如果没有再解析通用的可执行文件路径）
#ifdef _WIN32
        // 如果是 Windows 平台，优先检查 "executable_windows" 字段
        if (definition.contains("executable_windows") && definition["executable_windows"].is_string()) {
            return definition["executable_windows"].get<std::string>();
        }
#elif defined(__APPLE__)
        // 如果是 macOS 平台，优先检查 "executable_macos" 字段
        if (definition.contains("executable_macos") && definition["executable_macos"].is_string()) {
            return definition["executable_macos"].get<std::string>();
        }
#else
        // 如果是 Linux 平台，优先检查 "executable_linux" 字段
        if (definition.contains("executable_linux") && definition["executable_linux"].is_string()) {
            return definition["executable_linux"].get<std::string>();
        }
#endif
        // 如果没有指定平台则检查通用的 "executable_generic" 字段
        if (definition.contains("executable_generic") && definition["executable_generic"].is_string()) {
            return definition["executable_generic"].get<std::string>();
        }
        // 如果都没有找到合适的可执行文件路径，则返回空字符串
        return std::string();
    }

    // 内部辅助函数：提取超时时间（毫秒），如果未指定则返回 -1 表示无超时
    static int64_t extract_timeout_ms(const json& definition) {
        // 这个字段是可选的，如果存在则必须是整数或字符串，默认值为 -1 表示不设置超时
        if (definition.contains("timeout_ms")) {
            try {
                // 允许整数或字符串类型
                if (definition["timeout_ms"].is_number_integer()) {
                    return definition["timeout_ms"].get<int64_t>();
                }
                if (definition["timeout_ms"].is_string()) {
                    // allow string, best-effort parse
                    return std::stoll(definition["timeout_ms"].get<std::string>());
                }
            } catch (...) {
                // 输出提示日志
                LOG_ERR("工具定义中的 timeout_ms 字段无效，必须是整数或字符串");
            }
        }
        return -1;
    }
}

// 执行工具（内置或外部）
json ToolExecutor::execute(const std::string& name, const json& arguments) const {
    // 首先检查是否为内置工具
    ToolFunction builtin_fn;
    bool has_builtin = false;
    {
        std::lock_guard<std::mutex> lock(tools_mutex);
        auto it_local = builtinTools.find(name);
        if (it_local != builtinTools.end()) {
            builtin_fn = it_local->second;
            has_builtin = true;
        }
    }
    // 如果是内置工具，直接调用其函数
    if (has_builtin) {
        // 执行内置工具并构建统一的输出格式
        auto t_start = std::chrono::steady_clock::now();
        // 构建统一的输出封装
        auto build_builtin_envelope = [&](bool success, const std::string &error_msg) {
            auto dur_ms = (int64_t) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t_start).count();
            json out = {
                {"success", success},
                {"exit_code", success ? 0 : 1},
                {"stdout", ""},
                {"stderr", success ? std::string("") : error_msg},
                {"command_line", std::string("builtin:") + name},
                {"argv", json::array()},
                {"executable", std::string("builtin:") + name},
                {"duration_ms", dur_ms},
                {"timed_out", false},
                {"llm_message", success ? std::string("Builtin tool executed successfully.") : (error_msg.empty() ? std::string("Builtin tool failed.") : error_msg)}
            };
            return out;
        };
        // 执行内置工具时捕获异常
        try {
            // 如果定义中包含参数 schema，则进行参数验证
            json definition;
            bool have_def = false;
            {
                std::lock_guard<std::mutex> lock(tools_mutex);
                auto defIt = tool_definitions.find(name);
                if (defIt != tool_definitions.end()) {
                    definition = defIt->second;
                    have_def = true;
                }
            }
            // 如果找到了定义且包含参数 schema，则进行参数验证
            if (have_def) {
                if (definition.contains("function") && definition["function"].contains("parameters")) {
                    // 验证参数
                    if (!validate_arguments(arguments, definition["function"]["parameters"])) {
                        // 输出错误日志并返回失败结果
                        LOG_ERR("Invalid tool arguments (schema mismatch) for builtin tool %s", name.c_str());
                        return build_builtin_envelope(false, "Invalid tool arguments (schema mismatch)");
                    }
                }
            }

            // 执行内置工具函数
            json res = builtin_fn(arguments);
            // 从结果中提取 success 字段（如果没有则视为成功，因此在构建内置工具的时候必须要返回一个 success 字段用来标识内置工具执行是否成功）
            bool success = res.contains("success") ? res["success"].get<bool>() : true;
            // 获取工具执行的持续时间
            auto dur_ms = (int64_t) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t_start).count();
            res["success"] = success;
            // 如果结果中没有 exit_code、stdout、stderr 字段，则补充默认值
            if (!res.contains("exit_code"))
            {
                res["exit_code"] = success ? 0 : 1;
            }
            if (!res.contains("stdout"))
            {
                res["stdout"] = "";
            }
            if (!res.contains("stderr")) {
                // 如果有 error 字段且是字符串，则用其作为 stderr，否则设为空字符串
                if (!success && res.contains("error") && res["error"].is_string())
                {
                    res["stderr"] = res["error"].get<std::string>();
                }
                else res["stderr"] = "";
            }
            res["command_line"] = std::string("builtin:") + name;
            res["argv"] = json::array();
            res["executable"] = std::string("builtin:") + name;
            res["duration_ms"] = dur_ms;
            res["timed_out"] = false;
            if (!res.contains("llm_message")){
                res["llm_message"] = success ? "Builtin tool executed successfully." : (res.contains("error") && res["error"].is_string() ? res["error"].get<std::string>() : "Builtin tool failed.");
            }
            return res;

        } catch (const std::exception& e) {
            // 捕获异常并返回失败结果
            LOG_ERR("Tool execution failed: %s", e.what());
            return build_builtin_envelope(false, e.what());
        }
    }

    // 不是内置工具的情况下检查是否为外部工具
    json definition;
    bool have_definition = false;
    {
        std::lock_guard<std::mutex> lock(tools_mutex);
        auto it_def = tool_definitions.find(name);
        if (it_def != tool_definitions.end()) {
            definition = it_def->second;
            have_definition = true;
        }
    }
    if (have_definition) {
        // 按平台解析可执行文件路径
        std::string executable = resolve_executable_for_platform(definition);
        // 如果找到了可执行文件路径，则执行外部工具
        if (!executable.empty()) {
            // 提取命令模板和超时时间
            std::string command_template;
            if (definition.contains("command_template") && definition["command_template"].is_string()) {
                command_template = definition["command_template"].get<std::string>();
            }
            const int64_t timeout_ms = extract_timeout_ms(definition);
            // 执行外部工具时捕获异常
            try {
                // 如果定义中包含参数 schema，则进行参数验证
                if (definition.contains("function") && definition["function"].contains("parameters")) {
                    if (!validate_arguments(arguments, definition["function"]["parameters"])) {
                        LOG_ERR("Invalid tool arguments (schema mismatch) for tool %s", name.c_str());
                        auto dur_ms = (int64_t)0;
                        json out = {{"success", false}, {"error", "Invalid tool arguments (schema mismatch)"}};
                        out["llm_message"] = out["error"];
                        out["exit_code"] = 1;
                        out["stdout"] = "";
                        out["stderr"] = out["error"];
                        out["command_line"] = std::string("tool:") + name;
                        out["argv"] = json::array();
                        out["executable"] = std::string("tool:") + name;
                        out["duration_ms"] = dur_ms;
                        out["timed_out"] = false;
                        return out;
                    }
                }

                // 使用统一的外部工具执行函数
                json res = executeExternalTool(executable, arguments, command_template, timeout_ms);
                // 将超时信息向上传达（若执行层实现了超时）
                if (timeout_ms >= 0 && !res.contains("timeout_ms"))
                {
                    res["timeout_ms"] = timeout_ms;
                }
                if (!res.contains("timed_out"))
                {
                    res["timed_out"] = false;
                }
                return res;

            } catch (const std::exception& e) {
                // 捕获异常并返回失败结果
                LOG_ERR("External tool execution failed: %s", e.what());
                return json{{"success", false}, {"error", e.what()}};
            }
        }
    }

    // 工具不存在
    LOG_ERR("Tool not found: %s", name.c_str());
    json out = {
        {"success", false},
        {"error", std::string("Tool not found: ") + name},
        {"llm_message", std::string("Tool not found: ") + name},
        {"exit_code", 127},
        {"stdout", ""},
        {"stderr", std::string("Tool not found: ") + name},
        {"command_line", ""},
        {"argv", json::array()},
        {"executable", ""},
        {"duration_ms", 0},
        {"timed_out", false}
    };
    return out;
}

// 检查工具是否存在
bool ToolExecutor::hasTool(const std::string& name) const {
    std::lock_guard<std::mutex> lock(tools_mutex);
    return builtinTools.find(name) != builtinTools.end() || tool_definitions.find(name) != tool_definitions.end();
}

// 获取所有注册的工具定义
json ToolExecutor::getTools() const {
    json result = json::array();
    {
        std::lock_guard<std::mutex> lock(tools_mutex);
        for (const auto& kv : tool_definitions) {
            result.push_back(kv.second);
        }
    }
    return result;
}

json ToolExecutor::executeExternalTool(
    const std::string& executable,
    const json& arguments,
    const std::string& command_template,
    long long timeout_ms) const {
    // 使用跨平台的方式执行外部工具，并捕获其输出
    try {
        auto t_start = std::chrono::steady_clock::now();
        // 使用命令模板构建 argv 列表
        std::vector<std::string> argv = build_argv_from_template(command_template, arguments, executable);
        // 如果 argv 为空，则至少添加可执行文件路径
        if (argv.empty()) {
            // 如果没有提供命令模板，则至少添加可执行文件路径
            argv.push_back(executable);
        }

        // 构建日志用的命令行字符串（带适当的转义）
#ifdef _WIN32
        std::string log_cmd;
        for (size_t i = 0; i < argv.size(); ++i) {
            if (i) log_cmd.push_back(' ');
            // 注意 windows_quote_arg 接受的是 UTF-16 字符串
            log_cmd += utf16_to_utf8(windows_quote_arg(utf8_to_utf16(argv[i])));
        }
#else
        std::string log_cmd;
        for (size_t i = 0; i < argv.size(); ++i) {
            if (i) log_cmd.push_back(' ');
            // 适当转义参数以便日志输出
            log_cmd += posix_quote_arg(argv[i]);
        }
#endif

        // 动态检查可执行文件是否存在（如果提供了路径的话）
        const std::string& exe_path = argv[0];
#ifdef _WIN32
        // Windows 下使用宽字符 API 检查文件属性
        std::wstring wexe = utf8_to_utf16(exe_path);
        if (has_path_separator(exe_path)) {
            DWORD attrs = GetFileAttributesW(wexe.c_str());
            // 如果文件不存在或不可访问则返回错误
            if (attrs == INVALID_FILE_ATTRIBUTES) {
                auto dur_ms = (int64_t) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t_start).count();
                json err = {{"success", false}, {"error", std::string("Executable not found: ") + exe_path}, {"llm_message", std::string("Executable not found: ") + exe_path}, {"command_line", log_cmd}, {"argv", argv}, {"executable", exe_path}, {"duration_ms", dur_ms}};
                if (timeout_ms >= 0) err["timeout_ms"] = timeout_ms;
                LOG_ERR("Executable not found: %s (cmd=%s)", exe_path.c_str(), log_cmd.c_str());
                return err;
            }
        }
#else
        // 如果 exe_path 中包含路径分隔符，则检查文件是否存在且可执行
        if (has_path_separator(exe_path)) {
            if (access(exe_path.c_str(), X_OK) != 0) {
                auto dur_ms = (int64_t) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t_start).count();
                json err = {{"success", false}, {"error", std::string("Executable not found or not executable: ") + exe_path}, {"llm_message", std::string("Executable not found or not executable: ") + exe_path}, {"command_line", log_cmd}, {"argv", argv}, {"executable", exe_path}, {"duration_ms", dur_ms}};
                if (timeout_ms >= 0) err["timeout_ms"] = timeout_ms;
                LOG_ERR("Executable not found or not executable: %s (cmd=%s)", exe_path.c_str(), log_cmd.c_str());
                return err;
            }
        }
#endif

        LOG_INF("QCopilot 执行外部工具: %s", log_cmd.c_str());

        // Cross-platform execution with pipes
#ifdef _WIN32
        SECURITY_ATTRIBUTES saAttr;
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        HANDLE hChildStd_IN_Rd = NULL;
        HANDLE hChildStd_IN_Wr = NULL;
        HANDLE hChildStd_OUT_Rd = NULL;
        HANDLE hChildStd_OUT_Wr = NULL;
        HANDLE hChildStd_ERR_Rd = NULL;
        HANDLE hChildStd_ERR_Wr = NULL;

        if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0) ||
            !SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0) ||
            !CreatePipe(&hChildStd_ERR_Rd, &hChildStd_ERR_Wr, &saAttr, 0) ||
            !SetHandleInformation(hChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0) ||
            !CreatePipe(&hChildStd_IN_Rd, &hChildStd_IN_Wr, &saAttr, 0) ||
            !SetHandleInformation(hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0)) {
            auto dur_ms = (int64_t) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t_start).count();
            json err = {{"success", false}, {"error", "Failed to create pipes"}, {"llm_message", "Failed to create pipes"}, {"command_line", log_cmd}, {"argv", argv}, {"executable", exe_path}, {"duration_ms", dur_ms}};
            if (timeout_ms >= 0) err["timeout_ms"] = timeout_ms;
            LOG_ERR("Failed to create pipes (cmd=%s)", log_cmd.c_str());
            return err;
        }

        PROCESS_INFORMATION piProcInfo;
        STARTUPINFOW siStartInfo;
        ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
        ZeroMemory(&siStartInfo, sizeof(STARTUPINFOW));
        siStartInfo.cb = sizeof(STARTUPINFOW);
        siStartInfo.hStdError = hChildStd_ERR_Wr;
        siStartInfo.hStdOutput = hChildStd_OUT_Wr;
        siStartInfo.hStdInput = hChildStd_IN_Rd;
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

        // Build Unicode command line
        std::wstring cmdline = build_windows_cmdline_w(argv);

        // If exe_path has no separator, let Windows search PATH by passing NULL for applicationName
        BOOL bSuccess = CreateProcessW(
            has_path_separator(exe_path) ? wexe.c_str() : NULL,
            cmdline.empty() ? NULL : &cmdline[0],
            NULL, NULL, TRUE, 0, NULL, NULL,
            &siStartInfo, &piProcInfo);

        if (!bSuccess) {
            DWORD err = GetLastError();
            CloseHandle(hChildStd_OUT_Rd);
            CloseHandle(hChildStd_OUT_Wr);
            CloseHandle(hChildStd_ERR_Rd);
            CloseHandle(hChildStd_ERR_Wr);
            CloseHandle(hChildStd_IN_Rd);
            CloseHandle(hChildStd_IN_Wr);
            auto dur_ms = (int64_t) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t_start).count();
            json j = {{"success", false}, {"error", std::string("Failed to create process: error ") + std::to_string(err)}, {"llm_message", std::string("Failed to create process: error ") + std::to_string(err)}, {"command_line", log_cmd}, {"argv", argv}, {"executable", exe_path}, {"duration_ms", dur_ms}};
            if (timeout_ms >= 0) j["timeout_ms"] = timeout_ms;
            LOG_ERR("Failed to create process (err=%lu) cmd=%s", err, log_cmd.c_str());
            return j;
        }

        CloseHandle(hChildStd_OUT_Wr);
        CloseHandle(hChildStd_ERR_Wr);
        CloseHandle(hChildStd_IN_Rd);

        if (command_template.empty()) {
            std::string json_input = arguments.dump();
            DWORD dwWritten = 0;
            const char* data = json_input.c_str();
            size_t remaining = json_input.size();
            while (remaining > 0) {
                DWORD chunk = 0;
                if (!WriteFile(hChildStd_IN_Wr, data, (DWORD)remaining, &chunk, NULL)) break;
                if (chunk == 0) break;
                remaining -= chunk;
                data += chunk;
            }
        }
        CloseHandle(hChildStd_IN_Wr);

        std::string output_bytes;
        std::string error_bytes;
        DWORD exitCode = 0;
        bool timed_out = false;

        // Timed wait + nonblocking drain using PeekNamedPipe
        const DWORD kSleepMs = 10;
        ULONGLONG start = GetTickCount64();
        ULONGLONG deadline = (timeout_ms >= 0) ? (start + (ULONGLONG)timeout_ms) : 0ULL;

        // Poll for process completion and drain output
        for (;;) {
            // drain available bytes
            DWORD bytesAvail = 0;
            if (PeekNamedPipe(hChildStd_OUT_Rd, NULL, 0, NULL, &bytesAvail, NULL) && bytesAvail > 0) {
                CHAR chBuf[4096];
                DWORD toRead = (DWORD)std::min<DWORD>(bytesAvail, sizeof(chBuf));
                DWORD dwRead = 0;
                if (ReadFile(hChildStd_OUT_Rd, chBuf, toRead, &dwRead, NULL) && dwRead > 0) {
                    output_bytes.append(chBuf, chBuf + dwRead);
                }
            }
            if (PeekNamedPipe(hChildStd_ERR_Rd, NULL, 0, NULL, &bytesAvail, NULL) && bytesAvail > 0) {
                CHAR chBuf[4096];
                DWORD toRead = (DWORD)std::min<DWORD>(bytesAvail, sizeof(chBuf));
                DWORD dwRead = 0;
                if (ReadFile(hChildStd_ERR_Rd, chBuf, toRead, &dwRead, NULL) && dwRead > 0) {
                    error_bytes.append(chBuf, chBuf + dwRead);
                }
            }
            // check process state
            DWORD w = WaitForSingleObject(piProcInfo.hProcess, kSleepMs);
            if (w == WAIT_OBJECT_0) break; // finished
            if (w == WAIT_FAILED) break;
            if (timeout_ms >= 0 && GetTickCount64() >= deadline) {
                // timeout -> terminate
                TerminateProcess(piProcInfo.hProcess, 124);
                timed_out = true;
                break;
            }
        }
        // Read any remaining data until pipe EOF
        for (;;) {
            CHAR chBuf[4096];
            DWORD dwRead = 0;
            BOOL ok = ReadFile(hChildStd_OUT_Rd, chBuf, (DWORD)sizeof(chBuf), &dwRead, NULL);
            if (!ok || dwRead == 0) break;
            output_bytes.append(chBuf, chBuf + dwRead);
        }
        for (;;) {
            CHAR chBuf[4096];
            DWORD dwRead = 0;
            BOOL ok = ReadFile(hChildStd_ERR_Rd, chBuf, (DWORD)sizeof(chBuf), &dwRead, NULL);
            if (!ok || dwRead == 0) break;
            error_bytes.append(chBuf, chBuf + dwRead);
        }

        GetExitCodeProcess(piProcInfo.hProcess, &exitCode);

        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);
        CloseHandle(hChildStd_OUT_Rd);
        CloseHandle(hChildStd_ERR_Rd);

        // Try decode output as UTF-8, fallback to ACP
        std::string output;
        if (!output_bytes.empty()) {
            int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, output_bytes.c_str(), (int)output_bytes.size(), NULL, 0);
            if (wlen > 0) {
                std::wstring wtmp(wlen, 0);
                MultiByteToWideChar(CP_UTF8, 0, output_bytes.c_str(), (int)output_bytes.size(), &wtmp[0], wlen);
                output = utf16_to_utf8(wtmp);
            } else {
                int wlen_acp = MultiByteToWideChar(CP_ACP, 0, output_bytes.c_str(), (int)output_bytes.size(), NULL, 0);
                if (wlen_acp > 0) {
                    std::wstring wtmp(wlen_acp, 0);
                    MultiByteToWideChar(CP_ACP, 0, output_bytes.c_str(), (int)output_bytes.size(), &wtmp[0], wlen_acp);
                    output = utf16_to_utf8(wtmp);
                } else {
                    output = output_bytes; // fallback raw
                }
            }
        }
        std::string errout;
        if (!error_bytes.empty()) {
            int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, error_bytes.c_str(), (int)error_bytes.size(), NULL, 0);
            if (wlen > 0) {
                std::wstring wtmp(wlen, 0);
                MultiByteToWideChar(CP_UTF8, 0, error_bytes.c_str(), (int)error_bytes.size(), &wtmp[0], wlen);
                errout = utf16_to_utf8(wtmp);
            } else {
                int wlen_acp = MultiByteToWideChar(CP_ACP, 0, error_bytes.c_str(), (int)error_bytes.size(), NULL, 0);
                if (wlen_acp > 0) {
                    std::wstring wtmp(wlen_acp, 0);
                    MultiByteToWideChar(CP_ACP, 0, error_bytes.c_str(), (int)error_bytes.size(), &wtmp[0], wlen_acp);
                    errout = utf16_to_utf8(wtmp);
                } else {
                    errout = error_bytes; // fallback raw
                }
            }
        }

        auto dur_ms = (int64_t) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t_start).count();
        json base = {
            {"exit_code", (int)exitCode},
            {"stdout", output},
            {"stderr", errout},
            {"command_line", log_cmd},
            {"argv", argv},
            {"executable", exe_path},
            {"duration_ms", dur_ms}
        };
        if (timeout_ms >= 0) base["timeout_ms"] = timeout_ms;
        if (timed_out) base["timed_out"] = true;
        if (exitCode != 0) {
            base["success"] = false;
            base["error"] = timed_out ? std::string("Timed out") : std::string("External tool exited with code ") + std::to_string(exitCode);
            base["llm_message"] = base["error"].get<std::string>();
            LOG_ERR("External tool failed (code=%d, timed_out=%s). cmd=%s, stderr=%s",
                    (int)exitCode, timed_out ? "true" : "false", log_cmd.c_str(), errout.c_str());
            return base;
        }
        try {
            json result = json::parse(output);
            result["success"] = true;
            result["exit_code"] = 0;
            result["stdout"] = output; // keep raw stdout for uniformity
            result["stderr"] = errout;
            result["command_line"] = log_cmd;
            result["argv"] = argv;
            result["executable"] = exe_path;
            result["duration_ms"] = dur_ms;
            if (timeout_ms >= 0) result["timeout_ms"] = timeout_ms;
            result["timed_out"] = false;
            result["llm_message"] = std::string("Tool executed successfully in ") + std::to_string(dur_ms) + " ms.";
            return result;
        } catch (...) {
            base["success"] = true;
            base["llm_message"] = "Tool returned non-JSON output; returning raw stdout/stderr.";
            LOG_WRN("Tool returned non-JSON output. cmd=%s, stdout_len=%zu, stderr_len=%zu",
                    log_cmd.c_str(), base["stdout"].get<std::string>().size(), base["stderr"].get<std::string>().size());
            return base;
        }

#else // POSIX
        int stdin_pipe[2];
        int stdout_pipe[2];
        int stderr_pipe[2];
        if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1) {
            auto dur_ms = (int64_t) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t_start).count();
            json err = {{"success", false}, {"error", "Failed to create pipes"}, {"llm_message", "Failed to create pipes"}, {"command_line", log_cmd}, {"argv", argv}, {"executable", exe_path}, {"duration_ms", dur_ms}};
            if (timeout_ms >= 0) err["timeout_ms"] = timeout_ms;
            LOG_ERR("Failed to create pipes (cmd=%s)", log_cmd.c_str());
            return err;
        }

        pid_t pid = fork();
        if (pid == -1) {
            close(stdin_pipe[0]); close(stdin_pipe[1]);
            close(stdout_pipe[0]); close(stdout_pipe[1]);
            close(stderr_pipe[0]); close(stderr_pipe[1]);
            auto dur_ms = (int64_t) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t_start).count();
            json err = {{"success", false}, {"error", "Failed to fork process"}, {"llm_message", "Failed to fork process"}, {"command_line", log_cmd}, {"argv", argv}, {"executable", exe_path}, {"duration_ms", dur_ms}};
            if (timeout_ms >= 0) err["timeout_ms"] = timeout_ms;
            LOG_ERR("Failed to fork process (cmd=%s)", log_cmd.c_str());
            return err;
        }

        if (pid == 0) {
            // Child
            close(stdin_pipe[1]);
            close(stdout_pipe[0]);
            close(stderr_pipe[0]);
            dup2(stdin_pipe[0], STDIN_FILENO);
            dup2(stdout_pipe[1], STDOUT_FILENO);
            dup2(stderr_pipe[1], STDERR_FILENO);
            close(stdin_pipe[0]);
            close(stdout_pipe[1]);
            close(stderr_pipe[1]);

            // Build argv for execvp
            std::vector<char*> c_argv;
            c_argv.reserve(argv.size() + 1);
            for (auto& s : argv) c_argv.push_back(const_cast<char*>(s.c_str()));
            c_argv.push_back(nullptr);
            execvp(c_argv[0], c_argv.data());
            _exit(127);
        }

        // Parent
        close(stdin_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);
        // set non-blocking for stdout/stderr pipe
        int flags = fcntl(stdout_pipe[0], F_GETFL, 0);
        if (flags != -1) fcntl(stdout_pipe[0], F_SETFL, flags | O_NONBLOCK);
        flags = fcntl(stderr_pipe[0], F_GETFL, 0);
        if (flags != -1) fcntl(stderr_pipe[0], F_SETFL, flags | O_NONBLOCK);

        // write stdin fully
        if (command_template.empty()) {
            std::string json_input = arguments.dump();
            const char* p = json_input.c_str();
            size_t n = json_input.size();
            while (n > 0) {
                ssize_t w = write(stdin_pipe[1], p, n);
                if (w > 0) { p += w; n -= (size_t)w; continue; }
                if (w == -1 && (errno == EINTR)) continue;
                break; // EPIPE or other errors -> stop writing
            }
        }
        close(stdin_pipe[1]);

        std::string output;
        std::string errout;
        int exit_code = 0;

        const int poll_timeout_ms = 50; // responsive polling
        auto start_tp = std::chrono::steady_clock::now();
        auto deadline_tp = (timeout_ms >= 0)
                                ? start_tp + std::chrono::milliseconds(timeout_ms)
                                : std::chrono::steady_clock::time_point::max();
        bool child_alive = true;
        bool timed_out = false;
        for (;;) {
            // poll for data readability
            struct pollfd pfds[2];
            pfds[0].fd = stdout_pipe[0];
            pfds[0].events = POLLIN;
            pfds[1].fd = stderr_pipe[0];
            pfds[1].events = POLLIN;
            int r = poll(pfds, 2, poll_timeout_ms);
            if (r > 0 && (pfds[0].revents & POLLIN)) {
                char buffer[4096];
                for (;;) {
                    ssize_t bytes_read = read(stdout_pipe[0], buffer, sizeof(buffer));
                    if (bytes_read > 0) output.append(buffer, buffer + bytes_read);
                    else if (bytes_read == -1 && errno == EAGAIN) break;
                    else break; // 0 or error -> likely EOF or closed
                }
            }
            if (r > 0 && (pfds[1].revents & POLLIN)) {
                char buffer[4096];
                for (;;) {
                    ssize_t bytes_read = read(stderr_pipe[0], buffer, sizeof(buffer));
                    if (bytes_read > 0) errout.append(buffer, buffer + bytes_read);
                    else if (bytes_read == -1 && errno == EAGAIN) break;
                    else break;
                }
            }

            if (child_alive) {
                int status = 0;
                pid_t w = waitpid(pid, &status, WNOHANG);
                if (w == pid) {
                    if (WIFEXITED(status)) exit_code = WEXITSTATUS(status);
                    else if (WIFSIGNALED(status)) exit_code = 128 + WTERMSIG(status);
                    else exit_code = -1;
                    child_alive = false;
                }
            }

            if (child_alive && timeout_ms >= 0 && std::chrono::steady_clock::now() >= deadline_tp) {
                // timeout -> kill child
                kill(pid, SIGKILL);
                int status = 0;
                waitpid(pid, &status, 0);
                if (WIFEXITED(status)) exit_code = WEXITSTATUS(status);
                else if (WIFSIGNALED(status)) exit_code = 128 + WTERMSIG(status);
                else exit_code = -1;
                child_alive = false;
                timed_out = true;
            }

            if (!child_alive) break;
        }

        // drain remaining after child exit
        for (;;) {
            char buffer[4096];
            ssize_t bytes_read = read(stdout_pipe[0], buffer, sizeof(buffer));
            if (bytes_read > 0) output.append(buffer, buffer + bytes_read);
            else break;
        }
        for (;;) {
            char buffer[4096];
            ssize_t bytes_read = read(stderr_pipe[0], buffer, sizeof(buffer));
            if (bytes_read > 0) errout.append(buffer, buffer + bytes_read);
            else break;
        }
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);

        auto dur_ms = (int64_t) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t_start).count();
        json base = {{"exit_code", exit_code}, {"stdout", output}, {"stderr", errout}, {"command_line", log_cmd}, {"argv", argv}, {"executable", exe_path}, {"duration_ms", dur_ms}};
        if (timeout_ms >= 0) base["timeout_ms"] = timeout_ms;
        if (timed_out) base["timed_out"] = true;
        if (exit_code != 0) {
            base["success"] = false;
            base["error"] = timed_out ? std::string("Timed out") : std::string("External tool exited with code ") + std::to_string(exit_code);
            base["llm_message"] = base["error"].get<std::string>();
            LOG_ERR("External tool failed (code=%d, timed_out=%s). cmd=%s, stderr=%s",
                    exit_code, timed_out ? "true" : "false", log_cmd.c_str(), errout.c_str());
            return base;
        }
        try {
            json result = json::parse(output);
            result["success"] = true;
            result["exit_code"] = 0;
            result["stdout"] = output;
            result["stderr"] = errout;
            result["command_line"] = log_cmd;
            result["argv"] = argv;
            result["executable"] = exe_path;
            result["duration_ms"] = dur_ms;
            if (timeout_ms >= 0) result["timeout_ms"] = timeout_ms;
            result["timed_out"] = false;
            result["llm_message"] = std::string("Tool executed successfully in ") + std::to_string(dur_ms) + " ms.";
            return result;
        } catch (...) {
            base["success"] = true;
            base["llm_message"] = "Tool returned non-JSON output; returning raw stdout/stderr.";
            LOG_WRN("Tool returned non-JSON output. cmd=%s, stdout_len=%zu, stderr_len=%zu",
                    log_cmd.c_str(), base["stdout"].get<std::string>().size(), base["stderr"].get<std::string>().size());
            return base;
        }
#endif

    } catch (const std::exception& e) {
        LOG_ERR("External tool execution error: %s", e.what());
        return json{{"success", false}, {"error", e.what()}};
    }
}

// 辅助函数：安全地转义命令行参数
static std::string escapeShellArgument(const std::string& arg) {
    // 如果参数包含空白或特殊字符，需要用双引号包围，并转义内部的双引号
    auto is_special = [](char c) {
        switch (c) {
            case ' ': case '\t': case '\n': case '\v':
            case '"': case '\'': case '$': case '`':
            case ';': case '&': case '|':
                return true;
            default:
                return false;
        }
    };
    bool need_quote = std::any_of(arg.begin(), arg.end(), is_special);
    if (!need_quote) return arg;

    std::string escaped;
    escaped.reserve(arg.size() + 2);
    escaped.push_back('"');
    for (char c : arg) {
        if (c == '"') escaped += "\\\""; else escaped.push_back(c);
    }
    escaped.push_back('"');
    return escaped;
}

/*
一、根据命令模板和参数构建完整的命令行（仅用于日志展示；实际执行按 argv 构建）
    @param command_template 命令模板字符串
    @param arguments JSON格式的参数对象
    @param executable 可执行文件路径，用于替换模板开头的硬编码可执行文件名
    @return 构建好的完整命令行字符串

二、支持的模板语法（在 token 维度展开；引用括起来的 token 不会被拆成多个 argv）：
    1. 基本替换：{param}
       - 直接将参数值替换到占位符位置。

    2. 条件替换（真值判断）：{param:?text}
       - 当参数“为真”时替换为 text，否则为空。
       - 真值规则：true、非零数字、非空字符串、非空数组/对象。
       - text 中可以包含同名占位符再次替换，如 {encoding:?-lco ENCODING={encoding}}。

    3. 不等于默认值：{param:!default?text}
       - 当参数值不等于 default 时替换为 text。

    4. 默认值回退：{param:or:default}
       - 当参数缺失或不为真时，用 default 替换。

    5. 数组连接：{param:join:sep}
       - 将数组参数用 sep 连接为一个 token；
       - 若该 token 未被引号包围且 sep 为单个空格，则会展开为多个 argv token。

    6. 旗标输出：{param:flag:--name}
       - 当参数为真时，替换为给定 flag（例如 --name）。

    7. 转换：{param:upper} / {param:lower} / {param:json} / {param:url|urlencode}
       - 分别输出大写/小写/JSON 序列化/URL 编码形式。
 */
std::string ToolExecutor::buildCommandFromTemplate(const std::string& command_template, const json& arguments, const std::string& executable) {
    // Build via argv and stringify with platform quoting for logging/compat only
    auto argv = build_argv_from_template(command_template, arguments, executable);
    std::string out;
    bool first = true;
    for (const auto& a : argv) {
        if (!first) out.push_back(' ');
        first = false;
#ifdef _WIN32
        out += utf16_to_utf8(windows_quote_arg(utf8_to_utf16(a)));
#else
        out += posix_quote_arg(a);
#endif
    }
    return out;
}
