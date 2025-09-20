#include "glob.h"
#include "../common/filesystem_utils.h"
#include "../common/tool_response.h"

#include <filesystem>
#include <vector>
#include <algorithm>
#include <system_error>
// 根据平台包含隐藏文件检测所需头文件
#if defined(_WIN32)
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>
#endif

namespace BuiltinTools {
namespace SystemTools {

constexpr const char* k_tool_name = "glob";

namespace common = builtin_tools::common;

namespace fs = std::filesystem;

// 内部辅助函数
static inline bool nameStartsWithDot(const fs::path& p) {
    auto s = common::path_to_utf8_string(p.filename());
    return !s.empty() && s[0] == '.';
}
// 内部辅助函数：跨平台隐藏项检测
#if defined(_WIN32)
static bool isHiddenWin(const fs::path& p) {
    std::wstring ws = p.wstring();
    DWORD attrs = GetFileAttributesW(ws.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) return false;
    return (attrs & FILE_ATTRIBUTE_HIDDEN) || (attrs & FILE_ATTRIBUTE_SYSTEM);
}
#endif
static bool isHiddenCrossPlatform(const fs::path& p) {
#if defined(_WIN32)
    return isHiddenWin(p) || nameStartsWithDot(p);
#else
    return nameStartsWithDot(p);
#endif
}

// 获取 glob 工具的定义
ToolDefinition get_glob_definition() {
    return {
        "glob",
        {
            {"type", "function"},
            {"function", {
                {"name", "glob"},
                {"description", "Expand shell-like patterns under a base directory with options for including directories, following symlinks, case sensitivity, hidden-item visibility, and a soft result cap. Patterns support segments like '*', '?', '[]', '\\' and the recursive '**'. Examples: args {'base_dir':'.','pattern':'**/*.cpp'}; args {'base_dir':'src','pattern':'**','include_directories':true}; args {'base_dir':'data','pattern':'*.csv','case_sensitive':false,'show_hidden':true}. Returns: items[], count, truncated."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"base_dir",            {{"type","string"},  {"description","Root directory to expand the pattern from (absolute or relative)."}}},
                        {"pattern",             {{"type","string"},  {"description","Glob pattern supporting **, *, ?, [], \\\\ ."}}},
                        {"include_directories", {{"type","boolean"}, {"description","If true, include directories in results; otherwise only files. Default false."}, {"default", false}}},
                        {"follow_symlinks",     {{"type","boolean"}, {"description","Follow directory symlinks during traversal. Default false."}, {"default", false}}},
                        {"case_sensitive",      {{"type","boolean"}, {"description","Case-sensitive matching for pattern segments. Default true."}, {"default", true}}},
                        {"show_hidden",         {{"type","boolean"}, {"description","Include hidden files/directories (Windows hidden/System and dot-prefixed). Default false."}, {"default", false}}},
                        {"max_results",         {{"type","integer"}, {"description","Soft cap on number of returned items; set 'truncated=true' if reached. Default 50."}, {"default", 50}}}
                    }},
                    {"required", {"base_dir", "pattern"}}
                }}
            }}
        }
    };
}

