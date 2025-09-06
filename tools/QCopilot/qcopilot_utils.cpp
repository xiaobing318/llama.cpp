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
#include <mutex>
#include <cstdarg>
#include <cctype>
#include <set>

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

/*
Notes:
1、如果配置中没有设置日志详细级别，这里默认将日志基准级别设置成 INFO 级别，日志可以通过配置文件实现调整。
2、这里将日志等级设置成 INFO 级别，意味着 INFO 及以上级别的日志都会被输出，而 DEBUG 级别的日志则会被忽略。
3、这里将日志等级设置成 INFO 级别另外一个原因是不论哪一种配置下都是可以输出内置工具相关信息。
*/
LogLevel   Logger::current_level_ = LogLevel::INFO;
std::mutex Logger::level_mutex_;

void Logger::set_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(level_mutex_);
    current_level_ = level;
}

LogLevel Logger::get_level() {
    std::lock_guard<std::mutex> lock(level_mutex_);
    return current_level_;
}

std::string Logger::get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

const char* Logger::level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERR: return "ERROR";
        case LogLevel::NONE:  return "NONE";
        default:              return "UNKNOWN";
    }
}

LogLevel Logger::string_to_level(const std::string& level_str) {
    std::string upper_str = level_str;
    std::transform(upper_str.begin(), upper_str.end(), upper_str.begin(), ::toupper);

    if (upper_str == "DEBUG") return LogLevel::DEBUG;
    if (upper_str == "INFO")  return LogLevel::INFO;
    if (upper_str == "WARN")  return LogLevel::WARN;
    if (upper_str == "ERROR") return LogLevel::ERR;
    if (upper_str == "NONE")  return LogLevel::NONE;

    // 默认返回INFO级别
    return LogLevel::INFO;
}

void Logger::set_level_from_string(const std::string& level_str) {
    set_level(string_to_level(level_str));
}

void Logger::log(LogLevel level, const char* file, int line, const char* format, ...) {
    // Thread-safe level check
    {
        std::lock_guard<std::mutex> level_lock(level_mutex_);
        if (level < current_level_) {
            return;
        }
    }

    // Thread-safe logging
    static std::mutex log_mutex;
    std::lock_guard<std::mutex> lock(log_mutex);

    // Extract filename from path for cleaner output
    const char* filename = strrchr(file, '/');
    if (!filename) filename = strrchr(file, '\\');  // Windows path separator
    filename = filename ? filename + 1 : file;

    // Format timestamp and header
    std::string timestamp = get_timestamp();
    FILE* output = (level == LogLevel::ERR) ? stderr : stdout;

    fprintf(output, "[%s] [%s] [%s:%d] ",
            timestamp.c_str(),
            level_to_string(level),
            filename,
            line);

    // Format and output the message
    va_list args;
    va_start(args, format);
    vfprintf(output, format, args);
    va_end(args);

    // Ensure newline at the end of each log message
    fprintf(output, "\n");
    fflush(output);
}

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
    if (str.empty()) {
        return {};
    }

    std::vector<std::string> tokens;
    // Reserve space for better performance
    tokens.reserve(std::count(str.begin(), str.end(), delimiter) + 1);

    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.emplace_back(std::move(token));
    }
    return tokens;
}

