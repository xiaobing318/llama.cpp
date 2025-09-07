#include "systemTools_utils.h"
#include "../common/common_utils.h"
#include <sstream>
#include <iomanip>
#include <chrono>

namespace BuiltinTools {
namespace SystemTools {
namespace Utils {

std::string formatFileSize(uintmax_t size_bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    double size = static_cast<double>(size_bytes);
    int unit = 0;

    while (size >= 1024 && unit < 4) {
        size /= 1024;
        unit++;
    }

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1) << size << " " << units[unit];
    return ss.str();
}

std::string formatTimeStamp(const std::chrono::system_clock::time_point& time_point) {
    auto time_t = std::chrono::system_clock::to_time_t(time_point);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

bool validateSystemPath(const std::string& path, std::string& error_message) {
    return BuiltinTools::Utils::validatePath(path, error_message);
}

} // namespace Utils
} // namespace SystemTools
} // namespace BuiltinTools
