#include "grep.h"
#include "../common/common_utils.h"

#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <system_error>

// 根据不同的平台包含头文件
#if defined(_WIN32)
  #ifndef NOMINMAX
  #define NOMINMAX
  #endif
  #include <windows.h>
#endif

namespace BuiltinTools {
namespace SystemTools {

namespace fs = std::filesystem;

// 内部辅助函数
static inline bool nameStartsWithDot(const fs::path& p) {
    auto s = BuiltinTools::Utils::pathToUtf8String(p.filename());
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

// 获取 grep 工具的定义
ToolDefinition getGrepDefinition() {
    return {
        "grep",
        {
            {"type", "function"},
            {"function", {
                {"name", "grep"},
                {"description", "Search text in a file or directory tree with options for regex, case sensitivity, line numbers, recursion, filename filtering via glob, hidden-item visibility, symlink following, and a global match cap. Examples: args {'path':'README.md','pattern':'QCopilot'}; args {'path':'src','pattern':'TODO','recursive':true,'file_glob':'**/*.cpp'}; args {'path':'.','pattern':'class [A-Za-z_][A-Za-z0-9_]*','use_regex':true,'recursive':true,'max_matches':5000}. Returns: results[], matches_count, files_scanned, truncated."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"path",            {{"type","string"},  {"description","Target file or directory to search in."}}},
                        {"pattern",         {{"type","string"},  {"description","Search pattern; literal by default or regex when 'use_regex' is true."}}},
                        {"use_regex",       {{"type","boolean"}, {"description","Interpret 'pattern' as a regular expression. Default false."}, {"default", false}}},
                        {"case_sensitive",  {{"type","boolean"}, {"description","Case-sensitive search. Default true."}, {"default", true}}},
                        {"line_numbers",    {{"type","boolean"}, {"description","Include line numbers in results. Default true."}, {"default", true}}},
                        {"recursive",       {{"type","boolean"}, {"description","Recurse into subdirectories when 'path' is a directory. Default false."}, {"default", false}}},
                        {"file_glob",       {{"type","string"},  {"description","Glob pattern for selecting files when searching a directory; supports **, *, ?, [], \\\\ . Default is '*' or '**/*' depending on 'recursive'."}}},
                        {"follow_symlinks", {{"type","boolean"}, {"description","Follow directory symlinks during file discovery. Default false."}, {"default", false}}},
                        {"show_hidden",     {{"type","boolean"}, {"description","Include hidden files/directories during discovery. Default false."}, {"default", false}}},
                        {"max_matches",     {{"type","integer"}, {"description","Global soft cap on total matches across files; sets 'truncated=true' if reached. Default 10000."}, {"default", 10000}}}
                    }},
                    {"required", {"path", "pattern"}}
                }}
            }}
        }
    };
}

