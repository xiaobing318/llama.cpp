#pragma once

#include <string>
#include <vector>
#include <chrono>
#include "json.hpp"

// TODO：需要将部分函数接口使用 static 修饰，这样可以控制符号的链接可见性。
using json = nlohmann::ordered_json;

// Logging functions
#define LOG_INF(...) fprintf(stdout, "[INFO] " __VA_ARGS__)
#define LOG_WRN(...) fprintf(stdout, "[WARN] " __VA_ARGS__)
#define LOG_ERR(...) fprintf(stderr, "[ERROR] " __VA_ARGS__)
#define LOG_DBG(...) fprintf(stdout, "[DEBUG] " __VA_ARGS__)

// Common initialization
void common_init();

// Time utilities
std::string get_current_timestamp();
int64_t get_current_time_ms();

// String utilities
std::string trim(const std::string& str);
std::vector<std::string> split_string(const std::string& str, char delimiter);
std::string join_strings(const std::vector<std::string>& strings, const std::string& delimiter);

// File utilities
bool file_exists(const std::string& path);
bool read_file_content(const std::string& path, std::string& content);
bool write_file_content(const std::string& path, const std::string& content);
std::vector<std::string> list_directory(const std::string& path);

// Process utilities
std::string execute_command(const std::string& command);
bool is_process_running(int pid);

// JSON utilities
json safe_parse_json(const std::string& str);
std::string format_json(const json& j);

// Validation utilities
bool validate_tool_name(const std::string& name);
bool validate_arguments(const json& args, const json& schema);
