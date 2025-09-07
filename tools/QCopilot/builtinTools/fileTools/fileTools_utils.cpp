#include "fileTools_utils.h"
#include "../common/common_utils.h"
#include <filesystem>

namespace BuiltinTools {
namespace FileTools {
namespace Utils {

bool validateFilePath(const std::string& path, std::string& error_message) {
    if (path.empty()) {
        error_message = "Path is required";
        return false;
    }

    // 路径安全检查：防止路径遍历攻击
    if (path.find("..") != std::string::npos) {
        error_message = "Path traversal not allowed";
        return false;
    }

    // 检查路径长度是否合理
    if (path.length() > 4096) {
        error_message = "Path too long (maximum 4096 characters)";
        return false;
    }

    return true;
}

bool isValidUtf8File(const std::string& path) {
    return BuiltinTools::Utils::isValidUtf8File(path);
}

std::string getFileExtension(const std::string& path) {
    std::filesystem::path fs_path(path);
    return fs_path.extension().string();
}

} // namespace Utils
} // namespace FileTools
} // namespace BuiltinTools
