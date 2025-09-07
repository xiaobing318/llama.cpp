#include "list_directory.h"
#include "systemTools_utils.h"
#include "../common/common_utils.h"
#include <filesystem>
#include <algorithm>
#include <set>
#include <iomanip>
#include <sstream>

namespace BuiltinTools {
namespace SystemTools {

ToolDefinition getListDirectoryDefinition() {
    return {
        "list_directory",
        {
            {"type", "function"},
            {"function", {
                {"name", "list_directory"},
                {"description", "Comprehensive directory enumeration utility with recursion, hidden-item visibility, kind filtering (files/dirs), extension allow-list, sorting, pagination, and optional size/timestamp enrichment. Ideal for project inventories, build prep, housekeeping (largest/oldest), packaging manifests, and pre-filters before heavier steps. Capabilities: (1) Single folder or full subtree traversal (2) Filter to 'files' or 'dirs' via 'kinds' (3) Restrict by extensions (e.g., 'cpp,h,py') via 'ext_filter' (4) Sort by 'name'/'size'/'modified' with 'asc'/'desc' (5) Paginate using 'offset' and 'limit' (6) Soft cap via 'max_results' and 'truncated=true' when reached (7) Optional size and human-readable sizes, plus modified time. Examples: args {'path':'.','kinds':'files','ext_filter':'cpp,h'} (list C/C++ sources in cwd); args {'path':'data','recursive':true,'sort_by':'size','order':'desc','limit':100} (top 100 largest under data); args {'path':'.','show_hidden':true,'kinds':'dirs'} (include hidden directories); args {'path':'assets','sort_by':'modified','order':'desc','offset':50,'limit':25} (paged recent items)."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"path",        {{"type","string"},  {"description","Directory to list. Absolute or relative."}}},
                        {"recursive",   {{"type","boolean"}, {"description","Recurse into subdirectories. Default false."}, {"default", false}}},
                        {"show_hidden", {{"type","boolean"}, {"description","Include entries whose names start with '.'. Default false."}, {"default", false}}},
                        {"kinds",       {{"type","string"},  {"enum", {"all","files","dirs"}}, {"description","Filter by kind: 'all' | 'files' | 'dirs'. Default 'all'."}, {"default","all"}}},
                        {"ext_filter",  {{"type","string"},  {"description","Comma-separated extension allow-list (without dots), e.g., 'cpp,h,py'. Applies to files only."}}},
                        {"size_info",   {{"type","boolean"}, {"description","Include 'size' and human-readable size for files. Default true."}, {"default", true}}},
                        {"sort_by",     {{"type","string"},  {"enum", {"name","size","modified"}}, {"description","Sort field. Default 'name'."}, {"default","name"}}},
                        {"order",       {{"type","string"},  {"enum", {"asc","desc"}}, {"description","Sort order. Default 'asc'."}, {"default","asc"}}},
                        {"limit",       {{"type","integer"}, {"description","Return at most this many items (pagination). Default 0 = no explicit limit."}, {"default", 0}}},
                        {"offset",      {{"type","integer"}, {"description","Skip this many items before returning (pagination). Default 0."}, {"default", 0}}},
                        {"max_results", {{"type","integer"}, {"description","Internal soft cap during enumeration; sets 'truncated=true' if reached. Default 50000."}, {"default", 50000}}}
                    }},
                    {"required", {"path"}}
                }}
            }}
        }
    };
}

