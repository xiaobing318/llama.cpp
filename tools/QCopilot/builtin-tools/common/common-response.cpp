#include "common-response.h"

namespace BuiltinTools{
namespace Common{

// 辅助函数：生成基础响应模板
static json base_template(const std::string& tool_name, bool success) {
    return json{
        { "tool", tool_name },
        { "success", success },
        { "messages", json::array() },
        { "truncated", false }
    };
}

// 给定工具名称，生成成功响应
json make_success(const std::string& tool_name) {
    return base_template(tool_name, true);
}

// 给定工具名称、错误信息，生成失败响应
json make_error(const std::string& tool_name, const std::string& message) {
    json response = base_template(tool_name, false);
    response["error"] = message;
    response["messages"].push_back(message);
    return response;
}

// 向已存在的响应中追加消息
void append_message(json& response, const std::string& message) {
    if (!response.contains("messages") || !response["messages"].is_array()) {
        response["messages"] = json::array();
    }
    response["messages"].push_back(message);
}

// 设置已存在响应的截断状态
void set_truncated(json& response, bool truncated) {
    response["truncated"] = truncated;
}

} // namespace Common
} // namespace BuiltinTools
