#pragma once

#include "../common/tool_types.h"

namespace BuiltinTools {
namespace TimeTools {

// 获取get_current_time工具的定义
ToolDefinition get_current_time_definition();

// 执行get_current_time工具
json run_get_current_time(const json& args);

} // namespace TimeTools
} // namespace BuiltinTools