// 执行 grep 工具
json executeGrep(const json& args) {
    // 解析参数，设置默认值
    const std::string target_path    = args.value("path", "");
    const std::string pat            = args.value("pattern", "");
    const bool use_regex             = args.value("use_regex", false);
    const bool case_sensitive        = args.value("case_sensitive", true);
    const bool line_numbers          = args.value("line_numbers", true);
    const bool recursive             = args.value("recursive", false);
    const std::string file_glob_arg  = args.value("file_glob", "");
    const bool follow_symlinks       = args.value("follow_symlinks", false);
    const bool show_hidden           = args.value("show_hidden", false);
    const int  max_matches           = args.value("max_matches", 10000);

    // 参数校验
    if (pat.empty()) {
        json err = BuiltinTools::Utils::createErrorResponse("pattern is required");
        err["messages"] = json::array({"Missing 'pattern' argument"});
        return err;
    }
    std::string error_message;
    if (!BuiltinTools::Utils::validatePath(target_path, error_message)) {
        LOG_ERR("grep: Path validation failed for '%s': %s", target_path.c_str(), error_message.c_str());
        json err = BuiltinTools::Utils::createErrorResponse(error_message);
        err["path"] = target_path;
        err["pattern"] = pat;
        err["messages"] = json::array({"Target path validation failed"});
        return err;
    }
    if (!BuiltinTools::Utils::fileExists(target_path)) {
        LOG_ERR("grep: Path not found: %s", target_path.c_str());
        json err = BuiltinTools::Utils::createErrorResponse("Path not found: " + target_path);
        err["path"] = target_path;
        err["pattern"] = pat;
        err["messages"] = json::array({"Target path does not exist"});
        return err;
    }
    // 设置返回值的基础结构
    json result = BuiltinTools::Utils::createSuccessResponse();
    result["path"]            = target_path;
    result["pattern"]         = pat;
    result["use_regex"]       = use_regex;
    result["case_sensitive"]  = case_sensitive;
    result["line_numbers"]    = line_numbers;
    result["recursive"]       = recursive;
    result["follow_symlinks"] = follow_symlinks;
    result["show_hidden"]     = show_hidden;
    result["max_matches"]     = max_matches;

    std::vector<json> hits;       // 全部命中
    int total_matches = 0;        // 全局累计命中数
    int files_scanned = 0;        // 被扫描文件数
    bool truncated     = false;

    try {
        fs::path p = BuiltinTools::Utils::utf8ToPath(target_path);
        std::error_code ec;

        if (fs::is_regular_file(p, ec)) {
            // 直接在单个文件中搜索
            auto per_file = BuiltinTools::Utils::searchInFileRegex(
                p, pat, use_regex, case_sensitive, line_numbers, total_matches, max_matches
            );
            files_scanned = 1;
            if (!per_file.empty()) {
                // 直接拼接
                hits.insert(hits.end(),
                            std::make_move_iterator(per_file.begin()),
                            std::make_move_iterator(per_file.end()));
            }
            truncated = (total_matches >= max_matches);
        } else if (fs::is_directory(p, ec)) {
            // 目录：先发现要搜索的文件列表
            std::string file_glob = file_glob_arg;
            if (file_glob.empty()) {
                file_glob = recursive ? "**/*" : "*";
            }

            // 仅收集“文件”，不包含目录
            std::vector<fs::path> files = BuiltinTools::Utils::globFiles(
                p, file_glob, /*include_directories=*/false,
                /*follow_symlinks=*/follow_symlinks,
                /*case_sensitive=*/case_sensitive
            );

            // 可选过滤隐藏
            std::vector<fs::path> filtered;
            filtered.reserve(files.size());
            for (const auto& f : files) {
                if (!show_hidden && isHiddenCrossPlatform(f)) continue;
                filtered.push_back(f);
            }

            // 按序扫描，直到达到上限
            for (const auto& f : filtered) {
                if (total_matches >= max_matches) { truncated = true; break; }
                ++files_scanned;

                const int remaining = max_matches - total_matches;
                auto per_file = BuiltinTools::Utils::searchInFileRegex(
                    f, pat, use_regex, case_sensitive, line_numbers, total_matches, remaining
                );
                if (!per_file.empty()) {
                    hits.insert(hits.end(),
                                std::make_move_iterator(per_file.begin()),
                                std::make_move_iterator(per_file.end()));
                }
            }
        } else {
            json err = BuiltinTools::Utils::createErrorResponse("Unsupported path type for grep: " + target_path);
            err["path"] = target_path;
            err["pattern"] = pat;
            err["messages"] = json::array({"Only files or directories are supported"});
            return err;
        }
    } catch (const std::exception& e) {
        LOG_ERR("grep: Failed on '%s': %s", target_path.c_str(), e.what());
        json err = BuiltinTools::Utils::createErrorResponse("Failed to execute grep: " + std::string(e.what()));
        err["path"] = target_path;
        err["pattern"] = pat;
        err["messages"] = json::array({"Unexpected exception during grep"});
        return err;
    }

    result["results"]       = std::move(hits);
    result["matches_count"] = total_matches;
    result["files_scanned"] = files_scanned;
    // messages 汇总本次行为
    json messages = json::array();
    messages.push_back(recursive ? "Recursive mode enabled; scanned subdirectories" : "Recursive mode disabled; searched only the target path");
    if (!show_hidden) messages.push_back("Hidden items are filtered (dot entries on Unix; hidden/system on Windows)");
    else messages.push_back("Hidden items are included");
    if (use_regex) messages.push_back("Regex matching enabled");
    if (!case_sensitive) messages.push_back("Case-insensitive matching");
    if (truncated) {
        result["truncated"] = true;
        messages.push_back("Search results were truncated due to max_matches cap");
    }
    if (!messages.empty()) result["messages"] = std::move(messages);

    return result;
}

} // namespace SystemTools
} // namespace BuiltinTools
