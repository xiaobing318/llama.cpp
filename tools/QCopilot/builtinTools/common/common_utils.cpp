#include "common_utils.h"
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

namespace fs = std::filesystem;

namespace BuiltinTools {
namespace Utils {

#pragma region "内部辅助函数"
// 辅助：不安全的 tolower（假定输入是 char 范围内）
static inline char to_lower_unsafe(char c) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

// 辅助：统一大小写（若不区分大小写）
static std::string normalize_case(
    std::string s,
    bool case_sensitive) {
    if (!case_sensitive) {
        std::transform(s.begin(), s.end(), s.begin(), to_lower_unsafe);
    }
    return s;
}

// 辅助：统一路径分隔符为 '/'（仅影响匹配逻辑，不改动实际路径）
static std::string slashify(std::string s) {
#ifdef _WIN32
    std::replace(s.begin(), s.end(), '\\', '/');
#endif
    return s;
}

// 辅助：匹配字符类（方括号内），支持范围和否定
static bool matchCharClass(
    char c,
    const std::string& cls,
    bool case_sensitive){
    // cls 不含方括号本体：已在解析阶段剔除了 [ 和 ]
    if (!case_sensitive) c = to_lower_unsafe(c);
    bool negate = false;
    std::size_t i = 0;
    if (i < cls.size() && cls[i] == '!') { negate = true; ++i; }

    bool ok = false;
    while (i < cls.size()) {
        char first = cls[i++];
        if (!case_sensitive) first = to_lower_unsafe(first);
        // 范围或单字符
        if (i + 1 < cls.size() && cls[i] == '-' ) {
            // 范围 a-z，skip '-'
            ++i;
            char last = cls[i++];
            if (!case_sensitive) last = to_lower_unsafe(last);
            if (first <= c && c <= last) ok = true;
        } else {
            if (first == c) ok = true;
        }
    }
    return negate ? !ok : ok;
}

// 辅助：// 辅助：通配符匹配核心：单段匹配（不含路径分隔符），支持 *, ?, [],\（不跨分隔符）
static bool globSegmentMatch(
    const std::string& text,
    const std::string& pat,
    bool case_sensitive) {
    // 匹配单个路径段（不包含分隔符），支持 *, ?, []
    // DP/回溯均可；这里用回溯实现，简洁直观。
    const std::string t = case_sensitive ? text : normalize_case(text, false);
    const std::string p = case_sensitive ? pat  : normalize_case(pat,  false);

    std::size_t ti = 0, pi = 0;
    std::size_t star_pi = std::string::npos, star_ti = 0;

    while (ti < t.size()) {
        if (pi < p.size()) {
            if (p[pi] == '?') {
                ++pi; ++ti; continue;
            }
            if (p[pi] == '\\') {
                // 字面量转义
                ++pi;
                if (pi < p.size() && p[pi] == t[ti]) { ++pi; ++ti; continue; }
            } else if (p[pi] == '[') {
                // 字符类
                std::size_t end = p.find(']', pi + 1);
                if (end == std::string::npos) return false; // 不完整
                std::string cls = p.substr(pi + 1, end - (pi + 1));
                char tc = text[ti];
                if (!case_sensitive) tc = to_lower_unsafe(tc);
                if (!matchCharClass(tc, cls, case_sensitive)) {
                    // 不匹配，若之前见过 '*'，尝试吞字符
                    if (star_pi != std::string::npos) { ti = ++star_ti; pi = star_pi + 1; continue; }
                    return false;
                }
                pi = end + 1; ++ti; continue;
            } else if (p[pi] == '*') {
                // 记录星号位置，先让它匹配空，必要时回溯多吞一点
                star_pi = pi;
                star_ti = ti;
                ++pi;
                continue;
            } else if (p[pi] == t[ti]) {
                ++pi; ++ti; continue;
            }
        }
        // 不匹配时，如之前有 '*'，回溯：多吞一个字符
        if (star_pi != std::string::npos) {
            ti = ++star_ti;
            pi = star_pi + 1;
            continue;
        }
        return false;
    }
    // 吃掉末尾连续的 '*'
    while (pi < p.size() && p[pi] == '*') ++pi;
    return pi == p.size();
}

// 辅助：将 pattern 按 '/' 拆分成段
static std::vector<std::string> splitPatternSegments(std::string pat) {
    pat = slashify(std::move(pat));
    std::vector<std::string> segs;
    std::string cur;
    for (char ch : pat) {
        if (ch == '/') {
            if (!cur.empty()) segs.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(ch);
        }
    }
    if (!cur.empty()) segs.push_back(cur);
    return segs;
}

// 辅助：检测是否疑似二进制文件
static bool isLikelyBinary(
    const fs::path& filepath,
    std::size_t probe = 4096) {
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
#pragma endregion

#pragma region "匹配模式相关实用函数"
/***********************************************************
* 1、匹配模式
***********************************************************/

// 辅助：通配符匹配（支持 *, ?, [], \；* 不跨分隔符）
bool matchPattern(
    const std::string& text,
    const std::string& pattern,
    bool case_sensitive) {
    // 这里匹配的是“单个段”，所以先把分隔符统一后，禁止跨分隔符。
    std::string t = slashify(text);
    if (t.find('/') != std::string::npos) {
        // 若上层传的是“整条路径”，请先拆段后用；这里按典型 glob 约定：* 不跨分隔符。
        // 让调用方分段；或者在 globFiles 中处理。
    }
    return globSegmentMatch(t, slashify(pattern), case_sensitive);
}
#pragma endregion

#pragma region "路径相关实用函数"
/***********************************************************
* 1、验证路径合法性
***********************************************************/
bool validatePath(const std::string& path, std::string& error_message) {
    // 路径是否为空
    if (path.empty()) {
        error_message = "Path is required";
        LOG_WRN("validatePath: Empty path provided");
        return false;
    }
    // 路径长度是否过长
    if (path.length() > 4096) {
        error_message = "Path too long (maximum 4096 characters)";
        return false;
    }
    try {
        std::filesystem::path p(path);
        // 仅做“语义穿越”检查：规范化后若仍含有“..”组件，判为不安全
        auto norm = p.lexically_normal();
        for (const auto& part : norm) {
            if (part == "..") {
                error_message = "Path traversal not allowed";
                LOG_WRN("validatePath: Path traversal detected after normalize: %s", path.c_str());
                return false;
            }
        }
        // 如需限制到某根目录，可在此比较 norm 是否以允许前缀开头
    } catch (const std::exception& e) {
        error_message = std::string("Invalid path: ") + e.what();
        LOG_WRN("validatePath: Exception for path '%s': %s", path.c_str(), e.what());
        return false;
    }
    return true;
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
#pragma endregion

#pragma region "字符串操作相关实用函数"
/***********************************************************
* 1、清理控制字符并修复非法UTF-8以确保JSON安全
* 2、按定界符拆分字符串
* 3、去除字符串首尾空白
* 4、按定界符连接字符串
***********************************************************/

// 辅助函数
static inline void append_replacement_char(std::string& out) {
    out.push_back(static_cast<char>(0xEF));
    out.push_back(static_cast<char>(0xBF));
    out.push_back(static_cast<char>(0xBD));
}
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
        Utils::append_replacement_char(out);
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
* 10、检测文件是否为二进制文件
* 11、在目录书中匹配文件/目录
* 12、在文件中搜索
***********************************************************/
bool fileExists(const std::string& path) {
    if (path.empty()) {
        return false;
    }

    try {
        return std::filesystem::exists(path);
    } catch (const std::filesystem::filesystem_error& e) {
        LOG_WRN("fileExists: Filesystem error checking path '%s': %s", path.c_str(), e.what());
        return false;
    }
}

bool is_regular_readable_file(const std::string& path, std::string& err) {
    try {
        fs::path p(path);
        if (!fs::exists(p))                { err = "Path does not exist"; return false; }
        if (!fs::is_regular_file(p))       { err = "Path is not a regular file"; return false; }
        std::ifstream ifs(path, std::ios::binary);
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
        std::ifstream file(path, std::ios::binary);
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

bool readTextFileWithRange(
    const std::string& path,
    int start_line,
    int end_line,
    std::string& content,
    int& lines_read,
    int& actual_end_line) {

    content.clear();
    lines_read = 0;
    actual_end_line = 0;

    if (path.empty()) return false;
    if (start_line <= 0) start_line = 1; // 更健壮的入参防御

    try {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) return false;

        // 读取前3字节以检测 UTF-8 BOM
        unsigned char bom[3] = {0};
        file.read(reinterpret_cast<char*>(bom), 3);
        std::streamsize got = file.gcount();
        if (!(got == 3 && bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF)) {
            // 非UTF-8 BOM：回退
            file.clear();
            file.seekg(0, std::ios::beg);
        }

        std::string line;
        std::vector<std::string> lines;
        lines.reserve((end_line > 0) ? std::max(0, end_line - start_line + 1) : 256);

        int current_line = 1;
        while (std::getline(file, line)) {
            // 如果是CRLF，getline去掉LF，但可能保留结尾的CR
            if (!line.empty() && line.back() == '\r') line.pop_back();

            if (current_line >= start_line) {
                if (end_line > 0 && current_line > end_line) break;
                lines.emplace_back(std::move(line));
            }
            ++current_line;
        }

        // 用真实换行拼接
        for (size_t i = 0; i < lines.size(); ++i) {
            if (i) content.push_back('\n');
            content += lines[i];
        }

        lines_read = static_cast<int>(lines.size());
        actual_end_line = (lines_read > 0) ? (start_line + lines_read - 1) : (start_line - 1);

        return true;

    } catch (const std::exception& e) {
        LOG_ERR("readTextFileWithRange: Failed to read text file '%s': %s", path.c_str(), e.what());
        content.clear();
        lines_read = 0;
        actual_end_line = 0;
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
static inline bool is_cont(unsigned char x) { return (x & 0xC0) == 0x80; }
bool isValidUtf8String(const std::string& s) {
    const unsigned char* p = reinterpret_cast<const unsigned char*>(s.data());
    size_t i = 0, n = s.size();

    while (i < n) {
        unsigned char c = p[i];

        // 1-byte: 0xxxxxxx
        if (c <= 0x7F) { i += 1; continue; }

        // 2-byte: 110xxxxx 10xxxxxx, first byte C2..DF (C0/C1 禁止：避免 overlong)
        if (c >= 0xC2 && c <= 0xDF) {
            if (i + 1 >= n || !is_cont(p[i+1])) return false;
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
            if (!(b1 >= 0xA0 && b1 <= 0xBF) || !is_cont(b2)) return false;
            i += 3; continue;
        }
        if (c >= 0xE1 && c <= 0xEC) {
            if (i + 2 >= n || !is_cont(p[i+1]) || !is_cont(p[i+2])) return false;
            i += 3; continue;
        }
        if (c == 0xED) {
            if (i + 2 >= n) return false;
            unsigned char b1 = p[i+1], b2 = p[i+2];
            if (!(b1 >= 0x80 && b1 <= 0x9F) || !is_cont(b2)) return false; // 禁止代理区
            i += 3; continue;
        }
        if (c >= 0xEE && c <= 0xEF) {
            if (i + 2 >= n || !is_cont(p[i+1]) || !is_cont(p[i+2])) return false;
            i += 3; continue;
        }

        // 4-byte:
        //   F0 90..BF 80..BF 80..BF  （F0 第二字节 >= 0x90，避免 overlong）
        //   F1..F3 80..BF 80..BF 80..BF
        //   F4 80..8F 80..BF 80..BF  （限制到 U+10FFFF）
        if (c == 0xF0) {
            if (i + 3 >= n) return false;
            unsigned char b1 = p[i+1], b2 = p[i+2], b3 = p[i+3];
            if (!(b1 >= 0x90 && b1 <= 0xBF) || !is_cont(b2) || !is_cont(b3)) return false;
            i += 4; continue;
        }
        if (c >= 0xF1 && c <= 0xF3) {
            if (i + 3 >= n || !is_cont(p[i+1]) || !is_cont(p[i+2]) || !is_cont(p[i+3])) return false;
            i += 4; continue;
        }
        if (c == 0xF4) {
            if (i + 3 >= n) return false;
            unsigned char b1 = p[i+1], b2 = p[i+2], b3 = p[i+3];
            if (!(b1 >= 0x80 && b1 <= 0x8F) || !is_cont(b2) || !is_cont(b3)) return false;
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
        if (!fs::exists(path) || !fs::is_directory(path)) return result;

        for (const auto& entry :
             fs::directory_iterator(path, fs::directory_options::skip_permission_denied)) {
            result.push_back(entry.path().filename().string());
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
    std::vector<std::string> segs = splitPatternSegments(pattern);

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
                fs::directory_options opts = follow_symlinks ? fs::directory_options::follow_directory_symlink
                                                             : fs::directory_options::none;
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
            fs::directory_options opts = follow_symlinks ? fs::directory_options::follow_directory_symlink
                                                         : fs::directory_options::none;
            for (auto& de : fs::directory_iterator(dir, opts)) {
                const std::string name = de.path().filename().string();
                if (!globSegmentMatch(name, seg, case_sensitive)) continue;

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


// Grep：在文件中搜索
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
        needle = normalize_case(needle, false);
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
                std::string lower_line = normalize_case(line, false);
                pos0 = lower_line.find(needle);
            }
            found = (pos0 != std::string::npos);
            if (found) pos1 = pos0 + needle.size();
        }

        if (found) {
            json j = {
                {"file", filepath.string()},
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

#pragma endregion

} // namespace Utils
} // namespace BuiltinTools
