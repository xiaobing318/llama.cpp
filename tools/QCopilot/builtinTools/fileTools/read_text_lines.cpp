#include "read_text_lines.h"
#include "../common/common_utils.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

namespace fs = std::filesystem;

namespace BuiltinTools {
namespace FileTools {

// 获取 read_text_lines 工具的定义
ToolDefinition getReadTextLinesDefinition() {
    return {
        "read_text_lines",
        {
            {"type", "function"},
            {"function", {
                {"name", "read_text_lines"},
                {"description",
                    R"(Specialized UTF-8 text file reading utility with safe 1-based line-range selection.
Primary purpose:
- Load UTF-8 text documents and extract exact line ranges (1-based, inclusive).
- Always count total lines first, then validate the requested range; reads only the selected segment.
- Focused on text content extraction. Does not attempt to parse formats or return metadata beyond what is documented.
Recommended workflow:
- (Optional but recommended) First validate the file encoding and type using your dedicated tools,
  e.g., validate_utf8_file → read_text_lines, or validate_utf8_file → inspect_path → read_text_lines.
Key notes:
- Binary files are not supported. If enforce_utf8=true (default), a non-UTF-8 segment will cause failure.
- Windows CRLF is normalized by removing the trailing '\r' from each line; output lines end with '\n'.
- Large files are guarded by max_file_size_bytes to avoid accidental heavy reads.
Parameters:
- path (string, required): Target UTF-8 text file path (absolute or relative).
- start_line (integer, optional, default=1): 1-based inclusive start line.
- end_line (integer, optional, default=EOF): 1-based inclusive end line; omit or <=0 to read to end-of-file.
- include_line_numbers (boolean, optional, default=true): If true, returns `lines` array with {no,text}.
- enforce_utf8 (boolean, optional, default=true): Validate that the selected text is valid UTF-8.
- max_file_size_bytes (integer, optional, default=104857600): Hard upper bound for file size (100MB by default).
Returns:
- ok (boolean), tool (string), path (string), encoding (string="utf-8"), enforce_utf8 (bool), include_line_numbers (bool),
  total_lines (integer), range {start_line, end_line}, content (string), lines? (array of {no,text}).
Examples:
- { "path": "./README.md", "start_line": 1, "end_line": 80 }
- { "path": "C:/logs/app.log", "start_line": 100, "include_line_numbers": false }
- { "path": "/var/log/syslog", "end_line": 200, "max_file_size_bytes": 16777216 })"},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"path", {{"type", "string"}, {"description", "UTF-8 text file path"}}},
                        {"start_line", {{"type", "integer"}, {"minimum", 1}}},
                        {"end_line", {{"type", "integer"}, {"minimum", 1}}},
                        {"include_line_numbers", {{"type", "boolean"}, {"default", true}}},
                        {"enforce_utf8", {{"type", "boolean"}, {"default", true}}},
                        {"max_file_size_bytes", {{"type", "integer"}, {"minimum", 1}, {"default", 104857600}}}
                    }},
                    {"required", {"path"}},
                    {"additionalProperties", false}
                }}
            }}
        }
    };
}

