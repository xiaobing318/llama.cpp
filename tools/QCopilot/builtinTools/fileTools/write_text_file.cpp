#include "write_text_file.h"
#include "fileTools_utils.h"
#include "../common/common_utils.h"

namespace BuiltinTools {
namespace FileTools {

ToolDefinition getWriteTextFileDefinition() {
    return {
        "write_text_file",
        {
            {"type", "function"},
            {"function", {
                {"name", "write_text_file"},
                {"description", "Specialized UTF-8 text file writing utility for creating and modifying UTF-8 encoded text files with robust encoding validation and error handling. IMPORTANT: For existing files, you MUST first use the validate_utf8_file built-in tool to verify UTF-8 encoding before writing. New files are automatically created with UTF-8 encoding. Single responsibility: writes UTF-8 text content only, does not read file metadata. Key applications: 1) Creating UTF-8 source code files (.py, .js, .cpp, .h, .sql) 2) Writing UTF-8 configuration files (.ini, .conf, .json, .yaml) 3) Generating UTF-8 documentation (.md, .rst, .txt) 4) Maintaining UTF-8 log files with append mode 5) Creating UTF-8 data files (.csv, .xml).  Workflow for existing files: validate_utf8_file → write_text_file. For new files: directly use write_text_file."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"path", {{"type", "string"}, {"description", "Target UTF-8 text file path (absolute or relative) where content will be written. Automatically creates parent directories if they don't exist. Examples: './config.txt', '/var/log/app.log', 'C:/Documents/readme.md'. Only UTF-8 text files are supported. Ensure the full file path contains no spaces for cross-platform compatibility."}}},
                        {"content", {{"type", "string"}, {"description", "UTF-8 encoded text content to write to the file. Supports plain text, source code, configuration syntax, structured data (JSON, CSV, XML), and documentation formats. Handles newlines and UTF-8 special characters appropriately."}}},
                        {"append", {{"type", "boolean"}, {"description", "Write mode selection: false (overwrite mode, default) completely replaces existing file content, ideal for generating new files and configuration updates; true (append mode) adds content to existing file end, perfect for log files, data collection, and incremental updates. For existing files in append mode, UTF-8 encoding validation is required."}}}
                    }},
                    {"required", {"path", "content"}}
                }}
            }}
        }
    };
}

json executeWriteTextFile(const json& args) {
    std::string path = args.value("path", "");
    std::string content = args.value("content", "");
    bool append = args.value("append", false);

    // 参数验证
    std::string error_message;
    if (!BuiltinTools::Utils::validatePath(path, error_message)) {
        LOG_ERR("write_text_file: Path validation failed for '%s': %s", path.c_str(), error_message.c_str());
        return BuiltinTools::Utils::createErrorResponse(error_message);
    }

    // 内容允许为空，但记录警告日志
    if (content.empty()) {
        LOG_WRN("write_text_file: Empty content provided for '%s'", path.c_str());
    }

    try {
        bool file_existed = BuiltinTools::Utils::fileExists(path);

        // 对于已存在的文件，记录日志提醒应先检查编码
        if (file_existed) {
            LOG_INF("write_text_file: Writing to existing file '%s'", path.c_str());
        } else {
            LOG_INF("write_text_file: Creating new UTF-8 text file '%s'", path.c_str());
        }

        bool write_success;
        if (append && file_existed) {
            // 追加模式：直接以追加方式打开文件
            write_success = BuiltinTools::Utils::appendFileContent(path, content);
        } else {
            // 覆盖模式或新文件：直接写入
            write_success = BuiltinTools::Utils::writeFileContent(path, content);
        }

        if (!write_success) {
            LOG_ERR("write_text_file: Failed to write content to file '%s'", path.c_str());
            return BuiltinTools::Utils::createErrorResponse("Failed to write file");
        }

        LOG_INF("write_text_file: Successfully wrote %zu bytes to file '%s'", content.size(), path.c_str());

        json result = BuiltinTools::Utils::createSuccessResponse();
        result["path"] = path;
        result["bytes_written"] = content.size();
        result["mode"] = append ? "append" : "overwrite";
        result["file_existed"] = file_existed;

        return result;

    } catch (const std::exception& e) {
        LOG_ERR("write_text_file: Unexpected error writing file '%s': %s", path.c_str(), e.what());
        return BuiltinTools::Utils::createErrorResponse("Unexpected error during file write: " + std::string(e.what()));
    }
}

} // namespace FileTools
} // namespace BuiltinTools
