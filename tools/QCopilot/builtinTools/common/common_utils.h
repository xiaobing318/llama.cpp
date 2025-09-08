#pragma once

#include "../../qcopilot_utils.h"
#include "tool_types.h"
#include <string>
#include <vector>
#include <chrono>

namespace BuiltinTools {
namespace Utils {

/***********************************************************
* 1、匹配模式
* 2、检测文件是否为二进制文件
* 3、在目录书中匹配文件/目录
* 4、在文件中搜索
***********************************************************/
// 通用实用函数 1 ：匹配模式
bool matchPattern(
    const std::string& text,
    const std::string& pattern,
    bool case_sensitive);

// 通用实用函数 2 ：检测文件是否为二进制文件
bool isLikelyBinary(
    const std::filesystem::path& filepath,
    std::size_t probe = 4096);

// 通用实用函数 3 ：在目录树中匹配文件/目录
std::vector<std::filesystem::path> globFiles(
    const std::filesystem::path& base_dir,
    const std::string& pattern,
    bool include_directories = false,
    bool follow_symlinks = false,
    bool case_sensitive =
#ifdef _WIN32
    false
#else
    true
#endif
);

// 通用实用函数 4 ：在文件中搜索
std::vector<json> searchInFileRegex(
    const std::filesystem::path& filepath,
    const std::string& pattern,
    bool use_regex,
    bool case_sensitive,
    bool line_numbers,
    int& total_matches,
    int max_matches);
/***********************************************************
* 1、匹配模式
* 2、检测文件是否为二进制文件
* 3、在目录书中匹配文件/目录
* 4、在文件中搜索
***********************************************************/








/***********************************************************
* 1、验证路径合法性
***********************************************************/
// 通用实用函数 1 ：验证路径合法性
bool validatePath(
    const std::string& path,
    std::string& error_message);
/***********************************************************
* 1、验证路径合法性
***********************************************************/








/***********************************************************
* 1、构造错误响应JSON
* 2、构造成功响应JSON
* 3、安全的解析JSON文本
* 4、格式化JSON为字符串
***********************************************************/
// 通用实用函数 1 ：构造错误响应JSON
json createErrorResponse(const std::string& error_message);

// 通用实用函数 2 ：构造成功响应JSON
json createSuccessResponse();

// 通用实用函数 3 ：安全的解析JSON文本
json safeParseJson(const std::string& str);

// 通用实用函数 4 ：格式化JSON为字符串
std::string formatJson(const json& j);
/***********************************************************
* 1、构造错误响应JSON
* 2、构造成功响应JSON
* 3、安全的解析JSON文本
* 4、格式化JSON为字符串
***********************************************************/








/***********************************************************
* 1、返回本地时间戳字符串
* 2、返回当前时间的毫秒级时间戳
***********************************************************/
// 通用实用函数 1 ：返回本地时间戳字符串
std::string getCurrentTimestamp();

// 通用实用函数 2 ：返回当前时间的毫秒级时间戳
int64_t getCurrentTimeMs();
/***********************************************************
* 1、返回本地时间戳字符串
* 2、返回当前时间的毫秒级时间戳
***********************************************************/








/***********************************************************
* 1、清理控制字符并修复非法UTF-8以确保JSON安全
* 2、按定界符拆分字符串
* 3、去除字符串首尾空白
* 4、按定界符连接字符串
***********************************************************/
// 通用实用函数 1 ：清理控制字符并修复非法UTF-8以确保JSON安全
std::string sanitizeStringForJson(const std::string& input);

// 通用实用函数 2 ：按定界符拆分字符串
std::vector<std::string> splitString(
    const std::string& str,
    char delimiter);

// 通用实用函数 3 ：去除字符串首尾空白
std::string trimString(const std::string& str);

// 通用实用函数 4 ：按定界符连接字符串
std::string joinStrings(
    const std::vector<std::string>& strings,
    const std::string& delimiter);
/***********************************************************
* 1、清理控制字符并修复非法UTF-8以确保JSON安全
* 2、按定界符拆分字符串
* 3、去除字符串首尾空白
* 4、按定界符连接字符串
***********************************************************/








/***********************************************************
* 1、检查文件是否存在
* 2、读取整个文件内容到字符串
* 3、按指定编码和行范围读取文本文件内容
* 4、检查文件是否为有效的UTF-8文本文件
* 5、以覆盖方式写入二进制文件
* 6、以追加方式写入二进制文件
* 7、列出目录内容（不递归）
***********************************************************/
// 通用实用函数 1 ：检查文件是否存在
bool fileExists(const std::string& path);

// 通用实用函数 2 ：读取整个文件内容到字符串
bool readFileContent(
    const std::string& path,
    std::string& content);

// 通用实用函数 3 ：按指定编码和行范围读取文本文件内容
bool readTextFileWithRange(
    const std::string& path,
    int start_line,
    int end_line,
    std::string& content,
    int& lines_read,
    int& actual_end_line);

// 通用实用函数 4 ：检查文件是否为有效的UTF-8文本文件
bool isValidUtf8File(const std::string& path);

// 通用实用函数 5 ：以覆盖方式写入二进制文件
bool writeFileContent(
    const std::string& path,
    const std::string& content);

// 通用实用函数 6 ：以追加方式写入二进制文件
bool appendFileContent(
    const std::string& path,
    const std::string& content);

// 通用实用函数 7 ：列出目录内容（不递归）
std::vector<std::string> listDirectory(const std::string& path);
/***********************************************************
* 1、检查文件是否存在
* 2、读取整个文件内容到字符串
* 3、按指定编码和行范围读取文本文件内容
* 4、检查文件是否为有效的UTF-8文本文件
* 5、以覆盖方式写入二进制文件
* 6、以追加方式写入二进制文件
* 7、列出目录内容（不递归）
***********************************************************/

} // namespace Utils
} // namespace BuiltinTools
