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
    // 测试用例：针对空路径的测试应该返回 false
    T.check(!validatePath("", err), "针对空路径的测试没有返回 false，说明接口存在问题。");
    err.clear();
    // 测试用例：针对超过 4096 字节路径的测试应该返回 false
    T.check(!validatePath(std::string(5000, 'a'), err), "针对超过 4096 字节路径的测试没有返回 false，说明接口存在问题。");
    err.clear();
    // 测试用例：针对 URL 形式路径的测试应该返回 false
    T.check(!validatePath("https://示例.com/资源.txt", err), "针对 URL 形式路径的测试没有返回 false，说明接口存在问题。");
    err.clear();
    // 测试用例：针对包含空字节路径的测试应该返回 false
    std::string pathWithNull = dataRoot.string();
    pathWithNull.push_back('\0');
    pathWithNull += "通用";
    T.check(!validatePath(pathWithNull, err), "针对包含空字节路径的测试没有返回 false，说明接口存在问题。");
    err.clear();
    // 测试用例：针对包含控制字符路径的测试应该返回 false
    std::string pathWithControl = dataRoot.string();
    pathWithControl.push_back('\t');
    pathWithControl += "通用";
    T.check(!validatePath(pathWithControl, err), "针对包含控制字符路径的测试没有返回 false，说明接口存在问题。");
    err.clear();
    // 测试用例：针对包含..路径的测试应该返回 false
    std::string traversalAttempt = dataRoot.string();
    traversalAttempt += "../通用";
    T.check(!validatePath(traversalAttempt, err), "针对包含..路径的测试没有返回 false，说明接口存在问题。");
    err.clear();
    // 测试用例：针对不存在路径的测试应该返回 false
    const std::string missingFile = toUtf8(dataRoot / "通用" / "缺失文件.txt");
    T.check(!validatePath(missingFile, err), "针对不存在的路径的测试没有返回 false，说明接口存在问题。");
    err.clear();
    // 测试用例：针对存在路径的测试应该返回 true
    //const std::string existingFile = toUtf8(dataRoot / "通用" / "已存在文件.txt");
    //T.check(validatePath(existingFile, err), "针对存在路径的测试没有返回 true，说明接口存在问题。");
    //err.clear();
    // 测试用例：针对存在路径（目录）的测试应该返回 true
    const std::string existingDir = toUtf8(dataRoot / "通用" / "已存在目录");
    T.check(validatePath(existingDir, err), "针对存在路径（目录）的测试没有返回 true，说明接口存在问题。");
    err.clear();
    // 测试用例：针对包含空格路径的测试应该返回 true
    const std::string spacedFile = toUtf8(dataRoot / "通用" / "含空格 目录" / "含空格 文件.txt");
    T.check(validatePath(spacedFile, err), "针对包含空格路径的测试没有返回 true，说明接口存在问题。");
    err.clear();
    // 测试用例：针对包含中文字符路径的测试应该返回 true
    const std::string unicodeFile = toUtf8(dataRoot / "通用" / "含中文" / "子目录" / "文件.txt");
    T.check(validatePath(unicodeFile, err), "针对包含中文字符路径的测试没有返回 true，说明接口存在问题。");
    err.clear();

    // Windows 平台特定测试
#ifdef _WIN32
    // 测试用例：针对 Windows 平台反斜杠形式路径的测试应该返回 true
    const std::string windowsStyleFile = toWindowsStyle(dataRoot / "Windows特定" / "已存在Windows路径.txt");
    T.check(validatePath(windowsStyleFile, err), "针对 Windows 平台反斜杠形式路径的测试没有返回 true，说明接口存在问题。");
    err.clear();
    // 测试用例：针对 Windows 平台包含尖括号形式路径的测试应该返回 false
    const std::string windowsInvalidComponent = toWindowsStyle(dataRoot / "Windows特定" / "含有<尖括号>.txt");
    T.check(!validatePath(windowsInvalidComponent, err), "针对 Windows 平台包含尖括号形式路径的测试没有返回 false，说明接口存在问题。");
    err.clear();
    // 测试用例：针对 Windows 平台保留设备名相关路径的测试应该返回 false
    T.check(!validatePath("NUL.txt", err), "针对 Windows 平台保留设备名相关路径的测试没有返回 false，说明接口存在问题。");
    err.clear();
    // 测试用例：针对 Windows 平台以空格结尾路径的测试应该返回 false
    const std::string trailingSpace = toWindowsStyle(dataRoot / "Windows特定" / "结尾空格 ");
    T.check(!validatePath(trailingSpace, err), "针对 Windows 平台以空格结尾路径的测试没有返回 false，说明接口存在问题。");
    err.clear();

    // Linux 平台特定测试
#elif defined(__linux__)
    // 测试用例：针对 Linux 平台存在路径的测试应该返回 true
    const std::string linuxFile = toUtf8(dataRoot / "Linux特定" / "已存在Linux路径.txt");
    T.check(validatePath(linuxFile, err), "针对 Linux 平台存在路径的测试没有返回 true，说明接口存在问题。");
    err.clear();
    // 测试用例：针对 Linux 平台已存在绝对路径的测试应该返回 true
    const std::string linuxAbsolute = toUtf8(fs::absolute(dataRoot / "Linux特定" / "已存在Linux路径.txt"));
    T.check(validatePath(linuxAbsolute, err), "针对 Linux 平台已存在绝对路径的测试没有返回 true，说明接口存在问题。");
    err.clear();
    // 测试用例：针对 Linux 平台使用 Windows 平台路径的测试应该返回 false
    T.check(!validatePath("C:\\临时\\不存在.txt", err), "针对 Linux 平台使用 Windows 平台路径的测试应该返回 false，说明接口存在问题。");
    err.clear();

    // macOS 平台特定测试
#elif defined(__APPLE__)
    // 测试用例：针对 macOS 平台存在路径的测试应该返回 true
    const std::string macFile = toUtf8(dataRoot / "macOS特定" / "已存在macOS路径.txt");
    T.check(validatePath(macFile, err), "针对 macOS 平台存在路径的测试没有返回 true，说明接口存在问题。");
    err.clear();
    // 测试用例：针对 macOS 平台已存在绝对路径的测试应该返回 true
    const std::string macAbsolute = toUtf8(fs::absolute(dataRoot / "macOS特定" / "已存在macOS路径.txt"));
    T.check(validatePath(macAbsolute, err), "针对 macOS 平台已存在绝对路径的测试没有返回 true，说明接口存在问题。");
    err.clear();
    // 测试用例：针对 macOS 平台使用 Windows 平台路径的测试应该返回 false
    T.check(!validatePath("C:\\临时\\不存在.txt", err), "针对 macOS 平台使用 Windows 平台路径的测试没有返回 false，说明接口存在问题。");
    err.clear(); 
#endif
    // 汇总测试结果并返回退出码
    return T.finish(); 
} // main
