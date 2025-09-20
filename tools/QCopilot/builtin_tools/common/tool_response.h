#pragma once

#include "json.hpp"

#include <string>
#include <vector>

namespace builtin_tools::common {

using json = nlohmann::ordered_json;

json make_success(const std::string& tool_name);
json make_error(const std::string& tool_name, const std::string& message);
void append_message(json& response, const std::string& message);
void set_truncated(json& response, bool truncated);

} // namespace builtin_tools::common
