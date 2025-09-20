#pragma once

#include "json.hpp"

#include <string>

namespace builtin_tools::common {

using json = nlohmann::ordered_json;

json safe_parse_json(const std::string& text);
std::string format_json(const json& value);

} // namespace builtin_tools::common