// 执行 read_text_lines 工具
json executeReadTextLines(const json& args) {
    // 提取参数并设默认值（若键不存在）
    const std::string path = args.value("path", std::string{});
    int64_t  start_line = args.value("start_line", (int64_t)1);
    int64_t  end_line   = args.value("end_line",   (int64_t)-1); // -1 => EOF
    bool     include_line_numbers = args.value("include_line_numbers", true);
    bool     enforce_utf8         = args.value("enforce_utf8", true);
    uint64_t max_file_size_bytes  = args.value("max_file_size_bytes", (uint64_t)(100ULL * 1024ULL * 1024ULL));

    // 检查输入路径是否存在
    if (path.empty()) {
        LOG_ERR("read_text_lines: empty 'path'");
        return BuiltinTools::Utils::createErrorResponse("Path is empty");
    }
    // 检查输入文件是否存在且可读
    std::string path_err;
    if (!BuiltinTools::Utils::is_regular_readable_file(path, path_err)) {
        LOG_WRN("read_text_lines: invalid file: %s (%s)", path.c_str(), path_err.c_str());
        return BuiltinTools::Utils::createErrorResponse("Invalid file: " + path + " (" + path_err + ")");
    }
    // 检查文件大小是否在允许范围内
    uintmax_t fsz = 0;
    try { fsz = fs::file_size(fs::path(path)); } catch (...) {}
    if (fsz > max_file_size_bytes) {
        LOG_WRN("read_text_lines: file too large: %s size=%ju limit=%ju",
                path.c_str(), (uintmax_t)fsz, (uintmax_t)max_file_size_bytes);
        return BuiltinTools::Utils::createErrorResponse(
            "File too large: " + std::to_string((uintmax_t)fsz) +
            " bytes (limit " + std::to_string((uintmax_t)max_file_size_bytes) + ")"
        );
    }

    // 获取得到输入文件的总行数
    int64_t total_lines = 0;
    {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs) {
            LOG_ERR("read_text_lines: open for counting failed: %s", path.c_str());
            return BuiltinTools::Utils::createErrorResponse("Failed to open file");
        }
        std::string tmp;
        while (std::getline(ifs, tmp)) {
            if (!tmp.empty() && tmp.back() == '\r') tmp.pop_back(); // 统一处理 CRLF
            ++total_lines;
        }
        if (ifs.bad()) {
            LOG_ERR("read_text_lines: I/O error during line counting: %s", path.c_str());
            return BuiltinTools::Utils::createErrorResponse("I/O error during line counting");
        }
    }

    // 规范化并验证行号范围
    if (end_line <= 0){
        end_line = total_lines;
    }
    // 检查起始行号的合法性
    if (start_line < 1) {
        LOG_ERR("read_text_lines: invalid start_line=%lld", (long long)start_line);
        return BuiltinTools::Utils::createErrorResponse("start_line must be >= 1");
    }
    if (total_lines > 0 && start_line > total_lines) {
        LOG_ERR("read_text_lines: start_line(%lld) > total_lines(%lld)", (long long)start_line, (long long)total_lines);
        return BuiltinTools::Utils::createErrorResponse("start_line exceeds total_lines=" + std::to_string(total_lines));
    }
    // 检查结束行号的合法性
    if (total_lines > 0 && end_line > total_lines) {
        LOG_ERR("read_text_lines: end_line(%lld) > total_lines(%lld)", (long long)end_line, (long long)total_lines);
        return BuiltinTools::Utils::createErrorResponse("end_line exceeds total_lines=" + std::to_string(total_lines));
    }
    // 检查起始行号不大于结束行号
    if (total_lines > 0 && start_line > end_line) {
        LOG_ERR("read_text_lines: start_line(%lld) > end_line(%lld)", (long long)start_line, (long long)end_line);
        return BuiltinTools::Utils::createErrorResponse("Invalid range: start_line > end_line");
    }

    // 读取指定行号范围的数据，代码执行到这里说明参数合法
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        LOG_ERR("read_text_lines: open for reading failed: %s", path.c_str());
        return BuiltinTools::Utils::createErrorResponse("Failed to open file");
    }
    // 预分配空间
    std::vector<std::pair<int64_t, std::string>> buf;
    buf.reserve((size_t) std::max<int64_t>(0, end_line - start_line + 1));
    // 逐行读取
    std::string line;
    int64_t current = 0;
    while (std::getline(ifs, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        ++current;
        if (current < start_line) continue;
        if (end_line > 0 && current > end_line) break;
        buf.emplace_back(current, line);
    }
    // 读取过程中发生 I/O 错误
    if (ifs.bad()) {
        LOG_ERR("read_text_lines: I/O error during reading: %s", path.c_str());
        return BuiltinTools::Utils::createErrorResponse("I/O error during reading");
    }

    // 针对返回片段验证其是否为正确的UTF-8字符集编码
    if (enforce_utf8) {
        std::string joined;
        joined.reserve((size_t)std::min<uint64_t>((uint64_t)fsz, max_file_size_bytes));
        for (const auto& p : buf) {
            joined.append(p.second);
            joined.push_back('\n');
        }
        // 验证读取的内容是否为合法的 UTF-8 字符串
        if (!BuiltinTools::Utils::isValidUtf8String(joined)) {
            LOG_ERR("read_text_lines: selected segment is not valid UTF-8: %s", path.c_str());
            return BuiltinTools::Utils::createErrorResponse("Selected text is not valid UTF-8");
        }
    }

    // 组装输出
    json out = BuiltinTools::Utils::createSuccessResponse();
    out["tool"] = "read_text_lines";
    out["path"] = path;
    out["encoding"] = "utf-8";
    out["enforce_utf8"] = enforce_utf8;
    out["include_line_numbers"] = include_line_numbers;
    out["total_lines"] = total_lines;
    out["range"] = { {"start_line", start_line}, {"end_line", end_line} };

    // 拼接 content 字段
    std::string content;
    content.reserve((size_t) std::min<uint64_t>((uint64_t)fsz, max_file_size_bytes));
    for (const auto& p : buf) {
        content.append(p.second);
        content.push_back('\n');
    }
    out["content"] = std::move(content);
    // 根据需要拼接 lines 字段
    if (include_line_numbers) {
        json arr = json::array();
        for (const auto& p : buf) {
            arr.push_back({ {"no", p.first}, {"text", p.second} });
        }
        out["lines"] = std::move(arr);
    }
    return out;
}

} // namespace FileTools
} // namespace BuiltinTools
