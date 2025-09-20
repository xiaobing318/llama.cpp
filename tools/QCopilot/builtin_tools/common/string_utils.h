#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace builtin_tools::common {

std::string sanitize_string_for_json(const std::string& input);
std::vector<std::string> split_string(const std::string& str, char delimiter);
std::string trim_string(const std::string& str);
std::string join_strings(const std::vector<std::string>& strings, const std::string& delimiter);
std::string format_file_size(uintmax_t size_bytes);

} // namespace builtin_tools::common