json executeListDirectory(const json& args) {
    std::string path = args.value("path", ".");
    bool recursive = args.value("recursive", false);
    bool show_hidden = args.value("show_hidden", false);
    std::string kinds = args.value("kinds", "all");
    std::string ext_filter = args.value("ext_filter", "");
    bool size_info = args.value("size_info", true);
    std::string sort_by = args.value("sort_by", "name");
    std::string order = args.value("order", "asc");
    int limit = args.value("limit", 0);
    int offset = args.value("offset", 0);
    int max_results = args.value("max_results", 50000);

    // 参数验证
    std::string error_message;
    if (!Utils::validateSystemPath(path, error_message)) {
        LOG_ERR("list_directory: Path validation failed for '%s': %s", path.c_str(), error_message.c_str());
        return BuiltinTools::Utils::createErrorResponse(error_message);
    }

    if (!BuiltinTools::Utils::fileExists(path)) {
        LOG_ERR("list_directory: Directory not found: %s", path.c_str());
        return BuiltinTools::Utils::createErrorResponse("Directory not found: " + path);
    }

    if (!std::filesystem::is_directory(path)) {
        LOG_ERR("list_directory: Path is not a directory: %s", path.c_str());
        return BuiltinTools::Utils::createErrorResponse("Path is not a directory: " + path);
    }

    std::vector<json> file_list;

    // 解析扩展名过滤器
    std::set<std::string> allowed_exts;
    if (!ext_filter.empty()) {
        auto exts = BuiltinTools::Utils::splitString(ext_filter, ',');
        for (auto& ext : exts) {
            std::string trimmed = BuiltinTools::Utils::trimString(ext);
            if (!trimmed.empty()) {
                if (trimmed[0] != '.') trimmed = "." + trimmed;
                allowed_exts.insert(trimmed);
            }
        }
    }

    try {
        auto process_entry = [&](const std::filesystem::directory_entry& entry) {
            if (file_list.size() >= static_cast<size_t>(max_results)) {
                return false; // 达到上限
            }

            std::string filename = entry.path().filename().string();

            // 隐藏文件过滤
            if (!show_hidden && !filename.empty() && filename[0] == '.') {
                return true; // 继续
            }

            // 类型过滤
            bool is_dir = entry.is_directory();
            bool is_file = entry.is_regular_file();

            if (kinds == "files" && !is_file) return true;
            if (kinds == "dirs" && !is_dir) return true;

            // 扩展名过滤（仅对文件）
            if (!allowed_exts.empty() && is_file) {
                std::string ext = entry.path().extension().string();
                if (allowed_exts.find(ext) == allowed_exts.end()) {
                    return true;
                }
            }

            json file_info = {
                {"name", filename},
                {"path", entry.path().string()},
                {"type", is_dir ? "directory" : "file"}
            };

            if (size_info && is_file) {
                try {
                    auto file_size = std::filesystem::file_size(entry);
                    file_info["size"] = file_size;
                    file_info["human_size"] = Utils::formatFileSize(file_size);
                } catch (const std::exception& e) {
                    LOG_WRN("list_directory: Failed to get file size for '%s': %s", entry.path().string().c_str(), e.what());
                    file_info["size"] = 0;
                    file_info["human_size"] = "0 B";
                }

                try {
                    auto ftime = std::filesystem::last_write_time(entry);
                    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        ftime - std::filesystem::file_time_type::clock::now() +
                        std::chrono::system_clock::now()
                    );
                    auto time_t = std::chrono::system_clock::to_time_t(sctp);

                    file_info["modified"] = Utils::formatTimeStamp(sctp);
                    file_info["modified_timestamp"] = time_t;
                } catch (const std::exception& e) {
                    LOG_WRN("list_directory: Failed to get modification time for '%s': %s", entry.path().string().c_str(), e.what());
                    file_info["modified"] = "";
                    file_info["modified_timestamp"] = 0;
                }
            }

            file_list.push_back(file_info);
            return true; // 继续
        };

        bool truncated = false;

        if (recursive) {
            for (auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                if (!process_entry(entry)) {
                    truncated = true;
                    break;
                }
            }
        } else {
            for (auto& entry : std::filesystem::directory_iterator(path)) {
                if (!process_entry(entry)) {
                    truncated = true;
                    break;
                }
            }
        }

        // 排序
        if (sort_by == "name") {
            std::sort(file_list.begin(), file_list.end(),
                [&order](const json& a, const json& b) {
                    bool less = a["name"].get<std::string>() < b["name"].get<std::string>();
                    return order == "asc" ? less : !less;
                });
        } else if (sort_by == "size" && size_info) {
            std::sort(file_list.begin(), file_list.end(),
                [&order](const json& a, const json& b) {
                    uint64_t size_a = a.contains("size") ? a["size"].get<uint64_t>() : 0;
                    uint64_t size_b = b.contains("size") ? b["size"].get<uint64_t>() : 0;
                    bool less = size_a < size_b;
                    return order == "asc" ? less : !less;
                });
        } else if (sort_by == "modified" && size_info) {
            std::sort(file_list.begin(), file_list.end(),
                [&order](const json& a, const json& b) {
                    int64_t time_a = a.contains("modified_timestamp") ? a["modified_timestamp"].get<int64_t>() : 0;
                    int64_t time_b = b.contains("modified_timestamp") ? b["modified_timestamp"].get<int64_t>() : 0;
                    bool less = time_a < time_b;
                    return order == "asc" ? less : !less;
                });
        }

        // 分页
        std::vector<json> paged_list;
        if (limit > 0 || offset > 0) {
            size_t start = static_cast<size_t>(offset);
            size_t end = limit > 0 ? start + static_cast<size_t>(limit) : file_list.size();

            for (size_t i = start; i < std::min(end, file_list.size()); ++i) {
                paged_list.push_back(file_list[i]);
            }
        } else {
            paged_list = file_list;
        }

        // 清理不需要的 modified_timestamp 字段
        for (auto& item : paged_list) {
            if (item.contains("modified_timestamp")) {
                item.erase("modified_timestamp");
            }
        }

        json result = BuiltinTools::Utils::createSuccessResponse();
        result["path"] = path;
        result["recursive"] = recursive;
        result["show_hidden"] = show_hidden;
        result["kinds"] = kinds;
        result["files"] = paged_list;
        result["count"] = paged_list.size();

        if (truncated) {
            result["truncated"] = true;
        }

        return result;

    } catch (const std::exception& e) {
        LOG_ERR("list_directory: Failed to list directory '%s': %s", path.c_str(), e.what());
        return BuiltinTools::Utils::createErrorResponse("Failed to list directory: " + std::string(e.what()));
    }
}

} // namespace SystemTools
} // namespace BuiltinTools
