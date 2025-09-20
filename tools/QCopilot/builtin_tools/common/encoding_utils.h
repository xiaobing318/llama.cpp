#pragma once

#include <string>

namespace builtin_tools::common {

bool is_valid_utf8_string(const std::string& value);
bool is_valid_utf8_file(const std::string& path);

} // namespace builtin_tools::common
