#pragma once

#include "json.hpp"

#include <string>
#include <vector>
#include <chrono>
#include <mutex>
#include <utility>

using json = nlohmann::ordered_json;

#pragma region "日志系统"
/***********************************************************/
/*                       日志系统                           */
/***********************************************************/
// 日志等级枚举
enum class LogLevel {
    NONE  = 0,  // 完全禁用日志
    INFO = 1,
    WARN = 2,
    ERR = 3,    // 避免与Windows ERROR宏冲突，如果命名为 ERROR 则会与 Windows 中的 ERROR 宏冲突。
    DEBUG = 4
};

class Logger {
public:
    static void set_level(LogLevel level);
    static LogLevel get_level();
    static void set_level_from_string(const std::string& level_str);
    static void log(LogLevel level, const char* file, int line, const char* format, ...);

private:
    static LogLevel current_level;
    static std::mutex level_mutex;
    static std::string get_timestamp();
    static const char* level_to_string(LogLevel level);
    static LogLevel string_to_level(const std::string& level_str);
};

// 编译时日志控制宏：可以通过 -DQCOPILOT_DISABLE_LOGGING 完全禁用日志
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
#pragma endregion

#pragma region "通用初始化"
/***********************************************************/
/*                      通用初始化                           */
/***********************************************************/
void common_init();
#pragma endregion

#pragma region "文件通用工具"
/***********************************************************/
/*                      文件通用工具                         */
/***********************************************************/
bool file_exists(const std::string& path);
#pragma endregion

#pragma region "进程通用工具"
/***********************************************************/
/*                      进程通用工具                         */
/***********************************************************/
std::pair<bool, std::string> execute_command(const std::string& command);
bool is_process_running(int pid);
#pragma endregion

#pragma region "校验通用工具"

// 工具定义的种类：Builtin（内置，代码内置，无需可执行路径）；External（外部，需要运行绑定字段）
enum class ToolDefinitionKind {
    Builtin,
    External
};

// 校验工具名称是否有效
bool validate_tool_name(const std::string& name);

// 校验工具参数是否有效
bool validate_arguments(const json& args, const json& schema);

// 校验 function 子对象（供 Builtin/External 共用）
bool validate_tool_function_block(const json& function, std::string& error_message);

// 校验完整的工具定义 JSON
bool validate_tool_definition(
    const json& tool_definition,
    ToolDefinitionKind kind,
    std::string& error_message);
#pragma endregion
