#pragma once

#include "../common/tool_types.h"

namespace BuiltinTools {
namespace SystemTools {

// 获取path_stat工具的定义
ToolDefinition get_path_stat_definition();

// 执行path_stat工具
json run_path_stat(const json& args);

} // namespace SystemTools
} // namespace BuiltinTools