std::string join_strings(const std::vector<std::string>& strings, const std::string& delimiter) {
    if (strings.empty()) return "";
    if (strings.size() == 1) return strings[0];

    // Calculate total size for better performance
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

std::string sanitize_string_for_json(const std::string& str) {
    if (str.empty()) {
        return str;
    }
    /*
        清理字符串中的控制字符和无效UTF-8字节
        1. 先验证整个字符串是否为有效UTF-8
        2. 如果不是，则逐字符清理
    */
    bool is_valid_utf8 = true;
    for (size_t i = 0; i < str.size(); ) {
        unsigned char c = static_cast<unsigned char>(str[i]);

        if (c < 0x80) {
            // ASCII字符
            i++;
        } else if ((c & 0xE0) == 0xC0) {
            // 2字节UTF-8
            if (i + 1 >= str.size() || (static_cast<unsigned char>(str[i+1]) & 0xC0) != 0x80) {
                is_valid_utf8 = false;
                break;
            }
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            // 3字节UTF-8
            if (i + 2 >= str.size() ||
                (static_cast<unsigned char>(str[i+1]) & 0xC0) != 0x80 ||
                (static_cast<unsigned char>(str[i+2]) & 0xC0) != 0x80) {
                is_valid_utf8 = false;
                break;
            }
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            // 4字节UTF-8
            if (i + 3 >= str.size() ||
                (static_cast<unsigned char>(str[i+1]) & 0xC0) != 0x80 ||
                (static_cast<unsigned char>(str[i+2]) & 0xC0) != 0x80 ||
                (static_cast<unsigned char>(str[i+3]) & 0xC0) != 0x80) {
                is_valid_utf8 = false;
                break;
            }
            i += 4;
        } else {
            is_valid_utf8 = false;
            break;
        }
    }

    // 如果整个字符串都是有效UTF-8，直接返回
    if (is_valid_utf8) {
        return str;
    }

    // 否则，逐字节处理并清理
    std::string result;
    result.reserve(str.size());

    for (size_t i = 0; i < str.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(str[i]);

        if (c < 32) {
            // 控制字符
            if (c == '\n' || c == '\r' || c == '\t') {
                result += c;
            } else {
                result += ' ';
            }
        } else if (c < 127) {
            // 正常ASCII字符
            result += c;
        } else if (c == 127) {
            // DEL字符
            result += ' ';
        } else {
            // 高位字符，保留原始字节（应该是有效的UTF-8）
            result += c;
        }
    }

    return result;
}

bool file_exists(const std::string& path) {
    if (path.empty()) {
        return false;
    }

    try {
        return fs::exists(path);
    } catch (const fs::filesystem_error& e) {
        LOG_ERR("Filesystem error checking if file exists %s: %s", path.c_str(), e.what());
        return false;
    }
}

bool read_file_content(const std::string& path, std::string& content) {
    if (path.empty()) {
        LOG_ERR("Empty path provided to read_file_content");
        return false;
    }

    try {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            LOG_ERR("Failed to open file %s", path.c_str());
            return false;
        }

        // 获取文件大小
        file.seekg(0, std::ios::end);
        std::streampos file_size = file.tellg();

        // 检查 tellg() 是否失败
        if (file_size == std::streampos(-1)) {
            LOG_ERR("Failed to get file size for %s", path.c_str());
            return false;  // RAII will handle file close
        }

        // 检查文件是否为空
        if (file_size == 0) {
            content.clear();
            return true;  // RAII will handle file close
        }

        // 转换为 size_t 并检查是否超出合理范围
        size_t size = static_cast<size_t>(file_size);
        const size_t MAX_FILE_SIZE = 100 * 1024 * 1024; // 100MB 限制

        if (size > MAX_FILE_SIZE) {
            LOG_ERR("File %s is too large (%zu bytes, maximum %zu bytes)",
                    path.c_str(), size, MAX_FILE_SIZE);
            return false;  // RAII will handle file close
        }

        // 回到文件开始位置
        file.seekg(0, std::ios::beg);
        if (file.fail()) {
            LOG_ERR("Failed to seek to beginning of file %s", path.c_str());
            return false;  // RAII will handle file close
        }

        // 预分配内存
        try {
            content.resize(size);
        } catch (const std::bad_alloc& e) {
            LOG_ERR("Failed to allocate memory for file %s: %s", path.c_str(), e.what());
            return false;  // RAII will handle file close
        }

        // 读取文件内容
        file.read(&content[0], size);

        // 检查读取是否成功
        if (file.fail() && !file.eof()) {
            LOG_ERR("Failed to read file %s (read %zu bytes out of %zu)",
                    path.c_str(), static_cast<size_t>(file.gcount()), size);
            content.clear();
            return false;  // RAII will handle file close
        }

        // 调整内容大小为实际读取的字节数
        size_t bytes_read = static_cast<size_t>(file.gcount());
        if (bytes_read != size) {
            LOG_WRN("Read %zu bytes from file %s, expected %zu bytes",
                    bytes_read, path.c_str(), size);
            content.resize(bytes_read);
        }

        return true;  // RAII will handle file close

    } catch (const std::ios_base::failure& e) {
        LOG_ERR("IO error reading file %s: %s", path.c_str(), e.what());
        content.clear();
        return false;
    } catch (const std::bad_alloc& e) {
        LOG_ERR("Memory allocation error reading file %s: %s", path.c_str(), e.what());
        content.clear();
        return false;
    } catch (const std::exception& e) {
        LOG_ERR("Unexpected error reading file %s: %s", path.c_str(), e.what());
        content.clear();
        return false;
    } catch (...) {
        LOG_ERR("Unknown error reading file %s", path.c_str());
        content.clear();
        return false;
    }
}

