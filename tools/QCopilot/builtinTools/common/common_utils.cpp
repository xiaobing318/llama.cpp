#include "common_utils.h"
#include "common_utils_internal.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <cstdlib>
#include <regex>
#include <cstdarg>
#include <set>
#include <stack>
#include <system_error>

namespace BuiltinTools {
namespace Utils {

namespace fs = std::filesystem;

#pragma region "内部辅助函数"
// 已迁移到 common_utils_internal.{h,cpp}
#pragma endregion

#pragma region "路径相关实用函数"
// 判断给定字符串路径是否像URL
static inline bool looks_like_url(const std::string& s) {
    // RFC3986 scheme: ALPHA *( ALPHA / DIGIT / "+" / "-" / "." ) ":" 简化检测：以 "scheme://" 形式出现的，认为是 URL（拒绝）
    static const std::regex kUrlRe(R"(^[A-Za-z][A-Za-z0-9+\-.]*://)");
    return std::regex_search(s, kUrlRe);
}

// 判断给定字符串路径是否包含不安全字符（\0）与控制符
static inline bool contains_nul_or_control(const std::string& s) {
    for (unsigned char ch : s) {
        if (ch == 0 || (ch < 0x20 && ch != '\t' && ch != '\n' && ch != '\r')) {
            return true;
        }
    }
    return false;
}

#ifdef _WIN32
    // 检查路径组件是否为保留设备名
    static inline bool is_windows_reserved_device(const std::wstring& name) {
        if (name.empty()) return false;
    
        auto to_upper = [](wchar_t c){ return (wchar_t)std::toupper(c); };
        std::wstring u;
        u.reserve(name.size());
        for (auto c : name) u.push_back(to_upper(c));
    
        // 去掉尾部的空格与点（Windows 组件结尾不允许）
        while (!u.empty() && (u.back() == L' ' || u.back() == L'.')) u.pop_back();
        if (u.empty()) return false;
    
        auto starts_with = [&](const std::wstring& p){
            return u.size() >= p.size() && std::equal(p.begin(), p.end(), u.begin());
        };
    
        static const std::wstring base_devs[] = {
            L"CON", L"PRN", L"AUX", L"NUL"
        };
        for (const auto& d : base_devs) {
            if (starts_with(d) && (u.size() == d.size() || u[d.size()] == L'.')) return true;
        }
    
        // COM1..COM9, LPT1..LPT9
        if (u.size() >= 4) {
            if ((u.rfind(L"COM", 0) == 0 || u.rfind(L"LPT", 0) == 0) &&
                u[3] >= L'1' && u[3] <= L'9' &&
                (u.size() == 4 || u[4] == L'.')) {
                return true;
            }
        }
    
        return false;
    }

