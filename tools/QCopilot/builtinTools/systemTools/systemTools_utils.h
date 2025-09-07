#pragma once

#include <string>
#include <vector>
#include "../common/tool_types.h"

namespace BuiltinTools {
namespace SystemTools {
namespace Utils {

// 系统工具辅助函数
std::string formatFileSize(uintmax_t size_bytes);
std::string formatTimeStamp(const std::chrono::system_clock::time_point& time_point);
bool validateSystemPath(const std::string& path, std::string& error_message);

} // namespace Utils
} // namespace SystemTools
} // namespace BuiltinTools
