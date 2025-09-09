#include "validate_utf8_file.h"
#include "../common/common_utils.h"
#include <filesystem>

namespace BuiltinTools {
namespace FileTools {

ToolDefinition getValidateUtf8FileDefinition() {
    return {
        "validate_utf8_file",
        {
            {"type", "function"},
            {"function", {
                {"name", "validate_utf8_file"},
                {"description", "UTF-8 encoding validation tool for text files. Verifies whether a specified file contains valid UTF-8 encoded content. Essential for text processing workflows, data validation, and ensuring compatibility with UTF-8-only tools. Primary use cases: 1) Pre-processing validation before using UTF-8-only text tools 2) Data import validation to ensure encoding compatibility 3) File conversion workflow validation 4) International text content verification 5) Web content encoding validation. Performs comprehensive UTF-8 validation including proper byte sequences, overlong encodings, and invalid code points. Supports files up to 10MB for validation performance."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"path", {{"type", "string"}, {"description", "Complete file path (absolute or relative) to the file to validate for UTF-8 encoding. Examples: './document.txt', '/var/log/application.log', 'C:/Data/content.csv'. Only regular files are supported."}}}
                    }},
                    {"required", {"path"}}
                }}
            }}
        }
    };
}

json executeValidateUtf8File(const json& args) {
    std::string path = args.value("path", "");

    std::string error_message;
    // 验证路径合法性
    if (!BuiltinTools::Utils::validatePath(path, error_message)) {
        LOG_ERR("validate_utf8_file: Path validation failed for '%s': %s", path.c_str(), error_message.c_str());
        return BuiltinTools::Utils::createErrorResponse(error_message);
    }

    try {
        std::filesystem::path fs_path = BuiltinTools::Utils::utf8ToPath(path);

        // 检查路径是否存在
        if (!std::filesystem::exists(fs_path)) {
            LOG_ERR("validate_utf8_file: File not found: %s", path.c_str());
            return BuiltinTools::Utils::createErrorResponse("File not found: " + path);
        }

        // 检查是否是目录而不是文件
        if (std::filesystem::is_directory(fs_path)) {
            LOG_ERR("validate_utf8_file: Path is a directory, not a file: %s", path.c_str());
            return BuiltinTools::Utils::createErrorResponse("Path is a directory, not a file: " + path);
        }

        // 检查是否是常规文件
        if (!std::filesystem::is_regular_file(fs_path)) {
            LOG_ERR("validate_utf8_file: Path is not a regular file: %s", path.c_str());
            return BuiltinTools::Utils::createErrorResponse("Path is not a regular file: " + path);
        }

        // 获取文件大小
        auto file_size = std::filesystem::file_size(fs_path);

        // 使用UTF-8验证函数检查文件
        bool is_utf8 = BuiltinTools::Utils::isValidUtf8File(path);
        LOG_INF("validate_utf8_file: Validated file '%s' (UTF-8: %s, size: %llu bytes)", path.c_str(), is_utf8 ? "yes" : "no", static_cast<unsigned long long>(file_size));

        // 构建结果
        json result = BuiltinTools::Utils::createSuccessResponse();
        result["is_utf8"] = is_utf8;
        result["path"] = BuiltinTools::Utils::sanitizeStringForJson(path);
        result["file_size"] = static_cast<int64_t>(file_size);

        return result;

    } catch (const std::filesystem::filesystem_error& e) {
        LOG_ERR("validate_utf8_file: Filesystem error for '%s': %s", path.c_str(), e.what());
        return BuiltinTools::Utils::createErrorResponse("Filesystem error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        LOG_ERR("validate_utf8_file: Error checking UTF-8 encoding for '%s': %s", path.c_str(), e.what());
        return BuiltinTools::Utils::createErrorResponse("Error checking UTF-8 encoding: " + std::string(e.what()));
    } catch (...) {
        LOG_ERR("validate_utf8_file: Unknown error occurred while checking UTF-8 encoding for '%s'", path.c_str());
        return BuiltinTools::Utils::createErrorResponse("Unknown error occurred while checking UTF-8 encoding");
    }
}

} // namespace FileTools
} // namespace BuiltinTools
