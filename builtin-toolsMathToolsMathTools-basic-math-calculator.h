#pragma once

#include "../common/common-types.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace BuiltinTools {
namespace MathTools {

// 获取 basic_math_calculator 工具的定义
BuiltinTools::Types::ToolDefinition get_basic_math_calculator_definition();

// 执行 basic_math_calculator 工具的实现
json run_basic_math_calculator(const json& args);

} // namespace MathTools
} // namespace BuiltinTools