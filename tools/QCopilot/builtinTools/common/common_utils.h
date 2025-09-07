#pragma once

#include "../../qcopilot_utils.h"
#include "tool_types.h"
#include <string>
#include <vector>
#include <chrono>

namespace BuiltinTools {
namespace Utils {

// 参数验证相关
bool validatePath(const std::string& path, std::string& error_message);
bool validateStringLength(const std::string& str, size_t max_length, const std::string& field_name, std::string& error_message);

// 错误处理相关
json createErrorResponse(const std::string& error_message);
json createSuccessResponse();

// 时间工具
std::string getCurrentTimestamp();
int64_t getCurrentTimeMs();

// 字符串工具
std::string sanitizeStringForJson(const std::string& input);
std::vector<std::string> splitString(const std::string& str, char delimiter);
std::string trimString(const std::string& str);
std::string joinStrings(const std::vector<std::string>& strings, const std::string& delimiter);

// 文件工具
bool fileExists(const std::string& path);
bool readFileContent(const std::string& path, std::string& content);
bool readTextFileWithEncodingAndRange(
    const std::string& path,
    int start_line,
    int end_line,
    std::string& content,
    int& lines_read,
    int& actual_end_line);
bool isValidUtf8File(const std::string& path);
bool writeFileContent(const std::string& path, const std::string& content);
bool appendFileContent(const std::string& path, const std::string& content);
std::vector<std::string> listDirectory(const std::string& path);

// JSON工具
json safeParseJson(const std::string& str);
std::string formatJson(const json& j);

} // namespace Utils
} // namespace BuiltinTools
