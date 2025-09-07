#pragma once

#include "../common/tool_types.h"

namespace BuiltinTools {
namespace SystemTools {

// 获取inspect_path工具的定义
ToolDefinition getInspectPathDefinition();

// 执行inspect_path工具
json executeInspectPath(const json& args);

} // namespace SystemTools
} // namespace BuiltinTools