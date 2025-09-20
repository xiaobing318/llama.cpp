#include "mathTools_utils.h"
#include <filesystem>

namespace BuiltinTools {
namespace MathTools {
namespace Utils {

// 通用辅助工具：验证字符串长度
bool validateStringLength(
    const std::string& str,
    size_t max_length,
    const std::string& field_name,
    std::string& error_message) {
    if (str.length() > max_length) {
        error_message = field_name + " too long (maximum " + std::to_string(max_length) + " characters)";
        return false;
    }
    return true;
}

} // namespace Utils
} // namespace MathTools
} // namespace BuiltinTools
