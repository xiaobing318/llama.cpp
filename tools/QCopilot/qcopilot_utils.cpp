#include "qcopilot_utils.h"
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


// JSON Schema validation helper functions

// 基本类型验证
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
// 枚举值验证
static bool validate_enum(const json& value, const json& enum_values) {
    for (const auto& enum_val : enum_values) {
        if (value == enum_val) {
            return true;
        }
    }
    return false;
}
// 字符串约束检查
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
// 数值约束检查
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
// 对象属性验证
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
// 必需属性检查
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
// 主验证逻辑
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
