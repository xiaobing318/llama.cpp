#include "test_support.h"
#include "../builtinTools/common/common_utils.h"
// 引入算法库以便进行字符串替换
#include <algorithm>
// 引入文件系统库以处理路径
#include <filesystem>
#include <string>

#ifdef _WIN32
  #include <windows.h>
#endif // _WIN32

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;


// 计算测试数据根路径
fs::path getDataRoot() {
    // 将当前源文件路径转换为文件系统路径
    const fs::path source = fs::u8path(__FILE__);
    // 拼接得到测试数据目录
    return source.parent_path() / "data" / "validatePath";
}

// 将文件系统路径转换为 UTF-8 字符串
std::string toUtf8(const fs::path& p) {
    // 使用工具函数进行转换
    return BuiltinTools::Utils::pathToUtf8String(p);
}

#ifdef _WIN32
// 将路径转换为 Windows 风格的反斜杠表示
std::string toWindowsStyle(const fs::path& p) {
    // 先转换为 UTF-8 字符串
    std::string s = toUtf8(p);
    // 将斜杠替换为反斜杠
    std::replace(s.begin(), s.end(), '/', '\\');
    // 返回转换后的字符串
    return s;
}
#endif // _WIN32

int main() {

    // Windows 平台预处理块
#ifdef _WIN32
    // 设置控制台输出编码为 UTF-8 以便显示中文
    SetConsoleOutputCP(CP_UTF8);
    // 设置控制台输入编码为 UTF-8
    SetConsoleCP(CP_UTF8);
#endif // _WIN32

    // 创建测试运行器对象
    qctest::Test T;
    // 用于接收错误信息的字符串
    std::string err;
    // 获取测试数据根路径
    const fs::path dataRoot = getDataRoot();

    T.check(!validatePath("", err), "空路径应该被拒绝"); // 断言空路径会返回失败
    err.clear(); // 重置错误信息以便进行下一次校验

    T.check(!validatePath(std::string(5000, 'a'), err), "超长路径应该被拒绝"); // 断言超过长度限制的路径被拒绝
    err.clear(); // 清空错误信息缓存

    T.check(!validatePath("https://示例.com/资源.txt", err), "URL 形式的路径应该被拒绝"); // 断言 URL 形式的输入无效
    err.clear(); // 重置错误信息缓冲区

    std::string pathWithNull = "包含空字节"; // 构造包含中文说明的基础路径字符串
    pathWithNull.push_back('\0'); // 添加空字节以模拟非法字符
    pathWithNull += "继续的路径"; // 拼接后续文本形成完整字符串
    T.check(!validatePath(pathWithNull, err), "包含空字节的路径应该被拒绝"); // 断言含有空字节的路径无效
    err.clear(); // 清理错误信息

    std::string pathWithControl = "路径\t含有\t控制"; // 构造包含控制字符的路径字符串
    T.check(!validatePath(pathWithControl, err), "包含控制字符的路径应该被拒绝"); // 断言含控制字符的路径无效
    err.clear(); // 重置错误信息记录

    std::string traversalAttempt = "../越界"; // 构造尝试目录越界的路径
    T.check(!validatePath(traversalAttempt, err), "尝试越界的路径应该被拒绝"); // 断言目录穿越被拒绝
    err.clear(); // 清空错误信息字符串

    const std::string missingFile = toUtf8(dataRoot / "通用" / "缺失文件.txt"); // 构造不存在的文件路径
    T.check(!validatePath(missingFile, err), "不存在的路径应该被拒绝"); // 断言不存在的路径无效
    err.clear(); // 清理错误信息

    const std::string existingFile = toUtf8(dataRoot / "通用" / "已存在文件.txt"); // 获取已存在文件的路径
    T.check(validatePath(existingFile, err), "存在的文件应该通过校验"); // 断言存在的文件合法
    err.clear(); // 重置错误信息

    const std::string existingDir = toUtf8(dataRoot / "通用" / "已存在目录"); // 获取已存在目录路径
    T.check(validatePath(existingDir, err), "存在的目录应该通过校验"); // 断言存在的目录合法
    err.clear(); // 清空错误信息

    const std::string spacedFile = toUtf8(dataRoot / "通用" / "含空格 目录" / "含空格 文件.txt"); // 构造含空格路径的文件
    T.check(validatePath(spacedFile, err), "含空格的路径在文件存在时应该通过校验"); // 断言含空格的合法路径通过
    err.clear(); // 重置错误信息

    const std::string unicodeFile = toUtf8(dataRoot / "通用" / "含中文" / "子目录" / "文件.txt"); // 构造含中文字符的路径
    T.check(validatePath(unicodeFile, err), "包含中文字符的路径在文件存在时应该通过校验"); // 断言中文路径合法
    err.clear(); // 清空错误状态

#ifdef _WIN32 // Windows 平台特定测试
    const std::string windowsStyleFile = toWindowsStyle(dataRoot / "Windows特定" / "已存在Windows路径.txt"); // 构造 Windows 风格的反斜杠路径
    T.check(validatePath(windowsStyleFile, err), "反斜杠形式的 Windows 路径在文件存在时应该通过校验"); // 断言 Windows 风格路径合法
    err.clear(); // 清理错误信息

    const std::string windowsInvalidComponent = toWindowsStyle(dataRoot / "Windows特定" / "含有<尖括号>.txt"); // 构造包含非法字符的组件路径
    T.check(!validatePath(windowsInvalidComponent, err), "包含尖括号的组件应该被拒绝"); // 断言非法字符导致拒绝
    err.clear(); // 重置错误信息

    T.check(!validatePath("NUL.txt", err), "保留设备名应该被拒绝"); // 断言 Windows 设备名无效
    err.clear(); // 清理错误信息

    const std::string trailingSpace = toWindowsStyle(dataRoot / "Windows特定" / "结尾空格 "); // 构造以空格结尾的组件
    T.check(!validatePath(trailingSpace, err), "以空格结尾的组件应该被拒绝"); // 断言结尾空格路径无效
    err.clear(); // 清空错误信息
#elif defined(__linux__) // Linux 平台特定测试
    const std::string linuxFile = toUtf8(dataRoot / "Linux特定" / "已存在Linux路径.txt"); // 构造 Linux 下存在的文件路径
    T.check(validatePath(linuxFile, err), "Linux 平台存在的文件应该通过校验"); // 断言 Linux 文件合法
    err.clear(); // 清理错误信息

    const std::string linuxAbsolute = toUtf8(fs::absolute(dataRoot / "Linux特定" / "已存在Linux路径.txt")); // 构造 Linux 绝对路径
    T.check(validatePath(linuxAbsolute, err), "Linux 平台的绝对路径应该通过校验"); // 断言绝对路径合法
    err.clear(); // 重置错误信息

    T.check(!validatePath("C:\\临时\\不存在.txt", err), "伪造的 Windows 路径在 Linux 上应该被拒绝"); // 断言 Windows 风格路径在 Linux 无效
    err.clear(); // 清空错误信息
#elif defined(__APPLE__) // macOS 平台特定测试
    const std::string macFile = toUtf8(dataRoot / "macOS特定" / "已存在macOS路径.txt"); // 构造 macOS 下存在的文件路径
    T.check(validatePath(macFile, err), "macOS 平台存在的文件应该通过校验"); // 断言 macOS 文件合法
    err.clear(); // 清理错误信息

    const std::string macAbsolute = toUtf8(fs::absolute(dataRoot / "macOS特定" / "已存在macOS路径.txt")); // 构造 macOS 绝对路径
    T.check(validatePath(macAbsolute, err), "macOS 平台的绝对路径应该通过校验"); // 断言 macOS 绝对路径合法
    err.clear(); // 重置错误信息

    T.check(!validatePath("C:\\临时\\不存在.txt", err), "伪造的 Windows 路径在 macOS 上应该被拒绝"); // 断言 Windows 风格路径在 macOS 无效
    err.clear(); // 清空错误信息
#endif // 平台特定测试结束
    return T.finish(); // 汇总测试结果并返回退出码
} // main
