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
#include <optional>

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

// 构建统一的工具执行结果结构
static json make_uniform_result(
    bool success,
    int exit_code,
    const std::string &stdout_text,
    const std::string &stderr_text,
    const std::string &command_line,
    const json &argv,
    const std::string &executable,
    int64_t duration_ms,
    bool timed_out,
    const std::string &llm_message,
    std::optional<long long> timeout_ms = std::nullopt,
    const std::string &error_message = std::string()) {
    json out = {
        {"success", success},
        {"exit_code", exit_code},
        {"stdout", stdout_text},
        {"stderr", stderr_text},
        {"command_line", command_line},
        {"argv", argv},
        {"executable", executable},
        {"duration_ms", duration_ms},
        {"timed_out", timed_out},
        {"llm_message", llm_message}
    };

    if (timeout_ms.has_value()) {
        out["timeout_ms"] = timeout_ms.value();
    }

    std::string error = error_message;
    if (!success && error.empty()) {
        error = !stderr_text.empty() ? stderr_text : llm_message;
    }
    if (!error.empty()) {
        out["error"] = error;
    }

    return out;
}

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
    std::vector<std::string> argv;
    // 该函数负责把命令模板拆解成最终的 argv 数组：解析占位符、注入可执行文件绝对路径，并过滤掉空参数
    if (command_template.empty()) {
        if (!executable.empty()) argv.push_back(executable);
        return argv;
    }

    auto tokens = split_template_tokens(command_template);
    if (tokens.empty()) {
        if (!executable.empty()) argv.push_back(executable);
        return argv;
    }

    const bool first_token_has_placeholder = tokens[0].text.find('{') != std::string::npos;

    auto append_from_expanded = [&](const std::vector<std::string>& expanded,
                                    bool is_first_token,
                                    bool token_has_placeholder) {
        bool first_added = !is_first_token || !argv.empty();
        for (const auto& item : expanded) {
            if (is_first_token && !first_added) {
                const bool sentinel = (item == "executableFilePath");
                const bool looks_like_name = !sentinel && !token_has_placeholder && !item.empty() && !has_path_separator(item);
                if ((sentinel || looks_like_name) && !executable.empty()) {
                    argv.push_back(executable);
                    first_added = true;
                    continue;
                }
                if (item.empty() && !executable.empty()) {
                    argv.push_back(executable);
                    first_added = true;
                    continue;
                }
            }
            if (!item.empty()) {
                argv.push_back(item);
                if (is_first_token) first_added = true;
            }
        }
        if (is_first_token && !first_added && !executable.empty()) {
            argv.push_back(executable);
        }
    };

    auto first_expanded = expand_placeholders_token(tokens[0].text, arguments, tokens[0].quoted);
    append_from_expanded(first_expanded, true, first_token_has_placeholder);

    for (size_t i = 1; i < tokens.size(); ++i) {
        auto expanded = expand_placeholders_token(tokens[i].text, arguments, tokens[i].quoted);
        append_from_expanded(expanded, false, tokens[i].text.find('{') != std::string::npos);
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

} // namespace

#pragma endregion

// 构造函数
ToolExecutor::ToolExecutor() {
    //  在构造 ToolExecutor 实体的时候自动注册内置工具。
    registerBuiltinTools();
}

// 注册内置工具
void ToolExecutor::registerBuiltinTools() {
    // 获取内置工具定义和内置工具执行函数
    auto definitions = BuiltinTools::getBuiltinToolDefinitions();
    auto functions = BuiltinTools::getBuiltinToolFunctions();

    // 注册所有内置工具
    for (const auto& definition : definitions) {
        try {
            // 对内部工具的定义进行全面检查，确保其符合预期的 JSON Schema
            std::string error_message;
            if (!validate_tool_definition(definition.definition, ToolDefinitionKind::Builtin, error_message)) {
                LOG_ERR("内置工具定义校验失败，已跳过注册，错误: %s", error_message.c_str());
                continue;
            }
            // 对内部工具的定义进行全面检查之后说明其符合预期的 JSON Schema，获取其名称
            const std::string& name = definition.name;

            // 需要在函数映射中存在并且非空
            auto fit = functions.find(name);
            if (fit == functions.end() || !fit->second) {
                LOG_WRN("跳过注册内置工具 %s：未找到对应的执行函数", name.c_str());
                continue;
            }

            // 确保多线程注册安全
            std::lock_guard<std::mutex> lock(tools_mutex);
            // 先检查是否已经注册了同名的工具，如果已经存在，则直接返回不需要进行注册。
            if (tool_functions.find(name) != tool_functions.end() || tool_definitions.find(name) != tool_definitions.end()){
                LOG_ERR("内置工具注册失败：已存在名称为 %s 的工具（重复注册）", name.c_str());
                continue;
            }
            // 注册内置工具的工具定义并标注来源
            tool_definitions[name] = definition.definition;
            tool_definitions[name]["_kind"] = "builtin";
            // 注册内置工具的执行函数
            tool_functions[name] = fit->second;
            // 输出日志
            LOG_INF("成功注册内置工具: %s - %s", name.c_str(), tool_definitions[name]["function"]["description"].get<std::string>().c_str());

        } catch (...) {
            LOG_ERR("注册内置工具时发生异常");
            continue;
        }
    }
}

