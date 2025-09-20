#include "json_utils.h"

namespace builtin_tools::common {

json safe_parse_json(const std::string& text) {
    try {
        return json::parse(text);
    } catch (const json::parse_error& error) {
        return json{
            { "success", false },
            { "error",   "JSON parse failed" },
            { "message", error.what() },
            { "byte",    error.byte }
        };
    }
}

std::string format_json(const json& value) {
    try {
        return value.dump(2);
    } catch (const std::exception&) {
        return "{}";
    }
}

} // namespace builtin_tools::common
