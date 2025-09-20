#pragma once

#include "../common/tool_types.h"

namespace BuiltinTools {
namespace SystemTools {

// 获取 grep 工具的定义
ToolDefinition get_grep_definition();

// 执行 grep 工具
json run_grep(const json& args);

} // namespace SystemTools
} // namespace BuiltinTools
