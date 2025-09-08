#pragma once

#include "../common/tool_types.h"

namespace BuiltinTools {
namespace SystemTools {

// 获取path_stat工具的定义
ToolDefinition getPathStatDefinition();

// 执行path_stat工具
json executePathStat(const json& args);

} // namespace SystemTools
} // namespace BuiltinTools