// 注册外部工具
bool ToolExecutor::registerExternalTools(const json& tool_definition) {
    try{
        // 对外部工具的定义进行全面检查，确保其符合预期的 JSON Schema
        std::string error_message;
        if (!validate_tool_definition(tool_definition, ToolDefinitionKind::External, error_message)) {
            LOG_ERR("外部工具定义校验失败，已跳过注册, 错误: %s", error_message.c_str());
            return false;
        }
        // 对外部工具的定义进行全面检查之后说明其符合预期的 JSON Schema，获取其名称
        const json& function = tool_definition["function"];
        std::string name = function["name"].get<std::string>();
        // 确保多线程注册安全
        std::lock_guard<std::mutex> lock(tools_mutex);
        // 先检查是否已经注册了同名的工具，如果已经存在，则直接返回不需要进行注册。
        if (tool_functions.find(name) != tool_functions.end() || tool_definitions.find(name) != tool_definitions.end()){
            LOG_ERR("外部工具注册失败：已存在名称为 %s 的工具（重复注册）", name.c_str());
            return false;
        }
        // 注册外部工具的工具定义并标注来源
        tool_definitions[name] = tool_definition;
        tool_definitions[name]["_kind"] = "external";
        // 输出日志
        LOG_INF("成功注册外部工具: %s - %s", name.c_str(), function["description"].get<std::string>().c_str());
        return true;
    }catch (...) {
            LOG_ERR("注册外部工具时发生异常");
            return false;
    }
}

