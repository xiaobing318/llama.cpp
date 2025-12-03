#include "qcopilot-utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <filesystem>
#include <cstdlib>
#include <regex>
#include <mutex>
#include <cstdarg>
#include <cctype>
#include <set>

#ifdef _WIN32
    #include <windows.h>
    #include <tlhelp32.h>
#else
    #include <unistd.h>
    #include <signal.h>
    #include <dirent.h>
    #include <sys/stat.h>
#endif

namespace fs = std::filesystem;

#pragma region "日志系统"
/*
Notes:
1、如果配置中没有设置日志详细级别，这里默认将日志基准级别设置成 INFO 级别，日志可以通过配置文件实现调整。
2、这里将日志等级设置成 INFO 级别，意味着 INFO 及以上级别的日志都会被输出，而 DEBUG 级别的日志则会被忽略。
3、这里将日志等级设置成 INFO 级别另外一个原因是不论哪一种配置下都是可以输出内置工具相关信息。
*/
LogLevel   Logger::current_level = LogLevel::INFO;
std::mutex Logger::level_mutex;

void Logger::set_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(level_mutex);
    current_level = level;
}

LogLevel Logger::get_level() {
    std::lock_guard<std::mutex> lock(level_mutex);
    return current_level;
}

std::string Logger::get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &time_t);
#else
    localtime_r(&time_t, &tm);
#endif

    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

const char* Logger::level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERR: return "ERROR";
        case LogLevel::NONE:  return "NONE";
        default:              return "UNKNOWN";
    }
}

LogLevel Logger::string_to_level(const std::string& level_str) {
    std::string upper_str = level_str;
    std::transform(upper_str.begin(), upper_str.end(), upper_str.begin(), ::toupper);

    if (upper_str == "DEBUG") return LogLevel::DEBUG;
    if (upper_str == "INFO")  return LogLevel::INFO;
    if (upper_str == "WARN")  return LogLevel::WARN;
    if (upper_str == "ERROR") return LogLevel::ERR;
    if (upper_str == "NONE")  return LogLevel::NONE;

    // 默认返回INFO级别
    return LogLevel::INFO;
}

void Logger::set_level_from_string(const std::string& level_str) {
    set_level(string_to_level(level_str));
}

void Logger::log(LogLevel level, const char* file, int line, const char* format, ...) {
    // Thread-safe level check
    {
        std::lock_guard<std::mutex> level_lock(level_mutex);
        if (level > current_level) {
            return;
        }
    }

    // Thread-safe logging
    static std::mutex log_mutex;
    std::lock_guard<std::mutex> lock(log_mutex);

    // Extract filename from path for cleaner output
    const char* filename = strrchr(file, '/');
    if (!filename) filename = strrchr(file, '\\');  // Windows path separator
    filename = filename ? filename + 1 : file;

    // Format timestamp and header
    std::string timestamp = get_timestamp();
    FILE* output = (level == LogLevel::ERR) ? stderr : stdout;

    fprintf(output, "[%s] [%s] [%s:%d] ",
            timestamp.c_str(),
            level_to_string(level),
            filename,
            line);

    // Format and output the message
    va_list args;
    va_start(args, format);
    vfprintf(output, format, args);
    va_end(args);

    // Ensure newline at the end of each log message
    fprintf(output, "\n");
    fflush(output);
}
#pragma endregion

#pragma region "通用初始化"
void common_init() {
    // Set UTF-8 locale
    std::setlocale(LC_ALL, "");

#ifdef _WIN32
    // Enable UTF-8 console output on Windows
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    // 强制 C++ 输出流使用 UTF-8 locale
    std::setlocale(LC_ALL, ".UTF-8");
#endif

    // Log initialization
    LOG_INF("QCopilot utilities initialized");
}
#pragma endregion

#pragma region "文件通用工具"
bool file_exists(const std::string& path) {
    if (path.empty()) {
        return false;
    }

    try {
        return fs::exists(fs::u8path(path));
    } catch (const fs::filesystem_error& e) {
        LOG_ERR("Filesystem error checking if file exists %s: %s", path.c_str(), e.what());
        return false;
    }
}
#pragma endregion

