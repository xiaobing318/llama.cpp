#include "list_directory.h"
#include "../common/filesystem_utils.h"
#include "../common/string_utils.h"
#include "../common/tool_response.h"

#include <filesystem>
#include <algorithm>
#include <vector>
#include <string>
#include <system_error>

#if defined(_WIN32)
  #ifndef NOMINMAX
  #define NOMINMAX
  #endif
  #include <windows.h>
#endif

namespace BuiltinTools {
namespace SystemTools {

constexpr const char* k_tool_name = "list_directory";

namespace common = builtin_tools::common;

// 获取 list_directory 工具的定义
ToolDefinition get_list_directory_definition() {
    // 单行 description，避免多行 JSON 文本问题
    return {
        "list_directory",
        {
            {"type", "function"},
            {"function", {
                {"name", "list_directory"},
                {"description", "List contents of a directory with optional recursion, hidden-item visibility, and size reporting. Cross-platform (Windows/Linux). Use cases: quick inventory, pre-check before heavy operations. Parameters: path (required), recursive (bool, default false), show_hidden (bool, default false), include_size (bool, default true), max_results (int, soft cap, default 50000). Returns: files[], count, truncated flag."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"path",         {{"type","string"},  {"description","Directory to list (absolute or relative)."}}},
                        {"recursive",    {{"type","boolean"}, {"description","Recurse into subdirectories. Default false."}, {"default", false}}},
                        {"show_hidden",  {{"type","boolean"}, {"description","Include hidden files/directories. Default false."}, {"default", false}}},
                        {"include_size", {{"type","boolean"}, {"description","Include size for regular files (adds 'size' and 'human_size'). Default true."}, {"default", true}}},
                        {"max_results",  {{"type","integer"}, {"description","Soft limit of returned entries; result sets 'truncated=true' if reached. Default 50000."}, {"default", 50000}}}
                    }},
                    {"required", {"path"}}
                }}
            }}
        }
    };
}

// 内部辅助函数：判断是否为以点开头的隐藏文件名
static inline bool isDotHiddenName(const std::filesystem::path& p) {
    auto name = common::path_to_utf8_string(p.filename());
    return !name.empty() && name[0] == '.';
}

// Windows 平台下的隐藏文件判断
#if defined(_WIN32)
static bool isHiddenWin(const std::filesystem::path& p) {
    // 使用宽字符以兼容非 ASCII 路径
    std::wstring ws = p.wstring();
    DWORD attrs = GetFileAttributesW(ws.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) return false;
    // HIDDEN 或 SYSTEM 都视为“隐藏”
    return (attrs & FILE_ATTRIBUTE_HIDDEN) || (attrs & FILE_ATTRIBUTE_SYSTEM);
}
#endif

// 跨平台隐藏文件判断
static bool isHiddenCrossPlatform(const std::filesystem::path& p) {
#if defined(_WIN32)
    // Windows：使用文件属性；另外也兼容以 “.” 开头的约定
    return isHiddenWin(p) || isDotHiddenName(p);
#else
    // Linux/Unix：以 “.” 开头
    return isDotHiddenName(p);
#endif
}

// 内部辅助函数：尝试获取文件大小
static bool tryGetFileSize(const std::filesystem::path& p, uint64_t& out_size) {
    std::error_code ec;
    auto sz = std::filesystem::file_size(p, ec);
    if (ec) return false;
    out_size = static_cast<uint64_t>(sz);
    return true;
}

