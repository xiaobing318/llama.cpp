#include "glob.h"
#include "systemTools_utils.h"
#include "../common/common_utils.h"

#include <filesystem>
#include <vector>
#include <algorithm>
#include <system_error>

#if defined(_WIN32)
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>
#endif

namespace BuiltinTools {
namespace SystemTools {

namespace fs = std::filesystem;

// 隐藏项检测（跨平台）
static inline bool nameStartsWithDot(const fs::path& p) {
    auto s = p.filename().string();
    return !s.empty() && s[0] == '.';
}
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

// 工具定义
ToolDefinition getGlobDefinition() {
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
                        {"max_results",         {{"type","integer"}, {"description","Soft cap on number of returned items; set 'truncated=true' if reached. Default 50000."}, {"default", 50000}}}
                    }},
                    {"required", {"base_dir", "pattern"}}
                }}
            }}
        }
    };
}

// 主执行逻辑
json executeGlob(const json& args) {
    const std::string base_dir        = args.value("base_dir", ".");
    const std::string pattern         = args.value("pattern", "");
    const bool include_directories    = args.value("include_directories", false);
    const bool follow_symlinks        = args.value("follow_symlinks", false);
    const bool case_sensitive         = args.value("case_sensitive", true);
    const bool show_hidden            = args.value("show_hidden", false);
    const int  max_results            = args.value("max_results", 50000);

    // 基础参数校验
    std::string error_message;
    if (!BuiltinTools::Utils::validatePath(base_dir, error_message)) {
        LOG_ERR("glob: Path validation failed for '%s': %s", base_dir.c_str(), error_message.c_str());
        return BuiltinTools::Utils::createErrorResponse(error_message);
    }
    if (!BuiltinTools::Utils::fileExists(base_dir)) {
        LOG_ERR("glob: Base directory not found: %s", base_dir.c_str());
        return BuiltinTools::Utils::createErrorResponse("Base directory not found: " + base_dir);
    }
    if (!fs::is_directory(base_dir)) {
        LOG_ERR("glob: Base path is not a directory: %s", base_dir.c_str());
        return BuiltinTools::Utils::createErrorResponse("Base path is not a directory: " + base_dir);
    }
    if (pattern.empty()) {
        return BuiltinTools::Utils::createErrorResponse("Pattern is required");
    }

    // 利用 Utils::globFiles 做主匹配（不过滤隐藏与上限）
    std::vector<fs::path> paths = BuiltinTools::Utils::globFiles(
        base_dir,
        pattern,
        /*include_directories=*/include_directories,
        /*follow_symlinks=*/follow_symlinks,
        /*case_sensitive=*/case_sensitive
    );

    // 过滤隐藏 & 封顶 & 构造结果
    std::vector<json> items;
    items.reserve(std::min<int>(static_cast<int>(paths.size()), std::max(0, max_results)));

    bool truncated = false;
    int produced = 0;

    for (const auto& p : paths) {
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

        items.push_back(json{
            {"name", p.filename().string()},
            {"path", p.string()},
            {"type", tp},
            {"is_hidden", isHiddenCrossPlatform(p)}
        });

        ++produced;
        if (produced >= max_results) { truncated = true; break; }
    }

    json result = BuiltinTools::Utils::createSuccessResponse();
    result["base_dir"]          = base_dir;
    result["pattern"]           = pattern;
    result["include_directories"]= include_directories;
    result["follow_symlinks"]   = follow_symlinks;
    result["case_sensitive"]    = case_sensitive;
    result["show_hidden"]       = show_hidden;
    result["max_results"]       = max_results;
    result["items"]             = std::move(items);
    result["count"]             = result["items"].size();
    if (truncated) result["truncated"] = true;

    return result;
}

} // namespace SystemTools
} // namespace BuiltinTools
