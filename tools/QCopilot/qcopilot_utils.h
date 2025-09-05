#pragma once

#include "json.hpp"

#include <string>
#include <vector>
#include <chrono>
#include <mutex>

// 符号的链接可见性已经通过 static 修饰符进行优化。
using json = nlohmann::ordered_json;

// Logging system
enum class LogLevel {
    INFO = 0,
    WARN = 1,
    ERR = 2,    // 避免与Windows ERROR宏冲突，如果命名为 ERROR 则会与 Windows 中的 ERROR 宏冲突。
    DEBUG = 3,
    NONE  = 4,  // 完全禁用日志
};

class Logger {
public:
    static void set_level(LogLevel level);
    static LogLevel get_level();
    static void set_level_from_string(const std::string& level_str);
    static void log(LogLevel level, const char* file, int line, const char* format, ...);

private:
    static LogLevel current_level_;
    static std::mutex level_mutex_;
    static std::string get_timestamp();
    static const char* level_to_string(LogLevel level);
    static LogLevel string_to_level(const std::string& level_str);
};

// 编译时日志控制宏
// 可以通过 -DQCOPILOT_DISABLE_LOGGING 完全禁用日志
#ifdef QCOPILOT_DISABLE_LOGGING
    #define LOG_DBG(...) do {} while(0)
    #define LOG_INF(...) do {} while(0)
    #define LOG_WRN(...) do {} while(0)
    #define LOG_ERR(...) do {} while(0)
#else
    // 运行时日志级别控制
    #define LOG_DBG(format, ...) Logger::log(LogLevel::DEBUG, __FILE__, __LINE__, format, ##__VA_ARGS__)
    #define LOG_INF(format, ...) Logger::log(LogLevel::INFO, __FILE__, __LINE__, format, ##__VA_ARGS__)
    #define LOG_WRN(format, ...) Logger::log(LogLevel::WARN, __FILE__, __LINE__, format, ##__VA_ARGS__)
    #define LOG_ERR(format, ...) Logger::log(LogLevel::ERR, __FILE__, __LINE__, format, ##__VA_ARGS__)
#endif

// Common initialization
void common_init();

// Time utilities
std::string get_current_timestamp();
int64_t get_current_time_ms();

// String utilities
std::string trim(const std::string& str);
std::vector<std::string> split_string(const std::string& str, char delimiter);
std::string join_strings(const std::vector<std::string>& strings, const std::string& delimiter);
std::string sanitize_string_for_json(const std::string& str);

// File utilities
bool file_exists(const std::string& path);
bool read_file_content(const std::string& path, std::string& content);
bool read_text_file_with_encoding_and_range(
    const std::string& path,
    int start_line,
    int end_line,
    std::string& content,
    int& lines_read,
    int& actual_end_line);
bool is_valid_utf8_file(const std::string& path);
bool write_file_content(const std::string& path, const std::string& content);
std::vector<std::string> list_directory(const std::string& path);

// Process utilities
std::pair<bool, std::string> execute_command(const std::string& command);
bool is_process_running(int pid);

// JSON utilities
json safe_parse_json(const std::string& str);
std::string format_json(const json& j);

// Validation utilities
bool validate_tool_name(const std::string& name);
bool validate_arguments(const json& args, const json& schema);
