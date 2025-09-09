#pragma once

#include <string>
#include <vector>

namespace BuiltinTools {
namespace Utils {
namespace Internal {

// 内部接口：按指定行范围读取文本（已由工具层进行更严格的 BOM/CRLF/UTF-8/二进制片段处理）
// 仅供内部使用或测试，外部业务请优先使用 FileTools::read_text_lines 工具。
bool readTextFileWithRange(
    const std::string& path,
    int start_line,
    int end_line,
    std::string& content,
    int& lines_read,
    int& actual_end_line);

// 以下内部辅助函数仅供 common 内部与单测使用
char to_lower_unsafe(char c);
std::string normalize_case(std::string s, bool case_sensitive);
std::string slashify(std::string s);
bool matchCharClass(char c, const std::string& cls, bool case_sensitive);
bool globSegmentMatch(const std::string& text, const std::string& pat, bool case_sensitive);
std::vector<std::string> splitPatternSegments(std::string pat);
void append_replacement_char(std::string& out);
bool is_cont(unsigned char x);

} // namespace Internal
} // namespace Utils
} // namespace BuiltinTools
