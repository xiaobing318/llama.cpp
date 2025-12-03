/**
 * @file MathTools-utils.h
 * @brief 文件介绍：提供 MathTools 模块常用的轻量级工具函数声明。
 *
 * 文件概述：本文件定义了数学工具（MathTools）在输入校验等场景下复用的通用小工具，
 * 例如对字符串长度的统一校验与错误信息生成，便于在不同数学类内置工具间保持一致的
 * 行为与提示文案。
 *
 * 使用方式：
 * - 在实现数学类内置工具（如 basic_math_calculator）时包含本头文件；
 * - 调用 `BuiltinTools::MathTools::Utils::validateStringLength` 对输入参数进行统一长度校验，
 *   并在失败时得到标准化的错误信息字符串；
 * - 将错误信息直接传递给上层响应构造（例如 `make_error`），以便对外输出一致的诊断说明。
 *
 * 使用场景：
 * - 任何需要对字符串参数做最大长度约束的工具实现；
 * - 需要将“字段名 + 约束条件”拼接成可读错误信息并返回给调用方时；
 * - 追求输入校验逻辑在 MathTools 内多处实现间保持统一、可维护时。
 */
#pragma once

#include <string>

namespace BuiltinTools {
namespace MathTools {
namespace Utils {

/**
 * @brief 验证字符串是否超过给定最大长度，并在失败时生成错误信息。
 *
 * 该函数用于对输入字符串 `str` 进行统一的最大长度检查：当长度大于 `max_length` 时，
 * 返回 false，并将 `error_message` 设置为可读的提示文本，包含字段名与限制值；
 * 当长度未超限时，返回 true，且不修改已有的 `error_message` 内容。
 *
 * @param str 待检查的字符串。
 * @param max_length 允许的最大长度（字符数）。
 * @param field_name 字段名（用于构造错误提示，例如 "Expression"）。
 * @param error_message 输出参数；当校验失败时，填充标准化错误信息。
 * @return true 表示长度未超限；false 表示长度超限并提供错误信息。
 */
bool validateStringLength(
    const std::string& str,
    size_t max_length,
    const std::string& field_name,
    std::string& error_message);

} // namespace Utils
} // namespace MathTools
} // namespace BuiltinTools