// 执行 list_directory 工具
json run_list_directory(const json& args) {

    // 解析参数，设置默认参数
    const std::string path = args.value("path", ".");
    const bool recursive = args.value("recursive", false);
    const bool show_hidden = args.value("show_hidden", false);
    const bool include_size = args.value("include_size", true);
    const int  max_results = args.value("max_results", 50000);

    // 基础校验
    std::string error_message;
    if (!common::validate_existing_path(path, error_message)) {
        LOG_ERR("list_directory: Path validation failed for '%s': %s", path.c_str(), error_message.c_str());
        return common::make_error(k_tool_name, error_message);
    }
    if (!common::file_exists(path)) {
        LOG_ERR("list_directory: Directory not found: %s", path.c_str());
        return common::make_error(k_tool_name, "Directory not found: " + path);
    }
    if (!std::filesystem::is_directory(common::utf8_to_path(path))) {
        LOG_ERR("list_directory: Path is not a directory: %s", path.c_str());
        return common::make_error(k_tool_name, "Path is not a directory: " + path);
    }

    // 枚举选项：跳过权限拒绝；不跟随符号链接（避免递归环）
    const auto opts = std::filesystem::directory_options::skip_permission_denied;

    std::vector<json> out_items;
    out_items.reserve(1024);

    bool truncated = false;
    size_t produced = 0;

    // 封装一次性处理逻辑
    auto process_entry = [&](const std::filesystem::directory_entry& entry,
                             const std::filesystem::path& root) -> void {
        if (produced >= static_cast<size_t>(max_results)) {
            truncated = true;
            return;
        }

        // 非抛异常方式获取状态，减少 I/O 异常影响
        std::error_code ec;
        const auto st = entry.symlink_status(ec);
        if (ec) return;

        const auto p  = entry.path();
        const bool is_dir  = std::filesystem::is_directory(st);
        const bool is_file = std::filesystem::is_regular_file(st);
        const bool is_sym  = std::filesystem::is_symlink(st);

        // 隐藏项处理（对文件 & 目录都生效）
        if (!show_hidden && isHiddenCrossPlatform(p)) {
            return;
        }

        json item = {
            {"name", common::path_to_utf8_string(p.filename())},
            {"path", common::path_to_utf8_string(p)},
            {"type", is_dir ? "directory" : (is_file ? "file" : (is_sym ? "symlink" : "other"))},
            {"is_hidden", isHiddenCrossPlatform(p)}
        };

        if (include_size && is_file) {
            uint64_t sz = 0;
            if (tryGetFileSize(p, sz)) {
                item["size"] = sz;
                item["human_size"] = common::format_file_size(sz);
            } else {
                // 文件大小不可读时给个占位
                item["size"] = 0;
                item["human_size"] = "0 B";
            }
        }

        out_items.push_back(std::move(item));
        ++produced;
    };

    try {
        if (recursive) {
            // 递归枚举：对隐藏目录的“剪枝”也要生效
            for (std::filesystem::recursive_directory_iterator it(common::utf8_to_path(path), opts), end; it != end; ++it) {
                if (produced >= static_cast<size_t>(max_results)) { truncated = true; break; }

                // 如果遇到隐藏目录且未开启 show_hidden，则跳过并阻止深入
                if (!show_hidden) {
                    std::error_code ec;
                    if (it->is_directory(ec) && !ec && isHiddenCrossPlatform(it->path())) {
                        it.disable_recursion_pending();
                        continue;
                    }
                }

                process_entry(*it, common::utf8_to_path(path));
            }
            // 提示递归行为
            out_items.size(); // no-op keep
        } else {
            for (std::filesystem::directory_iterator it(common::utf8_to_path(path), opts), end; it != end; ++it) {
                if (produced >= static_cast<size_t>(max_results)) { truncated = true; break; }
                process_entry(*it, common::utf8_to_path(path));
            }
        }
    } catch (const std::exception& e) {
        LOG_ERR("list_directory: Failed to list '%s': %s", path.c_str(), e.what());
        return common::make_error(k_tool_name, "Failed to list directory: " + std::string(e.what()));
    }

    // 组织返回
    json result = common::make_success(k_tool_name);
    result["path"] = path;
    result["recursive"] = recursive;
    result["show_hidden"] = show_hidden;
    result["include_size"] = include_size;
    result["max_results"] = max_results;
    result["files"] = std::move(out_items);
    result["count"] = result["files"].size();
    // 装配提示信息
    json messages = json::array();
    if (recursive) messages.push_back("Recursive mode enabled; scanned subdirectories");
    else messages.push_back("Recursive mode disabled; returned top-level entries only");
    if (!show_hidden) messages.push_back("Hidden items are filtered (dot entries on Unix; hidden/system on Windows)");
    else messages.push_back("Hidden items are included");
    if (truncated) {
        result["truncated"] = true;
        messages.push_back("Directory listing was truncated due to max_results cap");
    }
    if (!messages.empty()) result["messages"] = std::move(messages);

    return result;
}

} // namespace SystemTools
} // namespace BuiltinTools
