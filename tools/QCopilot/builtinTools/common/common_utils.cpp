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

namespace BuiltinTools {
namespace Utils {

bool validatePath(const std::string& path, std::string& error_message) {
    if (path.empty()) {
        error_message = "Path is required";
        LOG_WRN("validatePath: Empty path provided");
        return false;
    }

    // 路径安全检查：防止路径遍历攻击
    if (path.find("..") != std::string::npos) {
        error_message = "Path traversal not allowed";
        LOG_WRN("validatePath: Path traversal attempt detected: %s", path.c_str());
        return false;
    }

    // 检查路径长度是否合理
    if (path.length() > 4096) {
        error_message = "Path too long (maximum 4096 characters)";
        return false;
    }

    return true;
}

bool validateStringLength(const std::string& str, size_t max_length, const std::string& field_name, std::string& error_message) {
    if (str.length() > max_length) {
        error_message = field_name + " too long (maximum " + std::to_string(max_length) + " characters)";
        return false;
    }
    return true;
}

json createErrorResponse(const std::string& error_message) {
    return json{
        {"error", error_message},
        {"success", false}
    };
}

json createSuccessResponse() {
    return json{{"success", true}};
}

// 时间工具实现
std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

int64_t getCurrentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

// 字符串工具实现
std::string sanitizeStringForJson(const std::string& input) {
    if (input.empty()) {
        return input;
    }

    // 验证UTF-8并清理控制字符
    bool is_valid_utf8 = true;
    for (size_t i = 0; i < input.size(); ) {
        unsigned char c = static_cast<unsigned char>(input[i]);

        if (c < 0x80) {
            i++;
        } else if ((c & 0xE0) == 0xC0) {
            if (i + 1 >= input.size() || (static_cast<unsigned char>(input[i+1]) & 0xC0) != 0x80) {
                is_valid_utf8 = false;
                break;
            }
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            if (i + 2 >= input.size() ||
                (static_cast<unsigned char>(input[i+1]) & 0xC0) != 0x80 ||
                (static_cast<unsigned char>(input[i+2]) & 0xC0) != 0x80) {
                is_valid_utf8 = false;
                break;
            }
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            if (i + 3 >= input.size() ||
                (static_cast<unsigned char>(input[i+1]) & 0xC0) != 0x80 ||
                (static_cast<unsigned char>(input[i+2]) & 0xC0) != 0x80 ||
                (static_cast<unsigned char>(input[i+3]) & 0xC0) != 0x80) {
                is_valid_utf8 = false;
                break;
            }
            i += 4;
        } else {
            is_valid_utf8 = false;
            break;
        }
    }

    if (is_valid_utf8) {
        return input;
    }

    // 清理无效字符
    std::string result;
    result.reserve(input.size());

    for (size_t i = 0; i < input.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(input[i]);

        if (c < 32) {
            if (c == '\n' || c == '\r' || c == '\t') {
                result += c;
            } else {
                result += ' ';
            }
        } else if (c < 127) {
            result += c;
        } else if (c == 127) {
            result += ' ';
        } else {
            result += c;
        }
    }

    return result;
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

// 文件工具实现
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
        if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
            return result;
        }

        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            result.push_back(entry.path().filename().string());
        }

        std::sort(result.begin(), result.end());
    } catch (const std::filesystem::filesystem_error& e) {
        LOG_WRN("listDirectory: Filesystem error listing directory '%s': %s", path.c_str(), e.what());
        result.clear();
    }

    return result;
}

bool readTextFileWithEncodingAndRange(
    const std::string& path,
    int start_line,
    int end_line,
    std::string& content,
    int& lines_read,
    int& actual_end_line) {

    if (path.empty()) {
        return false;
    }

    try {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        std::string line;
        std::vector<std::string> lines;
        int current_line = 1;

        // 读取所有行
        while (std::getline(file, line)) {
            if (current_line >= start_line) {
                if (end_line > 0 && current_line > end_line) {
                    break;
                }
                lines.push_back(line);
            }
            current_line++;
        }

        // 构建内容
        content.clear();
        for (size_t i = 0; i < lines.size(); ++i) {
            if (i > 0) content += "\\n";
            content += lines[i];
        }

        lines_read = static_cast<int>(lines.size());
        actual_end_line = start_line + lines_read - 1;

        return true;

    } catch (const std::exception& e) {
        LOG_ERR("readTextFileWithEncodingAndRange: Failed to read text file '%s': %s", path.c_str(), e.what());
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

    // 简单的UTF-8验证
    for (size_t i = 0; i < content.size(); ) {
        unsigned char c = static_cast<unsigned char>(content[i]);

        if (c < 0x80) {
            i++;
        } else if ((c & 0xE0) == 0xC0) {
            if (i + 1 >= content.size() || (static_cast<unsigned char>(content[i+1]) & 0xC0) != 0x80) {
                return false;
            }
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            if (i + 2 >= content.size() ||
                (static_cast<unsigned char>(content[i+1]) & 0xC0) != 0x80 ||
                (static_cast<unsigned char>(content[i+2]) & 0xC0) != 0x80) {
                return false;
            }
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            if (i + 3 >= content.size() ||
                (static_cast<unsigned char>(content[i+1]) & 0xC0) != 0x80 ||
                (static_cast<unsigned char>(content[i+2]) & 0xC0) != 0x80 ||
                (static_cast<unsigned char>(content[i+3]) & 0xC0) != 0x80) {
                return false;
            }
            i += 4;
        } else {
            return false;
        }
    }
    return true;
}

// JSON工具实现
json safeParseJson(const std::string& str) {
    try {
        return json::parse(str);
    } catch (const json::parse_error& e) {
        return json{{"error", "JSON parse failed"}, {"success", false}};
    }
}

std::string formatJson(const json& j) {
    try {
        return j.dump(2); // 2空格缩进
    } catch (const std::exception& e) {
        return "{}";
    }
}

} // namespace Utils
} // namespace BuiltinTools