#pragma region "进程通用工具"
std::pair<bool, std::string> execute_command(const std::string& command) {
    if (command.empty()) {
        LOG_ERR("Empty command provided to execute_command");
        return {false, "ERROR: Empty command"};
    }

    std::string result;

#ifdef _WIN32
    FILE* pipe = _popen(command.c_str(), "r");
#else
    FILE* pipe = popen(command.c_str(), "r");
#endif

    if (!pipe) {
        LOG_ERR("Failed to execute command: %s", command.c_str());
        return {false, "ERROR: Failed to execute command"};
    }

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

#ifdef _WIN32
    int exit_code = _pclose(pipe);
#else
    int exit_code = pclose(pipe);
#endif

    bool success = (exit_code == 0);
    if (!success) {
        LOG_WRN("Command exited with code %d: %s", exit_code, command.c_str());
    } else {
        LOG_DBG("Command executed successfully: %s", command.c_str());
    }

    return {success, result};
}

bool is_process_running(int pid) {
#ifdef _WIN32
    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (process == NULL) {
        return false;
    }

    DWORD exitCode;
    GetExitCodeProcess(process, &exitCode);
    CloseHandle(process);

    return exitCode == STILL_ACTIVE;
#else
    return kill(pid, 0) == 0;
#endif
}
#pragma endregion

