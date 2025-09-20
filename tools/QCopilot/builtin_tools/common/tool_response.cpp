#include "tool_response.h"

namespace builtin_tools::common {

namespace {

json base_template(const std::string& tool_name, bool success) {
    return json{
        { "tool", tool_name },
        { "success", success },
        { "messages", json::array() },
        { "truncated", false }
    };
}

} // namespace

json make_success(const std::string& tool_name) {
    return base_template(tool_name, true);
}

json make_error(const std::string& tool_name, const std::string& message) {
    json response = base_template(tool_name, false);
    response["error"] = message;
    response["messages"].push_back(message);
    return response;
}

void append_message(json& response, const std::string& message) {
    if (!response.contains("messages") || !response["messages"].is_array()) {
        response["messages"] = json::array();
    }
    response["messages"].push_back(message);
}

void set_truncated(json& response, bool truncated) {
    response["truncated"] = truncated;
}

} // namespace builtin_tools::common
