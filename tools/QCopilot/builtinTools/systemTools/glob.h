#pragma once

#include "../common/tool_types.h"

namespace BuiltinTools {
namespace SystemTools {

// 获取 glob 工具的定义
ToolDefinition getGlobDefinition();

// 执行 glob 工具
json executeGlob(const json& args);

} // namespace SystemTools
} // namespace BuiltinTools
