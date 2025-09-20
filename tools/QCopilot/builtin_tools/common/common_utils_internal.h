#pragma once

#include <string>
#include <vector>

namespace builtin_tools::internal {

bool read_text_with_range(
    const std::string& path,
    int start_line,
    int end_line,
    std::string& content,
    int& lines_read,
    int& actual_end_line,
    std::vector<std::string>* lines_out = nullptr);

char to_lower_unsafe(char c);
std::string normalize_case(std::string s, bool case_sensitive);
std::string slashify(std::string s);
bool match_char_class(char c, const std::string& cls, bool case_sensitive);
bool glob_segment_match(const std::string& text, const std::string& pat, bool case_sensitive);
std::vector<std::string> split_pattern_segments(std::string pat);
void append_replacement_char(std::string& out);
bool is_cont(unsigned char x);

} // namespace builtin_tools::internal

namespace BuiltinTools {
namespace Utils {
namespace Internal {

using ::builtin_tools::internal::append_replacement_char;
using ::builtin_tools::internal::glob_segment_match;
using ::builtin_tools::internal::is_cont;
using ::builtin_tools::internal::match_char_class;
using ::builtin_tools::internal::normalize_case;
using ::builtin_tools::internal::read_text_with_range;
using ::builtin_tools::internal::slashify;
using ::builtin_tools::internal::split_pattern_segments;
using ::builtin_tools::internal::to_lower_unsafe;

} // namespace Internal
} // namespace Utils
} // namespace BuiltinTools
