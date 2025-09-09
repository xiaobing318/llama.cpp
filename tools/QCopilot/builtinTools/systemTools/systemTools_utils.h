#pragma once

#include <string>
#include <vector>
#include "../common/tool_types.h"

namespace BuiltinTools {
namespace SystemTools {
namespace Utils {

// 通用辅助函数：将字节大小格式化为人类可读的字符串
std::string formatFileSize(uintmax_t size_bytes);
// 通用辅助函数：将时间戳格式化为人类可读的字符串
std::string formatTimeStamp(const std::chrono::system_clock::time_point& time_point);
// 通用辅助函数：验证系统路径的合法性
bool validateSystemPath(const std::string& path, std::string& error_message);

} // namespace Utils
} // namespace SystemTools
} // namespace BuiltinTools
