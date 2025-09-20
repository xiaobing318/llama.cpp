#pragma once

#include "../common/tool_types.h"

namespace BuiltinTools {
namespace MathTools {

// 获取 calculate 工具的定义
ToolDefinition get_calculate_definition();

// 执行 calculate 工具
json run_calculate(const json& args);

} // namespace MathTools
} // namespace BuiltinTools
