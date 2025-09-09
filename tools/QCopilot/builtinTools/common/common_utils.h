#pragma once

#include "../../qcopilot_utils.h"
#include "tool_types.h"
#include <string>
#include <vector>
#include <chrono>
#include <fstream>
#include <filesystem>

namespace BuiltinTools {
namespace Utils {

#pragma region "匹配模式相关实用函数"
/***********************************************************
* 1、匹配模式
***********************************************************/

// 通用实用函数 1 ：匹配模式
bool matchPattern(
    const std::string& text,
    const std::string& pattern,
    bool case_sensitive);

#pragma endregion

#pragma region "路径相关实用函数"
/***********************************************************
* 1、验证路径合法性
***********************************************************/

// 通用实用函数 1 ：验证路径合法性
bool validatePath(
    const std::string& path,
    std::string& error_message);
#pragma endregion

#pragma region "跨平台路径编解码"
/***********************************************************
* 1、将 UTF-8 字符串安全转换为 std::filesystem::path
* 2、将 std::filesystem::path 安全转换为 UTF-8 字符串
***********************************************************/

// 通用实用函数 1 ：UTF-8 -> path（Windows 使用 u8path 以支持中文路径）
std::filesystem::path utf8ToPath(const std::string& s);

// 通用实用函数 2 ：path -> UTF-8（用于 JSON 输出，跨平台一致）
std::string pathToUtf8String(const std::filesystem::path& p);
#pragma endregion

#pragma region "JSON相关实用函数"
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
#pragma endregion

#pragma region "时间相关实用函数"
/***********************************************************
* 1、返回本地时间戳字符串
* 2、返回当前时间的毫秒级时间戳
* 3、将 time_point 格式化成人类可读时间
***********************************************************/

// 通用实用函数 1 ：返回本地时间戳字符串
std::string getCurrentTimestamp();

// 通用实用函数 2 ：返回当前时间的毫秒级时间戳
int64_t getCurrentTimeMs();

// 通用实用函数 3 ：将 time_point 格式化为 "%Y-%m-%d %H:%M:%S"
std::string formatTimeStamp(const std::chrono::system_clock::time_point& tp);
#pragma endregion

#pragma region "字符串操作相关实用函数"
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
#pragma endregion

#pragma region "文件操作相关实用函数"
/***********************************************************
* 1、检查文件是否存在
* 2、检查文件是否为常规可读文件
* 3、读取整个文件内容到字符串
* 4、按指定编码和行范围读取文本文件内容
* 5、检查文件是否为有效的UTF-8文本文件
* 6、检查给定字符串是否为有效的UTF-8编码
* 7、以覆盖方式写入二进制文件
* 8、以追加方式写入二进制文件
* 9、列出目录内容（不递归）
* 10、在目录书中匹配文件/目录
* 11、在文件中搜索
* 12、检测文本是否疑似二进制文件
* 13、以unicode友好方式打开指定文件，为了能够实现对中文路径的支持
***********************************************************/

// 通用实用函数 1 ：检查文件是否存在
bool fileExists(const std::string& path);
// 通用实用函数 2 : 检查文件是否为常规可读文件
bool is_regular_readable_file(
    const std::string& path,
    std::string& err);

// 通用实用函数 3 ：读取整个文件内容到字符串
bool readFileContent(
    const std::string& path,
    std::string& content);

// 通用实用函数 4 ：按指定编码和行范围读取文本文件内容
bool readTextFileWithRange(
    const std::string& path,
    int start_line,
    int end_line,
    std::string& content,
    int& lines_read,
    int& actual_end_line);

// 通用实用函数 5 ：检查文件是否为有效的UTF-8文本文件
bool isValidUtf8File(const std::string& path);

// 通用使用函数 6 : 检查给定字符串是否为有效的UTF-8编码
bool isValidUtf8String(const std::string& s);

// 通用实用函数 7 ：以覆盖方式写入二进制文件
bool writeFileContent(
    const std::string& path,
    const std::string& content);

// 通用实用函数 8 ：以追加方式写入二进制文件
bool appendFileContent(
    const std::string& path,
    const std::string& content);

// 通用实用函数 9 ：列出目录内容（不递归）
std::vector<std::string> listDirectory(const std::string& path);
// 通用实用函数 10 ：在目录树中匹配文件/目录
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
// 通用实用函数 11 ：在文件中搜索
std::vector<json> searchInFileRegex(
    const std::filesystem::path& filepath,
    const std::string& pattern,
    bool use_regex,
    bool case_sensitive,
    bool line_numbers,
    int& total_matches,
    int max_matches);

// 通用实用函数 12 ：检测文本是否疑似二进制文件
bool isLikelyBinary(
    const std::filesystem::path& filepath,
    std::size_t probe = 4096);

// 通用实用函数 12 扩展：检测字符串缓冲区是否疑似二进制（包含 NUL 字节）
bool isLikelyBinaryString(const std::string& buffer);

// 通用实用函数 13 ：以unicode友好方式打开指定文件，为了能够实现对中文路径的支持
std::ifstream open_ifstream_unicode(
    const std::string& path,
    std::ios::openmode mode);
#pragma endregion

#pragma region "格式化辅助实用函数"
/***********************************************************
* 1、将字节大小格式化为人类可读字符串（B/KB/MB/GB/TB）
***********************************************************/

std::string formatFileSize(uintmax_t size_bytes);
#pragma endregion

} // namespace Utils
} // namespace BuiltinTools
