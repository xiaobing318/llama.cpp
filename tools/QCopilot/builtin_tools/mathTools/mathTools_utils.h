#pragma once

#include <string>

namespace BuiltinTools {
namespace MathTools {
namespace Utils {

// 通用辅助工具：验证字符串长度
bool validateStringLength(
    const std::string& str,
    size_t max_length,
    const std::string& field_name,
    std::string& error_message);

} // namespace Utils
} // namespace MathTools
} // namespace BuiltinTools