// 执行内置工具或者外部工具
json ToolExecutor::execute(const std::string& name, const json& arguments) const {
    // 首先检查是否为内置工具
    ToolFunction builtin_fn;
    bool has_builtin = false;
    {
        std::lock_guard<std::mutex> lock(tools_mutex);
        auto it_local = tool_functions.find(name);
        if (it_local != tool_functions.end()) {
            builtin_fn = it_local->second;
            has_builtin = true;
        }
    }
    // 如果是内置工具，直接调用其函数
    if (has_builtin) {
        // 记录起始时间
        auto t_start = std::chrono::steady_clock::now();
        // 执行内置工具并构建统一的输出格式，也就是说内置工具的输出格式保持一致
        auto build_builtin_envelope = [&](bool success, const std::string &error_msg) {
            auto dur_ms = (int64_t) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t_start).count();
            const std::string command = std::string("builtin:") + name;
            const std::string llm = success
                                        ? std::string("Builtin tool executed successfully.")
                                        : (error_msg.empty() ? std::string("Builtin tool failed.") : error_msg);
            const std::string stderr_text = success ? std::string("") : error_msg;
            return make_uniform_result(success,
                                       success ? 0 : 1,
                                       std::string(),
                                       stderr_text,
                                       command,
                                       json::array(),
                                       command,
                                       dur_ms,
                                       false,
                                       llm,
                                       std::nullopt,
                                       error_msg);
        };
        // 健壮性保护：避免空的可调用体，即如果被调用的内置函数没有具体的实现函数，则输出提示信息
        if (!builtin_fn) {
            LOG_ERR("内置工具 [%s] 未注册执行函数，即没有具体的实现函数。", name.c_str());
            return build_builtin_envelope(false, "The implementation of the built-in tool is not registered, that is, there is only the definition of the tool call but no implementation of the tool");
        }
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
            if (!success && !res.contains("error")) {
                if (res.contains("stderr") && res["stderr"].is_string() && !res["stderr"].get<std::string>().empty()) {
                    res["error"] = res["stderr"].get<std::string>();
                } else if (res.contains("llm_message") && res["llm_message"].is_string()) {
                    res["error"] = res["llm_message"].get<std::string>();
                } else {
                    res["error"] = "Builtin tool failed.";
                }
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
                        std::optional<long long> timeout_opt;
                        if (timeout_ms >= 0) timeout_opt = timeout_ms;
                        const std::string command_desc = std::string("tool:") + name;
                        return make_uniform_result(false,
                                                   1,
                                                   std::string(),
                                                   std::string("Invalid tool arguments (schema mismatch)"),
                                                   command_desc,
                                                   json::array(),
                                                   command_desc,
                                                   dur_ms,
                                                   false,
                                                   std::string("Invalid tool arguments (schema mismatch)"),
                                                   timeout_opt,
                                                   std::string("Invalid tool arguments (schema mismatch)"));
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
                std::optional<long long> timeout_opt;
                if (timeout_ms >= 0) timeout_opt = timeout_ms;
                const std::string command_desc = std::string("tool:") + name;
                return make_uniform_result(false,
                                           1,
                                           std::string(),
                                           std::string(e.what()),
                                           command_desc,
                                           json::array(),
                                           command_desc,
                                           0,
                                           false,
                                           std::string(e.what()),
                                           timeout_opt,
                                           std::string(e.what()));
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
    // 因为在工具定义集合里面同时包含了内置工具和外部工具，因此只需要在工具定义集合里面查询即可
    return tool_definitions.find(name) != tool_definitions.end();
}

// 获取所有注册的工具定义
json ToolExecutor::getAllToolsDefinitions() const {
    // 创建一个空的 JSON 数组用来保存获取得到的工具调用定义
    json result = json::array();
    // 使用线程锁来确保数据访问正确
    {
        std::lock_guard<std::mutex> lock(tools_mutex);
        for (const auto& kv : tool_definitions) {
            result.push_back(kv.second);
        }
    }
    // 将获取到的工具调用定义返回
    return result;
}

// 执行外部工具
json ToolExecutor::executeExternalTool(
    const std::string& executable,
    const json& arguments,
    const std::string& command_template,
    long long timeout_ms) const {
    // 使用跨平台的方式执行外部工具，并捕获其输出
    try {
        // 开始记录起始时间
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
                std::optional<long long> timeout_opt;
                if (timeout_ms >= 0) timeout_opt = timeout_ms;
                auto message = std::string("Executable not found: ") + exe_path;
                json err = make_uniform_result(
                    false,
                    127,
                    std::string(),
                    message,
                    log_cmd,
                    json(argv),
                    exe_path,
                    dur_ms,
                    false,
                    message,
                    timeout_opt,
                    message);
                LOG_ERR("Executable not found: %s (cmd=%s)", exe_path.c_str(), log_cmd.c_str());
                return err;
            }
        }
#else
        // 如果 exe_path 中包含路径分隔符，则检查文件是否存在且可执行
        if (has_path_separator(exe_path)) {
            if (access(exe_path.c_str(), X_OK) != 0) {
                auto dur_ms = (int64_t) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t_start).count();
                std::optional<long long> timeout_opt;
                if (timeout_ms >= 0) timeout_opt = timeout_ms;
                auto message = std::string("Executable not found or not executable: ") + exe_path;
                json err = make_uniform_result(
                    false,
                    127,
                    std::string(),
                    message,
                    log_cmd,
                    json(argv),
                    exe_path,
                    dur_ms,
                    false,
                    message,
                    timeout_opt,
                    message);
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
            std::optional<long long> timeout_opt;
            if (timeout_ms >= 0) timeout_opt = timeout_ms;

            json err = make_uniform_result(false,
            1,
            std::string(),
            std::string("Failed to create pipes"),
            log_cmd,
            json(argv),
            exe_path,
            dur_ms,
            false,
            std::string("Failed to create pipes"),
            timeout_opt,
            std::string("Failed to create pipes"));

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
            std::optional<long long> timeout_opt;
            if (timeout_ms >= 0) timeout_opt = timeout_ms;
            auto message = std::string("Failed to create process: error ") + std::to_string(err);

            json j = make_uniform_result(
                false,
                1,
                std::string(),
                message,
                log_cmd,
                json(argv),
                exe_path,
                dur_ms,
                false,
                message,
                timeout_opt,
                message);

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
        base["timed_out"] = timed_out;
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
            if (!result.is_object()) {
                LOG_WRN("Tool returned JSON value that is not an object. cmd=%s, stdout_len=%zu, stderr_len=%zu",
                        log_cmd.c_str(), output.size(), errout.size());
            } else {
                if (!result.contains("success") || !result["success"].is_boolean()) {
                    result["success"] = true;
                }
                if (!result.contains("exit_code") || !result["exit_code"].is_number_integer()) {
                    result["exit_code"] = 0;
                }
                result["stdout"] = output; // keep raw stdout for uniformity
                result["stderr"] = errout;
                result["command_line"] = log_cmd;
                result["argv"] = argv;
                result["executable"] = exe_path;
                result["duration_ms"] = dur_ms;
                if (timeout_ms >= 0 && (!result.contains("timeout_ms") || !result["timeout_ms"].is_number_integer())) {
                    result["timeout_ms"] = timeout_ms;
                }
                if (!result.contains("timed_out") || !result["timed_out"].is_boolean()) {
                    result["timed_out"] = false;
                }
                if (!result.contains("llm_message") || !result["llm_message"].is_string()) {
                    result["llm_message"] = std::string("Tool executed successfully in ") + std::to_string(dur_ms) + " ms.";
                }
                return result;
            }
        } catch (const std::exception& e) {
            LOG_WRN("Tool returned non-JSON output. cmd=%s, stdout_len=%zu, stderr_len=%zu, err=%s",
                    log_cmd.c_str(), output.size(), errout.size(), e.what());
        }
        base["success"] = true;
        if (!base.contains("llm_message")) {
            base["llm_message"] = "Tool returned non-JSON output; returning raw stdout/stderr.";
        }
        return base;

#else // POSIX
        int stdin_pipe[2];
        int stdout_pipe[2];
        int stderr_pipe[2];
        if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1) {
            auto dur_ms = (int64_t) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t_start).count();
            std::optional<long long> timeout_opt;
            if (timeout_ms >= 0) timeout_opt = timeout_ms;

            json err = make_uniform_result(false,
            1,
            std::string(),
            std::string("Failed to create pipes"),
            log_cmd,
            json(argv),
            exe_path,
            dur_ms,
            false,
            std::string("Failed to create pipes"),
            timeout_opt,
            std::string("Failed to create pipes"));
            LOG_ERR("Failed to create pipes (cmd=%s)", log_cmd.c_str());

            return err;
        }

        pid_t pid = fork();
        if (pid == -1) {
            close(stdin_pipe[0]); close(stdin_pipe[1]);
            close(stdout_pipe[0]); close(stdout_pipe[1]);
            close(stderr_pipe[0]); close(stderr_pipe[1]);
            auto dur_ms = (int64_t) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t_start).count();
            std::optional<long long> timeout_opt;
            if (timeout_ms >= 0) timeout_opt = timeout_ms;

            json err = make_uniform_result(
                false,
                1,
                std::string(),
                std::string("Failed to fork process"),
                log_cmd,
                json(argv),
                exe_path,
                dur_ms,
                false,
                std::string("Failed to fork process"),
                timeout_opt,
                std::string("Failed to fork process"));

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
            if (!result.is_object()) {
                LOG_WRN("Tool returned JSON value that is not an object. cmd=%s, stdout_len=%zu, stderr_len=%zu",
                        log_cmd.c_str(), output.size(), errout.size());
            } else {
                if (!result.contains("success") || !result["success"].is_boolean()) {
                    result["success"] = true;
                }
                if (!result.contains("exit_code") || !result["exit_code"].is_number_integer()) {
                    result["exit_code"] = 0;
                }
                result["stdout"] = output;
                result["stderr"] = errout;
                result["command_line"] = log_cmd;
                result["argv"] = argv;
                result["executable"] = exe_path;
                result["duration_ms"] = dur_ms;
                if (timeout_ms >= 0 && (!result.contains("timeout_ms") || !result["timeout_ms"].is_number_integer())) {
                    result["timeout_ms"] = timeout_ms;
                }
                if (!result.contains("timed_out") || !result["timed_out"].is_boolean()) {
                    result["timed_out"] = false;
                }
                if (!result.contains("llm_message") || !result["llm_message"].is_string()) {
                    result["llm_message"] = std::string("Tool executed successfully in ") + std::to_string(dur_ms) + " ms.";
                }
                return result;
            }
        } catch (const std::exception& e) {
            LOG_WRN("Tool returned non-JSON output. cmd=%s, stdout_len=%zu, stderr_len=%zu, err=%s",
                    log_cmd.c_str(), output.size(), errout.size(), e.what());
        }
        base["success"] = true;
        if (!base.contains("llm_message")) {
            base["llm_message"] = "Tool returned non-JSON output; returning raw stdout/stderr.";
        }
        return base;
#endif

    } catch (const std::exception& e) {
        LOG_ERR("External tool execution error: %s", e.what());
        std::optional<long long> timeout_opt;
        if (timeout_ms >= 0) timeout_opt = timeout_ms;
        auto message = std::string(e.what());
        return make_uniform_result(
            false,
            1,
            std::string(),
            message,
            executable,
            json::array(),
            executable,
            0,
            false,
            message,
            timeout_opt,
            message);
    }
}
