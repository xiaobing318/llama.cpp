/**
 * @file MathTools-basic-math-calculator.h
 * @brief 文件介绍：提供基础数学计算类内置工具的接口声明。
 *
 * 文件概述：本文件定义了数学工具模块（MathTools）的接口，包括工具调用定义与执行函数。它依赖于通用的工具类型（types.h），
 * 并封装了统一的 JSON 输入输出接口，便于在工具调用框架中实现扩展与复用。
 *
 * 使用方式：
 * - 在需要注册数学计算工具时，调用 `get_basic_math_calculator_definition()` 获取该工具的元信息，并将其纳入工具注册表中。
 * - 在需要执行具体计算逻辑时，调用 `run_basic_math_calculator(const json& args)`，并传入 JSON 格式的参数，函数将返回 JSON 格式的结果。
 *
 * 使用场景：
 * - 在工具调用系统中，LLM 或其他模块通过工具注册表发现 `basic_math_calculator` 工具，并调用执行。
 * - 在扩展内置数学类工具时，作为接口层文件提供统一声明，方便实现和维护。
 * - 在跨平台或多模块协作时，作为统一入口暴露 `basic_math_calculator` 工具的功能。
 */
#pragma once

#include "../common/types.h"

namespace BuiltinTools {
namespace MathTools {

// 获取工具调用定义
BuiltinTools::Types::ToolDefinition get_basic_math_calculator_definition();

// 执行工具调用实现
json run_basic_math_calculator(const json& args);

} // namespace MathTools
} // namespace BuiltinTools