#pragma region "工具定义 JSON 形状校验"
// 内部辅助函数：基本类型验证
static bool validate_type(const json& value, const std::string& expected_type) {
    if (expected_type == "string") {
        return value.is_string();
    } else if (expected_type == "number") {
        return value.is_number();
    } else if (expected_type == "integer") {
        return value.is_number_integer();
    } else if (expected_type == "boolean") {
        return value.is_boolean();
    } else if (expected_type == "object") {
        return value.is_object();
    } else if (expected_type == "array") {
        return value.is_array();
    } else if (expected_type == "null") {
        return value.is_null();
    }
    return false;
}
// 内部辅助函数：枚举值验证
static bool validate_enum(const json& value, const json& enum_values) {
    for (const auto& enum_val : enum_values) {
        if (value == enum_val) {
            return true;
        }
    }
    return false;
}
// 内部辅助函数：字符串约束检查
static bool validate_string_constraints(const json& value, const json& schema) {
    if (!value.is_string()) {
        return false;
    }

    std::string str_value = value.get<std::string>();

    // Check minLength
    if (schema.contains("minLength")) {
        int min_length = schema["minLength"].get<int>();
        if (static_cast<int>(str_value.length()) < min_length) {
            LOG_ERR("String length %zu is less than minimum %d", str_value.length(), min_length);
            return false;
        }
    }

    // Check maxLength
    if (schema.contains("maxLength")) {
        int max_length = schema["maxLength"].get<int>();
        if (static_cast<int>(str_value.length()) > max_length) {
            LOG_ERR("String length %zu is greater than maximum %d", str_value.length(), max_length);
            return false;
        }
    }

    // Check pattern
    if (schema.contains("pattern")) {
        try {
            std::string pattern = schema["pattern"].get<std::string>();
            std::regex regex_pattern(pattern);
            if (!std::regex_match(str_value, regex_pattern)) {
                LOG_ERR("String '%s' does not match pattern '%s'", str_value.c_str(), pattern.c_str());
                return false;
            }
        } catch (const std::exception& e) {
            LOG_ERR("Invalid regex pattern: %s", e.what());
            return false;
        }
    }

    return true;
}
// 内部辅助函数：数值约束检查
static bool validate_number_constraints(const json& value, const json& schema) {
    if (!value.is_number()) {
        return false;
    }

    double num_value = value.get<double>();

    // Check minimum
    if (schema.contains("minimum")) {
        double minimum = schema["minimum"].get<double>();
        if (num_value < minimum) {
            LOG_ERR("Number %f is less than minimum %f", num_value, minimum);
            return false;
        }
    }

    // Check maximum
    if (schema.contains("maximum")) {
        double maximum = schema["maximum"].get<double>();
        if (num_value > maximum) {
            LOG_ERR("Number %f is greater than maximum %f", num_value, maximum);
            return false;
        }
    }

    // Check exclusiveMinimum
    if (schema.contains("exclusiveMinimum")) {
        double exclusive_min = schema["exclusiveMinimum"].get<double>();
        if (num_value <= exclusive_min) {
            LOG_ERR("Number %f is not greater than exclusive minimum %f", num_value, exclusive_min);
            return false;
        }
    }

    // Check exclusiveMaximum
    if (schema.contains("exclusiveMaximum")) {
        double exclusive_max = schema["exclusiveMaximum"].get<double>();
        if (num_value >= exclusive_max) {
            LOG_ERR("Number %f is not less than exclusive maximum %f", num_value, exclusive_max);
            return false;
        }
    }

    return true;
}
// 内部辅助函数：对象属性验证
static bool validate_object_properties(const json& value, const json& schema) {
    if (!value.is_object()) {
        return false;
    }

    // Validate each property according to its schema
    if (schema.contains("properties")) {
        const json& properties = schema["properties"];
        for (auto& [key, prop_value] : value.items()) {
            if (properties.contains(key)) {
                // Recursively validate property
                if (!validate_arguments(prop_value, properties[key])) {
                    LOG_ERR("Property '%s' failed validation", key.c_str());
                    return false;
                }
            }
        }
    }

    // Check additionalProperties
    if (schema.contains("additionalProperties") && schema.contains("properties")) {
        const json& additional_props = schema["additionalProperties"];
        if (additional_props.is_boolean() && !additional_props.get<bool>()) {
            const json& properties = schema["properties"];
            for (auto& [key, prop_value] : value.items()) {
                if (!properties.contains(key)) {
                    LOG_ERR("Additional property '%s' is not allowed", key.c_str());
                    return false;
                }
            }
        }
    }

    return true;
}
// 内部辅助函数：必需属性检查
static bool validate_required_properties(const json& value, const json& schema) {
    if (!value.is_object() || !schema.contains("required")) {
        return true;
    }

    const json& required = schema["required"];
    for (const auto& req_field : required) {
        std::string field_name = req_field.get<std::string>();
        if (!value.contains(field_name)) {
            LOG_ERR("Missing required field: %s", field_name.c_str());
            return false;
        }
    }

    return true;
}
// 内部辅助函数：主验证逻辑
static bool validate_json_schema(const json& value, const json& schema) {
    // Check type
    if (schema.contains("type")) {
        std::string expected_type = schema["type"].get<std::string>();
        if (!validate_type(value, expected_type)) {
            LOG_ERR("Type mismatch: expected %s", expected_type.c_str());
            return false;
        }

        // Type-specific validations
        if (expected_type == "string") {
            if (!validate_string_constraints(value, schema)) {
                return false;
            }
        } else if (expected_type == "number") {
            if (!validate_number_constraints(value, schema)) {
                return false;
            }
        } else if (expected_type == "object") {
            if (!validate_object_properties(value, schema)) {
                return false;
            }
        }
    }

    // Check enum
    if (schema.contains("enum")) {
        if (!validate_enum(value, schema["enum"])) {
            LOG_ERR("Value is not in allowed enum values");
            return false;
        }
    }

    // Check required properties (for objects)
    if (!validate_required_properties(value, schema)) {
        return false;
    }

    return true;
}
// 内部辅助函数：检查字符串是否是“可解析为整数”的形式
static bool is_integer_like_string(const std::string& s) {
    if (s.empty()) return false;
    // 允许前导空白、可选的 +/-、数字、尾随空白
    size_t i = 0, n = s.size();
    while (i < n && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
    if (i < n && (s[i] == '+' || s[i] == '-')) ++i;
    size_t digits = 0;
    while (i < n && std::isdigit(static_cast<unsigned char>(s[i]))) { ++i; ++digits; }
    while (i < n && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
    return digits > 0 && i == n;
}

// 工具名称验证
bool validate_tool_name(const std::string& name) {
    if (name.empty()) return false;

    // Tool name must start with a letter (uppercase or lowercase)
    if (!std::isalpha(name[0])) {
        return false;
    }

    // Check if name contains only valid characters: letters, digits, underscores
    // No spaces, hyphens, or other special characters allowed
    return std::all_of(name.begin(), name.end(), [](char c) {
        return std::isalpha(c) || std::isdigit(c) || c == '_';
    });
}

// 工具调用参数验证
bool validate_arguments(const json& args, const json& schema) {
    if (schema.empty()) {
        LOG_WRN("Empty schema provided for validation");
        return true;
    }

    if (args.is_null()) {
        // 如果没有必需字段，空参数是合法的
        if (!schema.contains("required") || schema["required"].empty()) {
            return true;
        }
        LOG_ERR("Arguments are null but required fields are specified");
        return false;
    }

    try {
        return validate_json_schema(args, schema);
    } catch (const std::exception& e) {
        LOG_ERR("Exception during argument validation: %s", e.what());
        return false;
    }
}

// 校验 function 子对象（供 Builtin/External 共用）
bool validate_tool_function_block(const json& function, std::string& error_message) {
    /*
    校验 function 子对象（供 Builtin/External 共用）
    要求：
    - function 为对象
    - function.name：非空字符串，命名合法（字母开头，仅字母/数字/下划线）
    - function.description：非空字符串
    - function.parameters（可选）：若存在则必须是 JSON Schema 子集，且 type=object、含 properties；若含 required 必须为字符串数组
    失败时返回 false，并在 error_message 中给出可读性错误信息
    */

    // function 必须为对象
    if (!function.is_object()) {
        error_message = "'function'字段必须是一个JSON对象";
        return false;
    }

    // name：存在、非空字符串、命名合法
    if (!function.contains("name")) {
        error_message = "function定义缺失必需的'name'字段";
        return false;
    }
    if (!function["name"].is_string() || function["name"].get<std::string>().empty()) {
        error_message = "function的'name'字段必须是字符串且非空";
        return false;
    }
    if (!validate_tool_name(function["name"].get<std::string>())) {
        error_message = "工具名称格式无效: '" + function["name"].get<std::string>() + "' (必须以字母开头，只能包含字母、数字和下划线)";
        return false;
    }

    // description：存在、非空字符串
    if (!function.contains("description")) {
        error_message = "function定义缺失必需的'description'字段";
        return false;
    }
    if (!function["description"].is_string()) {
        error_message = "function的'description'字段必须是字符串";
        return false;
    }
    if (function["description"].get<std::string>().empty()) {
        error_message = "function的'description'字段不能为空";
        return false;
    }

    // parameters（可选）：如存在则必须为 object schema，且 type=object、含 properties；required 为字符串数组
    if (function.contains("parameters")) {
        const json& parameters = function["parameters"];
        if (!parameters.is_object()) {
            error_message = "function的'parameters'字段必须是一个JSON对象";
            return false;
        }
        if (!parameters.contains("type")) {
            error_message = "function的'parameters'字段缺失必需的'type'字段";
            return false;
        }
        if (!parameters["type"].is_string() || parameters["type"].get<std::string>() != "object") {
            error_message = "parameters的'type'字段必须为'object'";
            return false;
        }
        if (!parameters.contains("properties")) {
            error_message = "'parameters'字段缺失必需的'properties'字段";
            return false;
        }
        if (!parameters["properties"].is_object()) {
            error_message = "parameters的'properties'字段必须是一个JSON对象";
            return false;
        }
        if (parameters.contains("required")) {
            if (!parameters["required"].is_array()) {
                error_message = "parameters的'required'字段必须是一个数组";
                return false;
            }
            for (const auto& req : parameters["required"]) {
                if (!req.is_string()) {
                    error_message = "parameters的'required'数组中的元素必须是字符串";
                    return false;
                }
            }
        }
    }

    return true;
}

// 校验完整的工具定义 JSON
bool validate_tool_definition(
    const json& tool_definition,
    ToolDefinitionKind kind,
    std::string& error_message) {
    /*
    校验完整的工具定义 JSON：
    - 顶层必须 type="function" 且包含 function 子对象（复用 validate_tool_function_block）
    - Builtin：仅校验共用 function 结构；若出现外部字段将忽略（可在实现中记录 WARN）
    - External：除共用部分外，还需至少提供一个可执行字段（executable_* 或 executable_generic）；
      若提供 command_template 则必须为字符串；若提供 timeout_ms 则必须为整数或可解析为整数的字符串
    失败时返回 false，并在 error_message 中给出可读性错误信息
    */

    // 1. 工具定义必须是一个对象
    if (!tool_definition.is_object()) {
        error_message = "工具定义必须是一个JSON对象";
        return false;
    }
    // 2. 工具定义中必须包含 type 字段
    if (!tool_definition.contains("type")) {
        error_message = "工具定义缺失必需的'type'字段";
        return false;
    }
    // 2.1 工具定义中的 type 字段必须是字符串类型并且 type 字段中的值必须是 function 字符串
    if (!tool_definition["type"].is_string() || tool_definition["type"].get<std::string>() != "function") {
        error_message = "工具定义的'type'字段必须字符串类型且只能为'function'";
        return false;
    }
    // 3. 工具定义中必须包含 function 字段
    if (!tool_definition.contains("function")) {
        error_message = "工具定义缺失必需的'function'字段";
        return false;
    }
    {
        // 获取工具定义中的 function 字段内容
        const json& function = tool_definition["function"];
        // 验证 function 字段内容
        if (!validate_tool_function_block(function, error_message)) {
            return false;
        }
    }

    // 外部工具特有字段校验
    if (kind == ToolDefinitionKind::External) {
        //
        bool has_any_exec = false;
        auto check_exec = [&](const char* key){
            if (tool_definition.contains(key)) {
                if (!tool_definition[key].is_string()) {
                    error_message = std::string("'") + key + "'字段必须是字符串";
                    return false;
                }
                const auto& v = tool_definition[key].get<std::string>();
                if (!v.empty()) has_any_exec = true;
            }
            return true;
        };
        // 验证是否存在指定字段并且指定字段的属性值是否为字符串
        if (!check_exec("executable_generic")) return false;
        if (!check_exec("executable_windows")) return false;
        if (!check_exec("executable_linux")) return false;
        if (!check_exec("executable_macos")) return false;
        // 如果外部工具的定义中不存在可执行文件路径则报错
        if (!has_any_exec) {
            error_message = "缺少可执行文件路径: 至少提供 'executable_generic' 或某个平台专属字段";
            return false;
        }
        // 如果外部工具定义中包含命令行模版但是命令行模版不是字符串则报错
        if (tool_definition.contains("command_template") && !tool_definition["command_template"].is_string()) {
            error_message = "'command_template'字段必须是字符串";
            return false;
        }
        // 如果外部工具定义中包含超时时间，则提取其属性值
        if (tool_definition.contains("timeout_ms")) {
            const auto& tm = tool_definition["timeout_ms"];
            bool ok = tm.is_number_integer() || (tm.is_string() && is_integer_like_string(tm.get<std::string>()));
            if (!ok) {
                error_message = "'timeout_ms'必须是整数或可解析的整数字符串";
                return false;
            }
        }
    } else {
        // Builtin：若发现外部字段则给出一次性警告但不作为失败（保持宽松兼容）
        auto warn_if_present = [&](const char* key){
            if (tool_definition.contains(key)) {
                LOG_WRN("Ignoring field '%s' in builtin tool definition", key);
            }
        };
        warn_if_present("executable_generic");
        warn_if_present("executable_windows");
        warn_if_present("executable_linux");
        warn_if_present("executable_macos");
        warn_if_present("command_template");
        warn_if_present("timeout_ms");
    }

    return true;
}
#pragma endregion