    // 检查是否包含非法字符或非法结尾（每个组件）
    static inline bool windows_component_invalid(const std::wstring& comp) {
        if (comp.empty()) return false; // 空组件（如根分隔）由外层忽略
    
        // 组件不能以空格或句点结尾
        if (comp.back() == L' ' || comp.back() == L'.') return true;
    
        for (wchar_t wc : comp) {
            if (wc < 0x20) return true;
            switch (wc) {
                case L'<': case L'>': case L':': case L'"':
                case L'|': case L'?': case L'*':
                    return true;
                default: break;
            }
        }
    
        return is_windows_reserved_device(comp);
    }
#endif // _WIN32

/**
* @brief 函数功能：校验文件系统路径字符串是否合法（本地路径，不允许 URL）
*
* 功能概述：
*  - 检查路径是否为空。
*  - 检查路径长度是否超过 4096 个 UTF-8 字节。
*  - 检查是否为 URL（拒绝 schema:// 前缀）。
*  - 检查是否包含 NUL 或不可见控制字符。
*  - 将字符串解析为 `std::filesystem::path`（通过 `fs::u8path`）并进行规范化 `lexically_normal()`。
*  - 若规范化结果仍包含 `..` 组件，判定为路径穿越（path traversal）风险。
*  - 检查路径是否存在（文件或目录），若不存在返回错误。
*  - Windows 平台：逐组件检查非法字符与保留设备名。
*
* @param path           输入的路径字符串（UTF-8 编码）。
* @param error_message  输出参数，当返回 false 时携带具体的错误原因描述。
* @return               合法且存在时返回 true；否则返回 false，并在 error_message 中给出原因。
*
* @note
* - 本函数会访问文件系统以检查存在性（`fs::exists`），因此结果受当前工作目录、权限与挂载等影响。
* - Path traversal 检查为语义层面：仅基于字符串规范化判断是否残留 `..`，不解析符号链接。
* - Windows 下通过 `fs::u8path` 将 UTF-8 转宽字节，能正确处理中文/日文等 Unicode 路径。
*
* @par 正确示例（返回 true）
* @code{.cpp}
* std::string err;
* validatePath("C:/数据/报告.pdf", err);                       // true（Windows）
* validatePath(R"(D:\项目\子目录\配置.json)", err);             // true（Windows）
* validatePath("/home/user/ドキュメント/集計.csv", err);        // true（Linux/macOS）
* validatePath("./相对路径/設定.yaml", err);                    // true（相对路径）
* @endcode
*
* @par 错误示例（返回 false）
* @code{.cpp}
* std::string err;
* validatePath("", err);                                     // 空路径
* validatePath(std::string(5000, 'a'), err);                 // 路径过长（>4096 字节）
* validatePath("../etc/passwd", err);                        // 路径穿越
* validatePath("file://C:/secret.txt", err);                 // URL 形式
* validatePath("C:\\con\\data.txt", err);                    // Windows 设备名
* validatePath("C:\\bad<name>.txt", err);                    // Windows 非法字符
* validatePath("/tmp/not_exists_日语名.txt", err);           // 路径不存在
* @endcode
*/
bool validatePath(const std::string& path, std::string& error_message) {
    // 1) 为空？
    if (path.empty()) {
        error_message = "Path is required";
        LOG_WRN("validatePath: Empty path provided");
        return false;
    }

    // 2) 拒绝 URL（仅允许本地文件系统路径）
    if (looks_like_url(path)) {
        error_message = "URL is not allowed; local filesystem paths only";
        LOG_WRN("validatePath: URL detected: %s", path.c_str());
        return false;
    }

    // 3) 快速过滤 NUL/控制字符
    if (contains_nul_or_control(path)) {
        error_message = "Path contains NUL or control characters";
        LOG_WRN("validatePath: Control/NUL characters in path: %s", path.c_str());
        return false;
    }

    // 4) 长度限制（按 UTF-8 字节数）
    if (path.size() > 4096) {
        error_message = "Path too long (maximum 4096 bytes in UTF-8)";
        LOG_WRN("validatePath: Over-length path (len=%zu): %s", path.size(), path.c_str());
        return false;
    }

    fs::path p;
    fs::path norm;
    try {
        // 5) 将 UTF-8 转为本机路径对象（Windows 下转宽字节）
        p = fs::u8path(path);

        // 6) 语义规范化，不触发 IO
        norm = p.lexically_normal();

        // 7) Path traversal：规范化后若仍含 ".." 组件，则拒绝
        for (const auto& part : norm) {
            if (part == "..") {
                error_message = "Path traversal not allowed";
                LOG_WRN("validatePath: Traversal after normalize: %s", path.c_str());
                return false;
            }
        }

#ifdef _WIN32
        // 8) Windows 组件级合法性检查（非法字符 / 设备名 / 结尾空格点）
        // 注意：跳过根名与根目录部分，仅检查普通组件
        for (const auto& part : norm) {
            // 根名如 "C:" 或 "\\server\share" 的组件由 filesystem 处理，这里只检查普通名称
            if (part.native().empty()) continue;
            const std::wstring comp = part.native();

            // 跳过分隔或根组件（例如 "\"）
            if (comp == L"\\" || comp == L"/") continue;

            if (windows_component_invalid(comp)) {
                error_message = "Invalid Windows path component (reserved or contains forbidden characters)";
                LOG_WRN("validatePath: Windows invalid component in '%s'", path.c_str());
                return false;
            }
        }
#endif

    } catch (const std::exception& e) {
        error_message = std::string("Invalid path: ") + e.what();
        LOG_WRN("validatePath: Exception for '%s': %s", path.c_str(), e.what());
        return false;
    }

    // 9) 必须存在（文件或目录）
    std::error_code ec;
    const bool exists = fs::exists(norm, ec);
    if (ec) {
        error_message = std::string("Filesystem error: ") + ec.message();
        LOG_WRN("validatePath: exists() error for '%s': %s", path.c_str(), ec.message().c_str());
        return false;
    }
    if (!exists) {
        error_message = "Path does not exist";
        LOG_WRN("validatePath: Not exists: %s", path.c_str());
        return false;
    }

    return true;
}
#pragma endregion

#pragma region "跨平台路径编解码"
/***********************************************************
* 1、将 UTF-8 字符串安全转换为 std::filesystem::path
* 2、将 std::filesystem::path 安全转换为 UTF-8 字符串
***********************************************************/

std::filesystem::path utf8ToPath(const std::string& s) {
    return fs::u8path(s);
}

std::string pathToUtf8String(const std::filesystem::path& p) {
#if defined(__cpp_lib_char8_t)
    auto u8 = p.u8string();
    return std::string(u8.begin(), u8.end());
#else
    return p.u8string();
#endif
}
#pragma endregion

#pragma region "JSON相关实用函数"
/***********************************************************
* 1、构造错误响应JSON
* 2、构造成功响应JSON
* 3、安全的解析JSON文本
* 4、格式化JSON为字符串
***********************************************************/

json createErrorResponse(const std::string& error_message) {
    return json{
        {"error", error_message},
        {"success", false}
    };
}

json createSuccessResponse() {
    return json{{"success", true}};
}

json safeParseJson(const std::string& str) {
    try {
        return json::parse(str);
    } catch (const json::parse_error& e) {
        // e.byte 是出错的大致位置
        return json{
            {"success", false},
            {"error", "JSON parse failed"},
            {"message", e.what()},
            {"byte", e.byte}
        };
    }
}

std::string formatJson(const json& j) {
    try {
        return j.dump(2); // 2空格缩进
    } catch (const std::exception& e) {
        return "{}";
    }
}
#pragma endregion

#pragma region "时间相关实用函数"
/***********************************************************
* 1、返回本地时间戳字符串
* 2、返回当前时间的毫秒级时间戳
* 3、将 time_point 格式化成人类可读时间
***********************************************************/

std::string getCurrentTimestamp() {
    using clock = std::chrono::system_clock;
    auto now   = clock::now();
    auto tt    = clock::to_time_t(now);

    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif

    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

int64_t getCurrentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

std::string formatTimeStamp(const std::chrono::system_clock::time_point& tp) {
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}
#pragma endregion

#pragma region "字符串操作相关实用函数"
/***********************************************************
* 1、清理控制字符并修复非法UTF-8以确保JSON安全
* 2、按定界符拆分字符串
* 3、去除字符串首尾空白
* 4、按定界符连接字符串
***********************************************************/

std::string sanitizeStringForJson(const std::string& input) {
    if (input.empty()) return input;

    std::string out;
    out.reserve(input.size());

    const unsigned char* s = reinterpret_cast<const unsigned char*>(input.data());
    size_t i = 0, n = input.size();

    auto push_ascii = [&](unsigned char c) {
        // 过滤控制字符（保留 \n \r \t），DEL->空格
        if (c < 0x20) {
            if (c == '\n' || c == '\r' || c == '\t') out.push_back(static_cast<char>(c));
            else out.push_back(' ');
        } else if (c == 0x7F) {
            out.push_back(' ');
        } else {
            out.push_back(static_cast<char>(c));
        }
    };

    while (i < n) {
        unsigned char c = s[i];

        if (c <= 0x7F) { // ASCII
            push_ascii(c);
            ++i;
            continue;
        }

        // 下面按 UTF-8 合法模式进行“尝试拷贝”，否则替换
        auto need_cont = [&](size_t k){ return i + k < n && (s[i+k] & 0xC0) == 0x80; };

        if (c >= 0xC2 && c <= 0xDF && need_cont(1)) {
            out.push_back(static_cast<char>(s[i++]));
            out.push_back(static_cast<char>(s[i++]));
            continue;
        }

        if (c == 0xE0 && need_cont(1) && need_cont(2) && s[i+1] >= 0xA0 && s[i+1] <= 0xBF) {
            out.append(reinterpret_cast<const char*>(s + i), 3); i += 3; continue;
        }
        if (c >= 0xE1 && c <= 0xEC && need_cont(1) && need_cont(2)) {
            out.append(reinterpret_cast<const char*>(s + i), 3); i += 3; continue;
        }
        if (c == 0xED && need_cont(1) && need_cont(2) && s[i+1] >= 0x80 && s[i+1] <= 0x9F) {
            out.append(reinterpret_cast<const char*>(s + i), 3); i += 3; continue;
        }
        if (c >= 0xEE && c <= 0xEF && need_cont(1) && need_cont(2)) {
            out.append(reinterpret_cast<const char*>(s + i), 3); i += 3; continue;
        }

        if (c == 0xF0 && need_cont(1) && need_cont(2) && need_cont(3) && s[i+1] >= 0x90 && s[i+1] <= 0xBF) {
            out.append(reinterpret_cast<const char*>(s + i), 4); i += 4; continue;
        }
        if (c >= 0xF1 && c <= 0xF3 && need_cont(1) && need_cont(2) && need_cont(3)) {
            out.append(reinterpret_cast<const char*>(s + i), 4); i += 4; continue;
        }
        if (c == 0xF4 && need_cont(1) && need_cont(2) && need_cont(3) && s[i+1] >= 0x80 && s[i+1] <= 0x8F) {
            out.append(reinterpret_cast<const char*>(s + i), 4); i += 4; continue;
        }

        // 不合法：写入替换符并前进一字节，避免卡死
        Internal::append_replacement_char(out);
        ++i;
    }

    return out;
}

std::vector<std::string> splitString(const std::string& str, char delimiter) {
    if (str.empty()) {
        return {};
    }

    std::vector<std::string> tokens;
    tokens.reserve(std::count(str.begin(), str.end(), delimiter) + 1);

    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.emplace_back(std::move(token));
    }
    return tokens;
}

std::string trimString(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

std::string joinStrings(const std::vector<std::string>& strings, const std::string& delimiter) {
    if (strings.empty()) return "";
    if (strings.size() == 1) return strings[0];

    size_t total_size = 0;
    for (const auto& str : strings) {
        total_size += str.size();
    }
    total_size += delimiter.size() * (strings.size() - 1);

    std::string result;
    result.reserve(total_size);

    result = strings[0];
    for (size_t i = 1; i < strings.size(); ++i) {
        result += delimiter;
        result += strings[i];
    }
    return result;
}
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
* 12、检查文件是否疑似二进制文件
* 13、以unicode友好方式打开指定文件，为了能够实现对中文路径的支持
***********************************************************/
bool fileExists(const std::string& path) {
    if (path.empty()) {
        return false;
    }

    try {
        return std::filesystem::exists(fs::u8path(path));
    } catch (const std::filesystem::filesystem_error& e) {
        LOG_WRN("fileExists: Filesystem error checking path '%s': %s", path.c_str(), e.what());
        return false;
    }
}

bool is_regular_readable_file(const std::string& path, std::string& err) {
    try {
        fs::path p = fs::u8path(path);
        if (!fs::exists(p))                { err = "Path does not exist"; return false; }
        if (!fs::is_regular_file(p))       { err = "Path is not a regular file"; return false; }
        std::ifstream ifs(p, std::ios::binary);
        if (!ifs)                          { err = "Failed to open file for reading"; return false; }
        return true;
    } catch (const fs::filesystem_error& e) {
        err = e.what();
        return false;
    }
}

bool readFileContent(const std::string& path, std::string& content) {
    if (path.empty()) {
        return false;
    }

    try {
        std::ifstream file = open_ifstream_unicode(path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        file.seekg(0, std::ios::end);
        std::streampos file_size = file.tellg();

        if (file_size == std::streampos(-1)) {
            return false;
        }

        if (file_size == 0) {
            content.clear();
            return true;
        }

        size_t size = static_cast<size_t>(file_size);
        const size_t MAX_FILE_SIZE = 100 * 1024 * 1024; // 100MB

        if (size > MAX_FILE_SIZE) {
            LOG_WRN("readFileContent: File too large '%s': %zu bytes (max %zu)", path.c_str(), size, MAX_FILE_SIZE);
            return false;
        }

        file.seekg(0, std::ios::beg);
        if (file.fail()) {
            return false;
        }

        try {
            content.resize(size);
        } catch (const std::bad_alloc& e) {
            LOG_ERR("readFileContent: Memory allocation failed for file '%s': %s", path.c_str(), e.what());
            return false;
        }

        file.read(&content[0], size);

        if (file.fail() && !file.eof()) {
            content.clear();
            return false;
        }

        size_t bytes_read = static_cast<size_t>(file.gcount());
        if (bytes_read != size) {
            content.resize(bytes_read);
        }

        return true;

    } catch (const std::exception& e) {
        LOG_ERR("readFileContent: Failed to read file '%s': %s", path.c_str(), e.what());
        content.clear();
        return false;
    }
}

bool isValidUtf8File(const std::string& path) {
    if (!fileExists(path)) {
        return false;
    }

    std::string content;
    if (!readFileContent(path, content)) {
        return false;
    }

    return Utils::isValidUtf8String(content);
}

// 辅助函数
bool isValidUtf8String(const std::string& s) {
    const unsigned char* p = reinterpret_cast<const unsigned char*>(s.data());
    size_t i = 0, n = s.size();

    while (i < n) {
        unsigned char c = p[i];

        // 1-byte: 0xxxxxxx
        if (c <= 0x7F) { i += 1; continue; }

        // 2-byte: 110xxxxx 10xxxxxx, first byte C2..DF (C0/C1 禁止：避免 overlong)
        if (c >= 0xC2 && c <= 0xDF) {
            if (i + 1 >= n || !Internal::is_cont(p[i+1])) return false;
            i += 2; continue;
        }

        // 3-byte:
        //   E0 A0..BF 80..BF   （E0 第二字节 >= A0，避免 overlong）
        //   E1..EC 80..BF 80..BF
        //   ED 80..9F 80..BF   （ED 第二字节 <= 9F，避开代理区）
        //   EE..EF 80..BF 80..BF
        if (c == 0xE0) {
            if (i + 2 >= n) return false;
            unsigned char b1 = p[i+1], b2 = p[i+2];
            if (!(b1 >= 0xA0 && b1 <= 0xBF) || !Internal::is_cont(b2)) return false;
            i += 3; continue;
        }
        if (c >= 0xE1 && c <= 0xEC) {
            if (i + 2 >= n || !Internal::is_cont(p[i+1]) || !Internal::is_cont(p[i+2])) return false;
            i += 3; continue;
        }
        if (c == 0xED) {
            if (i + 2 >= n) return false;
            unsigned char b1 = p[i+1], b2 = p[i+2];
            if (!(b1 >= 0x80 && b1 <= 0x9F) || !Internal::is_cont(b2)) return false; // 禁止代理区
            i += 3; continue;
        }
        if (c >= 0xEE && c <= 0xEF) {
            if (i + 2 >= n || !Internal::is_cont(p[i+1]) || !Internal::is_cont(p[i+2])) return false;
            i += 3; continue;
        }

        // 4-byte:
        //   F0 90..BF 80..BF 80..BF  （F0 第二字节 >= 0x90，避免 overlong）
        //   F1..F3 80..BF 80..BF 80..BF
        //   F4 80..8F 80..BF 80..BF  （限制到 U+10FFFF）
        if (c == 0xF0) {
            if (i + 3 >= n) return false;
            unsigned char b1 = p[i+1], b2 = p[i+2], b3 = p[i+3];
            if (!(b1 >= 0x90 && b1 <= 0xBF) || !Internal::is_cont(b2) || !Internal::is_cont(b3)) return false;
            i += 4; continue;
        }
        if (c >= 0xF1 && c <= 0xF3) {
            if (i + 3 >= n || !Internal::is_cont(p[i+1]) || !Internal::is_cont(p[i+2]) || !Internal::is_cont(p[i+3])) return false;
            i += 4; continue;
        }
        if (c == 0xF4) {
            if (i + 3 >= n) return false;
            unsigned char b1 = p[i+1], b2 = p[i+2], b3 = p[i+3];
            if (!(b1 >= 0x80 && b1 <= 0x8F) || !Internal::is_cont(b2) || !Internal::is_cont(b3)) return false;
            i += 4; continue;
        }

        // 其它 leading bytes（如 0xC0/0xC1 或 >0xF4）一律非法
        return false;
    }
    return true;
}

bool writeFileContent(const std::string& path, const std::string& content) {
    try {
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        file.write(content.c_str(), content.size());

        if (file.fail()) {
            file.close();
            return false;
        }

        file.close();

        if (file.fail()) {
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERR("writeFileContent: Failed to write file '%s': %s", path.c_str(), e.what());
        return false;
    }
}

bool appendFileContent(const std::string& path, const std::string& content) {
    try {
        std::ofstream file(path, std::ios::binary | std::ios::app);
        if (!file.is_open()) {
            return false;
        }

        file.write(content.c_str(), content.size());

        if (file.fail()) {
            file.close();
            return false;
        }

        file.close();

        if (file.fail()) {
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERR("appendFileContent: Failed to append to file '%s': %s", path.c_str(), e.what());
        return false;
    }
}

std::vector<std::string> listDirectory(const std::string& path) {
    std::vector<std::string> result;
    try {
        namespace fs = std::filesystem;
        fs::path p = fs::u8path(path);
        if (!fs::exists(p) || !fs::is_directory(p)) return result;

        auto opts = fs::directory_options::skip_permission_denied;
        for (const auto& entry : fs::directory_iterator(p, opts)) {
            result.push_back(pathToUtf8String(entry.path().filename()));
        }
        std::sort(result.begin(), result.end());
    } catch (const std::filesystem::filesystem_error& e) {
        LOG_WRN("listDirectory: Filesystem error listing directory '%s': %s", path.c_str(), e.what());
        result.clear();
    }
    return result;
}

// GLOB：在目录树中匹配文件/目录
std::vector<fs::path> globFiles(
    const fs::path& base_dir,
    const std::string& pattern,
    bool include_directories,
    bool follow_symlinks,
    bool case_sensitive) {
    std::vector<fs::path> out;
    std::vector<std::string> segs = Internal::splitPatternSegments(pattern);

    if (segs.empty()) return out;

    // 从 base_dir 出发逐段扩展
    std::vector<fs::path> frontier = { fs::weakly_canonical(base_dir) };

    for (std::size_t i = 0; i < segs.size(); ++i) {
        const std::string& seg = segs[i];
        const bool last = (i + 1 == segs.size());

        // 双星：收集所有子树（包含当前），交给下一段去过滤
        if (seg == "**") {
            // 展开成“当前及所有后代目录”
            std::vector<fs::path> expanded;
            for (const auto& root : frontier) {
                if (!fs::exists(root) || !fs::is_directory(root)) continue;
                expanded.push_back(root);
            fs::directory_options opts = fs::directory_options::skip_permission_denied;
            if (follow_symlinks) opts |= fs::directory_options::follow_directory_symlink;
            for (auto it = fs::recursive_directory_iterator(root, opts);
                 it != fs::recursive_directory_iterator(); ++it) {
                if (it->is_directory()) expanded.push_back(it->path());
            }
            }
            frontier.swap(expanded);
            continue;
        }

        // 普通段：在每个 frontier 目录下，列出一层目录项并用通配匹配
        std::vector<fs::path> next;
        for (const auto& dir : frontier) {
            if (!fs::exists(dir) || !fs::is_directory(dir)) continue;
            fs::directory_options opts = fs::directory_options::skip_permission_denied;
            if (follow_symlinks) opts |= fs::directory_options::follow_directory_symlink;
            for (auto& de : fs::directory_iterator(dir, opts)) {
                const std::string name = pathToUtf8String(de.path().filename());
                if (!Internal::globSegmentMatch(name, seg, case_sensitive)) continue;

                if (last) {
                    // 最后一段：按需收集文件/目录
                    if (include_directories) {
                        out.push_back(de.path());
                    } else {
                        // 只要“像文件”的条目
                        if (de.is_regular_file()) out.push_back(de.path());
                    }
                } else {
                    // 中间段：只把目录继续下传
                    if (de.is_directory()) next.push_back(de.path());
                }
            }
        }
        frontier.swap(next);
    }

    // 去重 & 规范化
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}
// Grep：在文件中搜索匹配模式
std::vector<json> searchInFileRegex(
    const fs::path& filepath,
    const std::string& pattern,
    bool use_regex,
    bool case_sensitive,
    bool line_numbers,
    int& total_matches,
    int max_matches) {

    std::vector<json> matches;
    if (max_matches <= 0) return matches;

    // 跳过疑似二进制
    if (isLikelyBinary(filepath)) {
        return matches;
    }

    std::ifstream ifs(filepath);
    if (!ifs) return matches;

    std::regex re;
    std::string needle = pattern;
    if (use_regex) {
        try {
            re = std::regex(pattern,
                            case_sensitive ? std::regex::ECMAScript
                                           : (std::regex::ECMAScript | std::regex::icase));
        } catch (const std::regex_error& e) {
            // 正则编译失败：直接返回空，或可在上层记录错误信息
            return matches;
        }
    } else if (!case_sensitive) {
        needle = Internal::normalize_case(needle, false);
    }

    std::string line;
    std::size_t line_num = 1;

    while (std::getline(ifs, line)) {
        bool found = false;
        std::size_t pos0 = std::string::npos;
        std::size_t pos1 = std::string::npos;

        if (use_regex) {
            std::smatch m;
            if (std::regex_search(line, m, re)) {
                found = true;
                pos0 = static_cast<std::size_t>(m.position());
                pos1 = pos0 + static_cast<std::size_t>(m.length());
            }
        } else {
            if (case_sensitive) {
                pos0 = line.find(needle);
            } else {
                std::string lower_line = Internal::normalize_case(line, false);
                pos0 = lower_line.find(needle);
            }
            found = (pos0 != std::string::npos);
            if (found) pos1 = pos0 + needle.size();
        }

        if (found) {
            json j = {
                {"file", pathToUtf8String(filepath)},
                {"line_content", line},
                {"match_start", static_cast<int>(pos0)},
                {"match_end", static_cast<int>(pos1)}
            };
            if (line_numbers) j["line_number"] = static_cast<int>(line_num);

            matches.push_back(std::move(j));
            ++total_matches;

            if (total_matches >= max_matches) break;
        }
        ++line_num;
    }

    return matches;
}
// 检测文本是否疑似二进制文件
bool isLikelyBinary(
    const fs::path& filepath,
    std::size_t probe) {
    // 以二进制方式打开
    std::ifstream ifs(filepath, std::ios::binary);
    if (!ifs) return false; // 打不开就别当二进制处理
    std::string buf;
    buf.resize(probe);
    ifs.read(&buf[0], static_cast<std::streamsize>(buf.size()));
    std::streamsize n = ifs.gcount();
    for (std::streamsize i = 0; i < n; ++i) {
        unsigned char c = static_cast<unsigned char>(buf[static_cast<std::size_t>(i)]);
        if (c == 0) return true; // NUL 字节高概率是二进制
    }
    return false;
}

bool isLikelyBinaryString(const std::string& buffer) {
    for (unsigned char c : buffer) {
        if (c == 0) return true;
    }
    return false;
}
// 以unicode友好方式打开指定文件，为了能够实现对中文路径的支持
std::ifstream open_ifstream_unicode(
    const std::string& path,
    std::ios::openmode mode) {
    // 使用 u8path 保证在 Windows 上走宽字符路径（支持中文等非 ASCII 路径）
    return std::ifstream(fs::u8path(path), mode);
}
#pragma endregion

#pragma region "格式化辅助实用函数"
std::string formatFileSize(uintmax_t size_bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    double size = static_cast<double>(size_bytes);
    int unit = 0;
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        ++unit;
    }
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1) << size << ' ' << units[unit];
    return ss.str();
}
#pragma endregion

} // namespace Utils
} // namespace BuiltinTools
