#pragma once

#include "../common/tool_types.h"

namespace BuiltinTools {
namespace MathTools {

// 获取 calculate 工具的定义
ToolDefinition getCalculateDefinition();

// 执行 calculate 工具
json executeCalculate(const json& args);

} // namespace MathTools
} // namespace BuiltinTools
