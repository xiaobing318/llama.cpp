#include "path_stat.h"
#include "../common/encoding_utils.h"
#include "../common/filesystem_utils.h"
#include "../common/io_utils.h"
#include "../common/string_utils.h"
#include "../common/tool_response.h"
#include <filesystem>
#include <algorithm>

namespace BuiltinTools {
namespace SystemTools {

constexpr const char* k_tool_name = "path_stat";

namespace common = builtin_tools::common;

// 获取 path_stat 工具的定义
ToolDefinition get_path_stat_definition() {
    return {
        "path_stat",
        {
            {"type", "function"},
            {"function", {
                {"name", "path_stat"},
                {"description", "Cross-platform path inspection and file analysis utility for comprehensive path validation and metadata extraction. Designed exclusively for path validation, file type identification, and content analysis with UTF-8 text file support. IMPORTANT: Uses only C++ standard library for maximum compatibility across Windows and Linux systems. Primary use cases: 1) Path validation and existence verification before file operations 2) File type identification (file/directory/other) for workflow routing 3) Basic file metadata extraction (size, timestamps, permissions) 4) UTF-8 text file analysis including line counting for documentation and code files 5) Directory content summarization for project organization 6) Cross-platform file system inspection without platform-specific dependencies. Key features: path validation, absolute/relative path resolution, file type detection, size analysis with human-readable formats, modification timestamps, Unix-style permissions (detailed mode), and UTF-8 text line counting. Workflow: path_stat → read_text_file/write_text_file (for confirmed text files)."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"path", {{"type", "string"}, {"description", "Target path for inspection (absolute or relative). Examples: './config.txt', '/var/log/app.log', 'C:/Documents/readme.md', '../src/main.cpp'. Supports cross-platform path formats. The tool validates path syntax and existence before analysis."}}},
                        {"detailed", {{"type", "boolean"}, {"description", "Analysis depth: false (basic inspection - path validation, type identification, size, modified time) or true (comprehensive analysis - includes permissions, directory statistics, and enhanced metadata). Default false for optimal performance. Detailed mode provides Unix-style permission strings and recursive directory analysis."}}},
                        {"text_analysis", {{"type", "boolean"}, {"description", "Enable UTF-8 text file analysis including line counting and content statistics. Only applicable to regular files that can be read as text. Provides line count, character count for text files. Default true to for file reading. Useful for code files, documentation, configuration files."}}}
                    }},
                    {"required", {"path"}}
                }}
            }}
        }
    };
}

