#include "validate_utf8_file.h"
#include "../common/encoding_utils.h"
#include "../common/filesystem_utils.h"
#include "../common/string_utils.h"
#include "../common/tool_response.h"
#include <filesystem>

namespace BuiltinTools {
namespace FileTools {

constexpr const char* k_tool_name = "validate_utf8_file";

namespace common = builtin_tools::common;

ToolDefinition get_validate_utf8_file_definition() {
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

json run_validate_utf8_file(const json& args) {
    std::string path = args.value("path", "");

    std::string error_message;
    // 验证路径合法性
    if (!common::validate_existing_path(path, error_message)) {
        LOG_ERR("validate_utf8_file: Path validation failed for '%s': %s", path.c_str(), error_message.c_str());
        json err = common::make_error(k_tool_name, error_message);
        err["path"] = path;
        err["messages"] = json::array({"Path validation failed; please check illegal traversal or length"});
        return err;
    }

    try {
        std::filesystem::path fs_path = common::utf8_to_path(path);

        // 检查路径是否存在
        if (!std::filesystem::exists(fs_path)) {
            LOG_ERR("validate_utf8_file: File not found: %s", path.c_str());
            json err = common::make_error(k_tool_name, "File not found: " + path);
            err["path"] = path;
            err["messages"] = json::array({"Path does not exist"});
            return err;
        }

        // 检查是否是目录而不是文件
        if (std::filesystem::is_directory(fs_path)) {
            LOG_ERR("validate_utf8_file: Path is a directory, not a file: %s", path.c_str());
            json err = common::make_error(k_tool_name, "Path is a directory, not a file: " + path);
            err["path"] = path;
            err["messages"] = json::array({"Expected a regular file but got a directory"});
            return err;
        }

        // 检查是否是常规文件
        if (!std::filesystem::is_regular_file(fs_path)) {
            LOG_ERR("validate_utf8_file: Path is not a regular file: %s", path.c_str());
            json err = common::make_error(k_tool_name, "Path is not a regular file: " + path);
            err["path"] = path;
            err["messages"] = json::array({"Only regular files are supported"});
            return err;
        }

        // 获取文件大小
        auto file_size = std::filesystem::file_size(fs_path);

        // 使用UTF-8验证函数检查文件
        bool is_utf8 = common::is_valid_utf8_file(path);
        LOG_INF("validate_utf8_file: Validated file '%s' (UTF-8: %s, size: %llu bytes)", path.c_str(), is_utf8 ? "yes" : "no", static_cast<unsigned long long>(file_size));

        // 构建结果
        json result = common::make_success(k_tool_name);
        result["is_utf8"] = is_utf8;
        result["path"] = common::sanitize_string_for_json(path);
        result["file_size"] = static_cast<int64_t>(file_size);
        if (!is_utf8) {
            result["messages"] = json::array({"File content is not valid UTF-8"});
        }

        return result;

    } catch (const std::filesystem::filesystem_error& e) {
        LOG_ERR("validate_utf8_file: Filesystem error for '%s': %s", path.c_str(), e.what());
        json err = common::make_error(k_tool_name, "Filesystem error: " + std::string(e.what()));
        err["path"] = path;
        err["messages"] = json::array({"Filesystem exception occurred while reading file metadata"});
        return err;
    } catch (const std::exception& e) {
        LOG_ERR("validate_utf8_file: Error checking UTF-8 encoding for '%s': %s", path.c_str(), e.what());
        json err = common::make_error(k_tool_name, "Error checking UTF-8 encoding: " + std::string(e.what()));
        err["path"] = path;
        err["messages"] = json::array({"Unexpected exception during UTF-8 validation"});
        return err;
    } catch (...) {
        LOG_ERR("validate_utf8_file: Unknown error occurred while checking UTF-8 encoding for '%s'", path.c_str());
        json err = common::make_error(k_tool_name, "Unknown error occurred while checking UTF-8 encoding");
        err["path"] = path;
        err["messages"] = json::array({"Unknown error during UTF-8 validation"});
        return err;
    }
}

} // namespace FileTools
} // namespace BuiltinTools
