#pragma once

#include "../common/tool_types.h"

namespace BuiltinTools {
namespace MathTools {

// 获取calculate工具的定义
ToolDefinition getCalculateDefinition();

// 执行calculate工具
json executeCalculate(const json& args);

} // namespace MathTools
} // namespace BuiltinTools