// 执行 glob 工具
json run_glob(const json& args) {
    // 解析参数，设置默认值
    const std::string base_dir        = args.value("base_dir", ".");
    const std::string pattern         = args.value("pattern", "");
    const bool include_directories    = args.value("include_directories", false);
    const bool follow_symlinks        = args.value("follow_symlinks", false);
    const bool case_sensitive         = args.value("case_sensitive", true);
    const bool show_hidden            = args.value("show_hidden", false);
    const int  max_results            = args.value("max_results", 50);

    // 创建一个错误消息变量
    std::string error_message;
    // 参数验证
    if (!common::validate_existing_path(base_dir, error_message)) {
        LOG_ERR("glob: Path validation failed for '%s': %s", base_dir.c_str(), error_message.c_str());
        json err = common::make_error(k_tool_name, error_message);
        err["base_dir"] = base_dir;
        err["pattern"] = pattern;
        err["messages"] = json::array({"Base directory validation failed"});
        return err;
    }
    if (!common::file_exists(base_dir)) {
        LOG_ERR("glob: Base directory not found: %s", base_dir.c_str());
        json err = common::make_error(k_tool_name, "Base directory not found: " + base_dir);
        err["base_dir"] = base_dir;
        err["pattern"] = pattern;
        err["messages"] = json::array({"Base directory does not exist"});
        return err;
    }
    if (!fs::is_directory(common::utf8_to_path(base_dir))) {
        LOG_ERR("glob: Base path is not a directory: %s", base_dir.c_str());
        json err = common::make_error(k_tool_name, "Base path is not a directory: " + base_dir);
        err["base_dir"] = base_dir;
        err["pattern"] = pattern;
        err["messages"] = json::array({"Expected a directory"});
        return err;
    }
    if (pattern.empty()) {
        return common::make_error(k_tool_name, "Pattern is required");
    }

    // 利用 Utils::glob_paths 做主匹配，这一步不过滤隐藏与设置上限
    std::vector<fs::path> paths = common::glob_paths(
        common::utf8_to_path(base_dir),
        pattern,
        /*include_directories=*/include_directories,
        /*follow_symlinks=*/follow_symlinks,
        /*case_sensitive=*/case_sensitive
    );

    // 过滤隐藏 & 封顶 & 构造结果
    std::vector<json> items;
    items.reserve(std::min<int>(static_cast<int>(paths.size()), std::max(0, max_results)));
    // 是否截断
    bool truncated = false;
    // 已经处理过的数量
    int produced = 0;
    // 遍历所有匹配路径
    for (const auto& p : paths) {
        // 如果不显示隐藏且是隐藏项则跳过
        if (!show_hidden && isHiddenCrossPlatform(p)) {
            continue;
        }
        // 计算类型（不抛异常）
        std::error_code ec;
        auto st = fs::symlink_status(p, ec);
        std::string tp = "other";
        if (!ec) {
            if (fs::is_directory(st))      tp = "directory";
            else if (fs::is_regular_file(st)) tp = "file";
            else if (fs::is_symlink(st))   tp = "symlink";
        }
        // 添加结果项
        items.push_back(json{
            {"name", common::path_to_utf8_string(p.filename())},
            {"path", common::path_to_utf8_string(p)},
            {"type", tp},
            {"is_hidden", isHiddenCrossPlatform(p)}
        });
        // 增加计数，检查是否达到上限
        ++produced;
        if (produced >= max_results) { truncated = true; break; }
    }
    // 构造返回值
    json result = common::make_success(k_tool_name);
    result["base_dir"]          = base_dir;
    result["pattern"]           = pattern;
    result["include_directories"]= include_directories;
    result["follow_symlinks"]   = follow_symlinks;
    result["case_sensitive"]    = case_sensitive;
    result["show_hidden"]       = show_hidden;
    result["max_results"]       = max_results;
    result["items"]             = std::move(items);
    result["count"]             = result["items"].size();
    // messages 说明本次行为
    json messages = json::array();
    messages.push_back(include_directories ? "Directories included in results" : "Only files are returned");
    if (!show_hidden) messages.push_back("Hidden items are filtered (dot entries on Unix; hidden/system on Windows)");
    else messages.push_back("Hidden items are included");
    if (follow_symlinks) messages.push_back("Following directory symlinks during traversal");
    if (!case_sensitive) messages.push_back("Case-insensitive matching");
    if (truncated) {
        result["truncated"] = true;
        messages.push_back("Glob results were truncated due to max_results cap");
    }
    if (!messages.empty()) result["messages"] = std::move(messages);
    // 返回结果
    return result;
}

} // namespace SystemTools
} // namespace BuiltinTools