bool write_file_content(const std::string& path, const std::string& content) {
    try {
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) {
            LOG_ERR("Failed to open file %s for writing", path.c_str());
            return false;
        }

        file.write(content.c_str(), content.size());

        // Check if write operation failed
        if (file.fail()) {
            LOG_ERR("Failed to write content to file %s", path.c_str());
            file.close();
            return false;
        }

        file.close();

        // Check if close operation failed
        if (file.fail()) {
            LOG_ERR("Failed to close file %s after writing", path.c_str());
            return false;
        }

        return true;
    } catch (const std::ios_base::failure& e) {
        LOG_ERR("IO error writing file %s: %s", path.c_str(), e.what());
        return false;
    } catch (const std::exception& e) {
        LOG_ERR("Unexpected error writing file %s: %s", path.c_str(), e.what());
        return false;
    }
}

bool append_file_content(const std::string& path, const std::string& content) {
    try {
        std::ofstream file(path, std::ios::binary | std::ios::app);
        if (!file.is_open()) {
            LOG_ERR("Failed to open file %s for appending", path.c_str());
            return false;
        }

        file.write(content.c_str(), content.size());

        // Check if write operation failed
        if (file.fail()) {
            LOG_ERR("Failed to append content to file %s", path.c_str());
            file.close();
            return false;
        }

        file.close();

        // Check if close operation failed
        if (file.fail()) {
            LOG_ERR("Failed to close file %s after appending", path.c_str());
            return false;
        }

        return true;
    } catch (const std::ios_base::failure& e) {
        LOG_ERR("IO error appending to file %s: %s", path.c_str(), e.what());
        return false;
    } catch (const std::exception& e) {
        LOG_ERR("Unexpected error appending to file %s: %s", path.c_str(), e.what());
        return false;
    }
}

bool read_text_file_with_encoding_and_range(
    const std::string& path,
    int start_line,
    int end_line,
    std::string& content,
    int& lines_read,
    int& actual_end_line) {
    // 如果路径为空，直接返回错误
    if (path.empty()) {
        LOG_ERR("Empty path provided to read_text_file_with_encoding_and_range");
        return false;
    }

    try {
        // 使用二进制模式打开文件，避免文本模式的编码转换问题
        std::ifstream file(path, std::ios::binary);
        // 检查文件是否成功打开
        if (!file.is_open()) {
            LOG_ERR("Failed to open text file %s", path.c_str());
            return false;
        }

        // 基于文件扩展名判断是否为文本文件
        std::filesystem::path fs_path(path);
        std::string extension = fs_path.extension().string();

        // 转换为小写以便比较
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

        // 定义常见的文本文件扩展名
        std::set<std::string> text_extensions = {
            // 源代码文件
            ".c", ".cpp", ".cxx", ".cc", ".c++", ".h", ".hpp", ".hxx", ".hh", ".h++",
            ".py", ".pyw", ".java", ".js", ".jsx", ".ts", ".tsx", ".php", ".rb", ".go",
            ".rs", ".swift", ".kt", ".scala", ".clj", ".hs", ".ml", ".fs", ".vb", ".cs",
            ".pl", ".pm", ".sh", ".bash", ".zsh", ".fish", ".ps1", ".bat", ".cmd",

            // 配置文件
            ".ini", ".conf", ".config", ".cfg", ".toml", ".yaml", ".yml", ".json",
            ".xml", ".plist", ".properties", ".env", ".gitignore", ".gitconfig",

            // 文档和标记语言
            ".txt", ".md", ".markdown", ".rst", ".asciidoc", ".tex", ".rtf",
            ".html", ".htm", ".xhtml", ".css", ".scss", ".sass", ".less",

            // 数据文件
            ".csv", ".tsv", ".sql", ".log", ".logs", ".out", ".err",

            // 脚本和配置
            ".makefile", ".cmake", ".dockerfile", ".vagrantfile",
            ".gemfile", ".podfile", ".rakefile",

            // 其他常见文本格式
            ".diff", ".patch", ".asm", ".s", ".inc", ".def", ".idl",
            ".proto", ".thrift", ".avro", ".graphql", ".gql",

            // Web相关
            ".vue", ".svelte", ".angular", ".jsp", ".asp", ".aspx", ".ejs", ".hbs",

            // 模板文件
            ".tpl", ".template", ".tmpl", ".mustache", ".handlebars",

            // 数据交换格式
            ".rss", ".atom", ".opml", ".kml", ".gpx", ".svg",

            // 版本控制和项目文件
            ".gitattributes", ".editorconfig", ".eslintrc", ".prettierrc",
            ".babelrc", ".npmignore", ".dockerignore",

            // 无扩展名的常见文件（通过文件名判断）
            // 这些将在后面单独处理
        };

        // 检查扩展名是否在文本文件列表中
        bool is_text_by_extension = text_extensions.find(extension) != text_extensions.end();

        // 对于没有扩展名的文件，检查常见的文本文件名
        if (extension.empty()) {
            std::string filename = fs_path.filename().string();
            std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);

            std::set<std::string> text_filenames = {
                "readme", "license", "copying", "changelog", "changes", "news",
                "authors", "contributors", "install", "todo", "makefile",
                "dockerfile", "vagrantfile", "gemfile", "rakefile", "podfile",
                "cmakelists.txt", ".gitignore", ".gitconfig", ".editorconfig",
                ".eslintrc", ".prettierrc", ".babelrc", ".npmignore", ".dockerignore"
            };

            is_text_by_extension = text_filenames.find(filename) != text_filenames.end();
        }

        // 如果根据扩展名判断不是文本文件，直接返回错误
        if (!is_text_by_extension) {
            LOG_ERR("File %s does not have a recognized text file extension", path.c_str());
            return false;
        }

        // 读取整个文件内容
        std::string file_content;
        file.seekg(0, std::ios::end);
        size_t file_size = file.tellg();
        file.seekg(0, std::ios::beg);

        file_content.resize(file_size);
        file.read(&file_content[0], file_size);

        // 手动分割行
        std::vector<std::string> lines;
        std::istringstream iss(file_content);
        std::string line;

        while (std::getline(iss, line)) {
            lines.push_back(line);
        }

        // 根据行范围提取内容
        int current_line = 1;
        lines_read = 0;
        actual_end_line = end_line;
        bool reached_end_line = false; // 标志是否到达指定的结束行

        content.clear();
        for (size_t i = 0; i < lines.size(); ++i) {
            if (current_line >= start_line) {
                if (lines_read > 0) {
                    content += '\n';
                }
                content += lines[i];
                lines_read++;

                // 如果指定了结束行且到达了结束行，停止
                if (end_line != -1 && current_line >= end_line) {
                    actual_end_line = current_line;
                    reached_end_line = true;
                    break;
                }
            }
            current_line++;
        }

        // 只有在end_line为-1（读取到文件末尾）或者没有达到指定的结束行时，才更新actual_end_line
        if (end_line == -1 || !reached_end_line) {
            actual_end_line = current_line - 1;
        }

        return true;

    } catch (const std::ios_base::failure& e) {
        LOG_ERR("IO error reading text file %s: %s", path.c_str(), e.what());
        content.clear();
        lines_read = 0;
        return false;
    } catch (const std::exception& e) {
        LOG_ERR("Error reading text file %s: %s", path.c_str(), e.what());
        content.clear();
        lines_read = 0;
        return false;
    } catch (...) {
        LOG_ERR("Unknown error reading text file %s", path.c_str());
        content.clear();
        lines_read = 0;
        return false;
    }
}

