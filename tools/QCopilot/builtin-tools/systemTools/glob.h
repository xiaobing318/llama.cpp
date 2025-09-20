#pragma once

#include "../common/tool_types.h"

namespace BuiltinTools {
namespace SystemTools {

// 获取 glob 工具的定义
ToolDefinition get_glob_definition();

// 执行 glob 工具
json run_glob(const json& args);

} // namespace SystemTools
} // namespace BuiltinTools
