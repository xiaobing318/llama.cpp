#include "read_text_file.h"
#include "../common/common_utils.h"
#include <filesystem>

namespace BuiltinTools {
namespace FileTools {

ToolDefinition getReadTextFileDefinition() {
    return {
        "read_text_file",
        {
            {"type", "function"},
            {"function", {
                {"name", "read_text_file"},
                {"description", "Specialized UTF-8 text file reading utility for loading text documents with line range selection.  This tool is designed exclusively for UTF-8 encoded text files and will fail on non-UTF-8 files. Primary use cases: 1) Reading UTF-8 configuration files (.ini, .conf, .json, .yaml) 2) Loading UTF-8 source code files (.py, .js, .cpp, .h) 3) Processing UTF-8 log files (.log) 4) Reading UTF-8 documentation (.md, .rst, .txt) 5) Extracting specific line ranges from large UTF-8 text files for analysis. Does not read binary files or provide file metadata - focuses solely on UTF-8 text content extraction. IMPORTANT: Before using this tool, you must first use the built-in tool validate_utf8_file to verify whether the target file is in UTF-8 character encoding. If you need to know the total number of lines in the target file, use the built-in tool inspect_path to obtain the total number of lines in the target file.Workflow: validate_utf8_file → read_text_file or validate_utf8_file → inspect_path → read_text_file."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"path", {{"type", "string"}, {"description", "The full file path (absolute or relative) of the target UTF-8 text file. For example: './config.txt', '/var/log/app.log', 'C:/Documents/readme.md'. Only UTF-8 text files are supported; binary files will be rejected. Also, please carefully check that the full file path you are extracting contains no spaces."}}},
                        {"start_line", {{"type", "integer"}, {"description", "Starting line number for range reading (1-based indexing). Default 1 (first line). Use with end_line to read specific sections of large files."}}},
                        {"end_line", {{"type", "integer"}, {"description", "Ending line number for range reading (1-based indexing, inclusive). Default -1 (last line). Combined with start_line allows reading specific portions of files."}}}
                    }},
                    {"required", {"path"}}
                }}
            }}
        }
    };
}

json executeReadTextFile(const json& args) {
    // 从json参数中提取输入，如果有则赋值，反之则使用默认值
    std::string path = args.value("path", "");
    int start_line = args.value("start_line", 1);
    int end_line = args.value("end_line", -1);

    // 验证路径参数是否有效
    std::string error_message;
    if (!BuiltinTools::Utils::validatePath(path, error_message)) {
        return BuiltinTools::Utils::createErrorResponse(error_message);
    }

    // 验证行号参数（TODO:需要得到一个文件总行数然后做更精确的参数有效性验证）
    if (start_line < 1) {
        return BuiltinTools::Utils::createErrorResponse("start_line must be >= 1");
    }

    // end_line 可以是 -1（表示读到文件末尾），否则必须 >= start_line
    if (end_line != -1 && end_line < start_line) {
        return BuiltinTools::Utils::createErrorResponse("end_line must be >= start_line or -1 for end of file");
    }

    // 使用 filesystem 库进行更完整的路径和文件检查
    try {
        std::filesystem::path fs_path(path);

        // 检查路径是否存在
        if (!std::filesystem::exists(fs_path)) {
            return BuiltinTools::Utils::createErrorResponse("File not found: " + path);
        }

        // 检查是否是目录而不是文件
        if (std::filesystem::is_directory(fs_path)) {
            return BuiltinTools::Utils::createErrorResponse("Path is a directory, not a text file: " + path);
        }

        // 检查是否是常规文件
        if (!std::filesystem::is_regular_file(fs_path)) {
            return BuiltinTools::Utils::createErrorResponse("Path is not a regular file: " + path);
        }

        // 获取文件大小并检查是否过大
        auto file_size = std::filesystem::file_size(fs_path);
        // 100MB 限制
        const size_t MAX_FILE_SIZE = 100 * 1024 * 1024;
        // 超过限制则报错
        if (file_size > MAX_FILE_SIZE) {
            return BuiltinTools::Utils::createErrorResponse("File too large for text reading (maximum 100MB): " + std::to_string(file_size) + " bytes");
        }

        // 读取文本文件内容
        std::string content;
        int lines_read = 0;
        int actual_end_line = end_line;
        // 读取文件内容，失败则报错
        if (!BuiltinTools::Utils::readTextFileWithRange(path, start_line, end_line, content, lines_read, actual_end_line)) {
            return BuiltinTools::Utils::createErrorResponse("Failed to read UTF-8 text file. File may be binary or not UTF-8 encoded.");
        }

        // 对内容进行清理，确保JSON序列化安全
        std::string safe_content = BuiltinTools::Utils::sanitizeStringForJson(content);

        // 额外的安全措施：确保字符串长度合理，10MB限制
        if (safe_content.size() > 10 * 1024 * 1024) {
            // Content too large, truncating
            safe_content = safe_content.substr(0, 10 * 1024 * 1024) + "...[truncated]";
        }

        // 构建结果
        json result = BuiltinTools::Utils::createSuccessResponse();
        result["lines_read"] = lines_read;
        result["start_line"] = start_line;
        result["end_line"] = actual_end_line;
        result["path"] = BuiltinTools::Utils::sanitizeStringForJson(path);

        try {
            result["content"] = safe_content;
        } catch (const std::exception& e) {
            LOG_ERR("Failed to set content field: %s", e.what());
            result["content"] = "[content unavailable due to UTF-8 encoding issues]";
        }

        return result;

    } catch (const std::filesystem::filesystem_error& e) {
        return BuiltinTools::Utils::createErrorResponse("Filesystem error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return BuiltinTools::Utils::createErrorResponse("Error reading text file: " + std::string(e.what()));
    } catch (...) {
        return BuiltinTools::Utils::createErrorResponse("Unknown error occurred while reading text file");
    }
}

} // namespace FileTools
} // namespace BuiltinTools
