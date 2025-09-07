#pragma once

#include <string>

namespace BuiltinTools {
namespace FileTools {
namespace Utils {

// 文件操作工具函数
bool validateFilePath(const std::string& path, std::string& error_message);
bool isValidUtf8File(const std::string& path);
std::string getFileExtension(const std::string& path);

} // namespace Utils
} // namespace FileTools  
} // namespace BuiltinTools