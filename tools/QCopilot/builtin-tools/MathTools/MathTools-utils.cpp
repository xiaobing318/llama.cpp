#include "MathTools-utils.h"
#include <filesystem>

namespace BuiltinTools {
namespace MathTools {
namespace Utils {

// 通用辅助工具：验证字符串长度是否超出指定长度
bool validateStringLength(
    const std::string& str,
    size_t max_length,
    const std::string& field_name,
    std::string& error_message) {
    // 验证字符串的长度是否超过了允许的最大长度
    if (str.length() > max_length) {
        // 如果超过了最大长度，则构造错误信息并返回 false
        error_message = field_name + " too long (maximum " + std::to_string(max_length) + " characters)";
        return false;
    }
    return true;
}

} // namespace Utils
} // namespace MathTools
} // namespace BuiltinTools