// 执行 path_stat 工具
json run_path_stat(const json& args) {
    // 提取参数，设置默认值
    std::string path = args.value("path", "");
    bool detailed = args.value("detailed", false);
    bool text_analysis = args.value("text_analysis", true);

    // 参数验证
    std::string error_message;
    if (!common::validate_existing_path(path, error_message)) {
        LOG_ERR("path_stat: Path validation failed for '%s': %s", path.c_str(), error_message.c_str());
        json err = common::make_error(k_tool_name, error_message);
        err["path"] = path;
        err["messages"] = json::array({"Path validation failed; please verify path syntax and traversal"});
        return err;
    }

    try {
        std::filesystem::path fs_path = common::utf8_to_path(path);

        // 检查路径是否存在
        if (!std::filesystem::exists(fs_path)) {
            LOG_ERR("path_stat: Path not found: %s", path.c_str());
            json err = common::make_error(k_tool_name, "Path not found: " + path);
            err["path"] = path;
            err["messages"] = json::array({"Path does not exist"});
            return err;
        }
        // 构建结果JSON
        json result = common::make_success(k_tool_name);
        result["path"] = path;
        result["exists"] = true;
        result["absolute_path"] = common::path_to_utf8_string(std::filesystem::absolute(fs_path));
        result["filename"] = common::path_to_utf8_string(fs_path.filename());
        // 汇总提示信息
        json messages = json::array();
        messages.push_back(detailed ? "Detailed mode enabled" : "Detailed mode disabled");
        messages.push_back(text_analysis ? "Text analysis enabled" : "Text analysis disabled");
        // 组合提示：详细模式关闭但启用了文本分析
        if (!detailed && text_analysis) {
            messages.push_back("Detailed mode is disabled; only basic metadata and text analysis for regular files will be returned (no directory statistics)");
        }
        // 文件类型识别
        if (std::filesystem::is_regular_file(fs_path)) {
            result["type"] = "file";
            // 文件大小信息
            auto file_size = std::filesystem::file_size(fs_path);
            result["size"] = file_size;
            result["human_size"] = common::format_file_size(file_size);
            // 文件扩展名
            if (fs_path.has_extension()) {
                result["extension"] = common::path_to_utf8_string(fs_path.extension());
            }
            // UTF-8文本文件分析
            if (text_analysis) {
                std::string content;
                if (common::read_file_content(path, content)) {
                    // 统计行数
                    size_t line_count = 1; // 至少有一行
                    if (!content.empty()) {
                        line_count = std::count(content.begin(), content.end(), '\n') + 1;
                        // 如果文件以换行符结尾，行数减1
                        if (content.back() == '\n' && content.size() > 1) {
                            line_count--;
                        }
                    } else {
                        line_count = 0; // 空文件
                    }
                    // 返回文本统计信息
                    result["text_stats"] = {
                        {"line_count", line_count},
                        {"character_count", content.size()},
                        {"is_text_readable", true}
                    };
                    messages.push_back("Text analysis completed (readable text)");
                } else {
                    // 读取失败，可能是二进制文件或编码问题
                    LOG_WRN("path_stat: Cannot read file as text: %s", path.c_str());
                    result["text_stats"] = {
                        {"is_text_readable", false},
                        {"error", "Cannot read file as text (may be binary or encoding issue)"}
                    };
                    messages.push_back("Text analysis skipped (not readable as UTF-8 text)");
                }
            }

        } else if (std::filesystem::is_directory(fs_path)) {
            result["type"] = "directory";
            messages.push_back("Path is a directory");
            if (text_analysis) {
                messages.push_back("Text analysis applies to regular files; ignored for directories");
            }

            // 目录统计（详细模式）
            if (detailed) {
                size_t file_count = 0;
                size_t dir_count = 0;
                uintmax_t total_size = 0;

                try {
                    for (auto& entry : std::filesystem::recursive_directory_iterator(fs_path)) {
                        if (entry.is_regular_file()) {
                            file_count++;
                            total_size += std::filesystem::file_size(entry);
                        } else if (entry.is_directory()) {
                            dir_count++;
                        }
                    }

                    result["directory_stats"] = {
                        {"file_count", file_count},
                        {"subdirectory_count", dir_count},
                        {"total_size", total_size}
                    };
                    messages.push_back("Directory stats computed recursively (detailed mode)");
                } catch (const std::exception& e) {
                    LOG_WRN("path_stat: Failed to analyze directory contents for '%s': %s", path.c_str(), e.what());
                    result["directory_stats"] = {
                        {"error", "Failed to analyze directory contents: " + std::string(e.what())}
                    };
                    messages.push_back("Failed to enumerate directory for stats");
                }
            }
        } else {
            result["type"] = "other";
            messages.push_back("Path is neither regular file nor directory");
        }

        // 时间戳信息
        try {
            auto ftime = std::filesystem::last_write_time(fs_path);
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - std::filesystem::file_time_type::clock::now() +
                std::chrono::system_clock::now()
            );
            auto time_t = std::chrono::system_clock::to_time_t(sctp);

            result["last_modified"] = common::format_timestamp(sctp);
            result["last_modified_timestamp"] = time_t;
        } catch (const std::exception& e) {
            LOG_WRN("path_stat: Failed to get modification time for '%s': %s", path.c_str(), e.what());
            result["last_modified"] = "unknown";
            result["last_modified_timestamp"] = 0;
        }

        // 详细权限信息（跨平台Unix风格）
        if (detailed) {
            try {
                auto perms = std::filesystem::status(fs_path).permissions();
                std::string perm_str;

                perm_str += (perms & std::filesystem::perms::owner_read) != std::filesystem::perms::none ? "r" : "-";
                perm_str += (perms & std::filesystem::perms::owner_write) != std::filesystem::perms::none ? "w" : "-";
                perm_str += (perms & std::filesystem::perms::owner_exec) != std::filesystem::perms::none ? "x" : "-";
                perm_str += (perms & std::filesystem::perms::group_read) != std::filesystem::perms::none ? "r" : "-";
                perm_str += (perms & std::filesystem::perms::group_write) != std::filesystem::perms::none ? "w" : "-";
                perm_str += (perms & std::filesystem::perms::group_exec) != std::filesystem::perms::none ? "x" : "-";
                perm_str += (perms & std::filesystem::perms::others_read) != std::filesystem::perms::none ? "r" : "-";
                perm_str += (perms & std::filesystem::perms::others_write) != std::filesystem::perms::none ? "w" : "-";
                perm_str += (perms & std::filesystem::perms::others_exec) != std::filesystem::perms::none ? "x" : "-";

                result["permissions"] = perm_str;
            } catch (const std::exception& e) {
                LOG_WRN("path_stat: Failed to get permissions for '%s': %s", path.c_str(), e.what());
                result["permissions"] = "unknown";
            }
        }

        if (!messages.empty()) result["messages"] = std::move(messages);
        return result;

    } catch (const std::filesystem::filesystem_error& e) {
        LOG_ERR("path_stat: Filesystem error for '%s': %s", path.c_str(), e.what());
        json err = common::make_error(k_tool_name, "Filesystem error: " + std::string(e.what()));
        err["path"] = path;
        err["messages"] = json::array({"Filesystem exception during stat"});
        return err;
    } catch (const std::exception& e) {
        LOG_ERR("path_stat: Failed to inspect path '%s': %s", path.c_str(), e.what());
        json err = common::make_error(k_tool_name, "Failed to inspect path: " + std::string(e.what()));
        err["path"] = path;
        err["messages"] = json::array({"Unexpected exception during path inspection"});
        return err;
    }
}

} // namespace SystemTools
} // namespace BuiltinTools
