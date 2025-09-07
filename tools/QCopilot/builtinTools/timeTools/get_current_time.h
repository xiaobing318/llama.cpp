#pragma once

#include "../common/tool_types.h"

namespace BuiltinTools {
namespace TimeTools {

// 获取get_current_time工具的定义
ToolDefinition getGetCurrentTimeDefinition();

// 执行get_current_time工具
json executeGetCurrentTime(const json& args);

} // namespace TimeTools
} // namespace BuiltinTools