bool is_valid_utf8_file(const std::string& path) {
    if (path.empty()) {
        LOG_ERR("Empty path provided to is_valid_utf8_file");
        return false;
    }

    try {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            LOG_ERR("Failed to open file for UTF-8 validation: %s", path.c_str());
            return false;
        }

        // 获取文件大小
        file.seekg(0, std::ios::end);
        std::streampos file_size = file.tellg();
        
        if (file_size == std::streampos(-1)) {
            LOG_ERR("Failed to get file size for UTF-8 validation: %s", path.c_str());
            return false;
        }

        // 空文件被认为是有效的 UTF-8
        if (file_size == 0) {
            return true;
        }

        // 限制检查文件大小（防止内存溢出）
        const size_t MAX_CHECK_SIZE = 10 * 1024 * 1024; // 10MB
        size_t size = static_cast<size_t>(file_size);
        if (size > MAX_CHECK_SIZE) {
            size = MAX_CHECK_SIZE;
            LOG_WRN("File too large for full UTF-8 validation, checking first %zu bytes: %s", 
                    MAX_CHECK_SIZE, path.c_str());
        }

        // 回到文件开始位置
        file.seekg(0, std::ios::beg);
        if (file.fail()) {
            LOG_ERR("Failed to seek to beginning for UTF-8 validation: %s", path.c_str());
            return false;
        }

        // 读取文件内容
        std::vector<unsigned char> buffer(size);
        file.read(reinterpret_cast<char*>(buffer.data()), size);
        
        if (file.fail() && !file.eof()) {
            LOG_ERR("Failed to read file for UTF-8 validation: %s", path.c_str());
            return false;
        }

        size_t bytes_read = static_cast<size_t>(file.gcount());

        // UTF-8 验证逻辑
        for (size_t i = 0; i < bytes_read; ) {
            unsigned char c = buffer[i];
            
            if (c < 0x80) {
                // ASCII 字符 (0xxxxxxx)
                i++;
            } else if ((c & 0xE0) == 0xC0) {
                // 2字节序列 (110xxxxx 10xxxxxx)
                if (i + 1 >= bytes_read) return false;
                if ((buffer[i + 1] & 0xC0) != 0x80) return false;
                // 检查过短编码
                if (c < 0xC2) return false;
                i += 2;
            } else if ((c & 0xF0) == 0xE0) {
                // 3字节序列 (1110xxxx 10xxxxxx 10xxxxxx)
                if (i + 2 >= bytes_read) return false;
                if ((buffer[i + 1] & 0xC0) != 0x80) return false;
                if ((buffer[i + 2] & 0xC0) != 0x80) return false;
                // 检查过短编码和代理对
                uint32_t codepoint = ((c & 0x0F) << 12) | 
                                   ((buffer[i + 1] & 0x3F) << 6) | 
                                   (buffer[i + 2] & 0x3F);
                if (codepoint < 0x800 || (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
                    return false;
                }
                i += 3;
            } else if ((c & 0xF8) == 0xF0) {
                // 4字节序列 (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
                if (i + 3 >= bytes_read) return false;
                if ((buffer[i + 1] & 0xC0) != 0x80) return false;
                if ((buffer[i + 2] & 0xC0) != 0x80) return false;
                if ((buffer[i + 3] & 0xC0) != 0x80) return false;
                // 检查过短编码和超出范围的码点
                uint32_t codepoint = ((c & 0x07) << 18) | 
                                   ((buffer[i + 1] & 0x3F) << 12) |
                                   ((buffer[i + 2] & 0x3F) << 6) | 
                                   (buffer[i + 3] & 0x3F);
                if (codepoint < 0x10000 || codepoint > 0x10FFFF) {
                    return false;
                }
                i += 4;
            } else {
                // 无效的UTF-8起始字节
                return false;
            }
        }

        return true;

    } catch (const std::ios_base::failure& e) {
        LOG_ERR("IO error during UTF-8 validation of file %s: %s", path.c_str(), e.what());
        return false;
    } catch (const std::exception& e) {
        LOG_ERR("Error during UTF-8 validation of file %s: %s", path.c_str(), e.what());
        return false;
    } catch (...) {
        LOG_ERR("Unknown error during UTF-8 validation of file %s", path.c_str());
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

std::pair<bool, std::string> execute_command(const std::string& command) {
    if (command.empty()) {
        LOG_ERR("Empty command provided to execute_command");
        return {false, "ERROR: Empty command"};
    }

    std::string result;

#ifdef _WIN32
    FILE* pipe = _popen(command.c_str(), "r");
#else
    FILE* pipe = popen(command.c_str(), "r");
#endif

    if (!pipe) {
        LOG_ERR("Failed to execute command: %s", command.c_str());
        return {false, "ERROR: Failed to execute command"};
    }

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

#ifdef _WIN32
    int exit_code = _pclose(pipe);
#else
    int exit_code = pclose(pipe);
#endif

    bool success = (exit_code == 0);
    if (!success) {
        LOG_WRN("Command exited with code %d: %s", exit_code, command.c_str());
    } else {
        LOG_DBG("Command executed successfully: %s", command.c_str());
    }

    return {success, result};
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
    if (str.empty()) {
        LOG_WRN("Empty string provided to safe_parse_json");
        return json();
    }

    try {
        return json::parse(str);
    } catch (const json::parse_error& e) {
        LOG_ERR("JSON parse error: %s (at position %zu)", e.what(), e.byte);
        return json();
    } catch (const std::exception& e) {
        LOG_ERR("Unexpected error parsing JSON: %s", e.what());
        return json();
    }
}

std::string format_json(const json& j) {
    try {
        return j.dump(2);
    } catch (const std::exception& e) {
        LOG_ERR("Error formatting JSON: %s", e.what());
        return "{}";
    }
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
    } else if (expected_type == "integer") {
        return value.is_number_integer();
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
        LOG_WRN("Empty schema provided for validation");
        return true;
    }

    if (args.is_null() && !schema.contains("required")) {
        return true;  // No arguments provided and none required
    }

    try {
        return validate_json_schema(args, schema);
    } catch (const std::exception& e) {
        LOG_ERR("Exception during argument validation: %s", e.what());
        return false;
    }
}
