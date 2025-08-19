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
    LOG_INF(" QCopilot utilities initialized\n");
}

std::string get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

int64_t get_current_time_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

std::vector<std::string> split_string(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string join_strings(const std::vector<std::string>& strings, const std::string& delimiter) {
    if (strings.empty()) return "";
    std::stringstream ss;
    for (size_t i = 0; i < strings.size(); ++i) {
        if (i > 0) ss << delimiter;
        ss << strings[i];
    }
    return ss.str();
}

bool file_exists(const std::string& path) {
    return fs::exists(path);
}

bool read_file_content(const std::string& path, std::string& content) {
    try {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        content.resize(size);
        file.read(&content[0], size);
        file.close();

        return true;
    } catch (const std::exception& e) {
        LOG_ERR("Failed to read file %s: %s\n", path.c_str(), e.what());
        return false;
    }
}

bool write_file_content(const std::string& path, const std::string& content) {
    try {
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        file.write(content.c_str(), content.size());
        file.close();

        return true;
    } catch (const std::exception& e) {
        LOG_ERR("Failed to write file %s: %s\n", path.c_str(), e.what());
        return false;
    }
}

std::vector<std::string> list_directory(const std::string& path) {
    std::vector<std::string> files;

    try {
        if (!fs::exists(path) || !fs::is_directory(path)) {
            return files;
        }

        for (const auto& entry : fs::directory_iterator(path)) {
            files.push_back(entry.path().filename().string());
        }
    } catch (const std::exception& e) {
        LOG_ERR("Failed to list directory %s: %s\n", path.c_str(), e.what());
    }

    return files;
}

std::string execute_command(const std::string& command) {
    std::string result;

#ifdef _WIN32
    FILE* pipe = _popen(command.c_str(), "r");
#else
    FILE* pipe = popen(command.c_str(), "r");
#endif

    if (!pipe) {
        return "ERROR: Failed to execute command";
    }

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif

    return result;
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

json safe_parse_json(const std::string& str) {
    try {
        return json::parse(str);
    } catch (const std::exception& e) {
        LOG_ERR("Failed to parse JSON: %s\n", e.what());
        return json();
    }
}

std::string format_json(const json& j) {
    return j.dump(2);
}

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

// JSON Schema validation helper functions

// 基本类型验证
static bool validate_type(const json& value, const std::string& expected_type) {
    if (expected_type == "string") {
        return value.is_string();
    } else if (expected_type == "number") {
        return value.is_number();
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
            LOG_ERR("String length %zu is less than minimum %d\n", str_value.length(), min_length);
            return false;
        }
    }

    // Check maxLength
    if (schema.contains("maxLength")) {
        int max_length = schema["maxLength"].get<int>();
        if (static_cast<int>(str_value.length()) > max_length) {
            LOG_ERR("String length %zu is greater than maximum %d\n", str_value.length(), max_length);
            return false;
        }
    }

    // Check pattern
    if (schema.contains("pattern")) {
        try {
            std::string pattern = schema["pattern"].get<std::string>();
            std::regex regex_pattern(pattern);
            if (!std::regex_match(str_value, regex_pattern)) {
                LOG_ERR("String '%s' does not match pattern '%s'\n", str_value.c_str(), pattern.c_str());
                return false;
            }
        } catch (const std::exception& e) {
            LOG_ERR("Invalid regex pattern: %s\n", e.what());
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
            LOG_ERR("Number %f is less than minimum %f\n", num_value, minimum);
            return false;
        }
    }

    // Check maximum
    if (schema.contains("maximum")) {
        double maximum = schema["maximum"].get<double>();
        if (num_value > maximum) {
            LOG_ERR("Number %f is greater than maximum %f\n", num_value, maximum);
            return false;
        }
    }

    // Check exclusiveMinimum
    if (schema.contains("exclusiveMinimum")) {
        double exclusive_min = schema["exclusiveMinimum"].get<double>();
        if (num_value <= exclusive_min) {
            LOG_ERR("Number %f is not greater than exclusive minimum %f\n", num_value, exclusive_min);
            return false;
        }
    }

    // Check exclusiveMaximum
    if (schema.contains("exclusiveMaximum")) {
        double exclusive_max = schema["exclusiveMaximum"].get<double>();
        if (num_value >= exclusive_max) {
            LOG_ERR("Number %f is not less than exclusive maximum %f\n", num_value, exclusive_max);
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
                    LOG_ERR("Property '%s' failed validation\n", key.c_str());
                    return false;
                }
            }
        }
    }

    // Check additionalProperties
    if (schema.contains("additionalProperties") && schema.contains("properties")) {
        bool allow_additional = schema["additionalProperties"].get<bool>();
        if (!allow_additional) {
            const json& properties = schema["properties"];
            for (auto& [key, prop_value] : value.items()) {
                if (!properties.contains(key)) {
                    LOG_ERR("Additional property '%s' is not allowed\n", key.c_str());
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
            LOG_ERR("Missing required field: %s\n", field_name.c_str());
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
            LOG_ERR("Type mismatch: expected %s\n", expected_type.c_str());
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
            LOG_ERR("Value is not in allowed enum values\n");
            return false;
        }
    }

    // Check required properties (for objects)
    if (!validate_required_properties(value, schema)) {
        return false;
    }

    return true;
}
// 工具调用参数验证
bool validate_arguments(const json& args, const json& schema) {
    if (schema.empty()) {
        LOG_WRN("Empty schema provided for validation\n");
        return true;
    }

    return validate_json_schema(args, schema);
}
