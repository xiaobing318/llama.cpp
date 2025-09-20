#include "../builtin_tools/common/common_utils.h"
#include "test_support.h"
// 引入算法库以便进行字符串替换
#include <algorithm>
// 引入文件系统库以处理路径
#include <filesystem>
#include <string>

#ifdef _WIN32
    #include <windows.h>
#endif  // _WIN32

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

// 计算测试数据根路径
fs::path getDataRoot() {
    // 将当前源文件路径转换为文件系统路径
    const fs::path source = fs::u8path(__FILE__);
    // 拼接得到测试数据目录
    return source.parent_path() / "data" / "validate_existing_path";
}

// 将文件系统路径转换为 UTF-8 字符串
std::string toUtf8(const fs::path & p) {
    // Windows 平台需要手动进行宽字符到 UTF-8 的转换
#ifdef _WIN32
    // 使用宽字符表示当前路径以匹配 Windows API
    const std::wstring widePath = p.wstring();
    // 空路径直接返回空字符串以避免不必要的转换
    if (widePath.empty()) {
        // 返回空字符串对象作为空路径的 UTF-8 表示
        return std::string{};
    }
    // 第一次调用以确定转换后缓冲区所需的字节数
    const int sizeWithNull = WideCharToMultiByte(CP_UTF8,           // 指定目标编码为 UTF-8
                                                 0,                 // 不使用任何附加转换标志
                                                 widePath.c_str(),  // 输入的宽字符路径数据
                                                 -1,                // 让 API 处理以空字符结尾的字符串
                                                 nullptr,           // 此次调用不输出结果只计算长度
                                                 0,                 // 输出缓冲区长度为零表示只请求长度
                                                 nullptr,           // 不提供替代字符
                                                 nullptr            // 不关心是否发生用了替代字符
    );

    // 如果计算结果不合法则认定转换失败
    if (sizeWithNull <= 0) {
        // 转换失败时返回空字符串以保持可预期行为
        return std::string{};
    }
    // 分配包含终止符空间的 UTF-8 缓冲区
    std::string utf8(static_cast<size_t>(sizeWithNull), '\0');
    // 第二次调用执行实际的宽字符到 UTF-8 转换并返回写入字节数
    const int   written = WideCharToMultiByte(CP_UTF8,           // 目标编码仍为 UTF-8
                                            0,                 // 同样不启用附加标志
                                            widePath.c_str(),  // 输入的宽字符数据
                                            -1,                // 使用以空字符结尾的字符串长度
                                            utf8.data(),       // 写入结果到预分配的 std::string 缓冲区
                                            sizeWithNull,      // 指定包含终止符的缓冲区大小
                                            nullptr,           // 不提供替代字符
                                            nullptr            // 不关心替代字符是否被使用
    );
    // 如果写入结果只有终止符说明转换失败
    if (written <= 1) {
        // 返回空字符串以表示转换失败
        return std::string{};
    }
    // 移除多余的终止符只保留有效 UTF-8 数据
    utf8.resize(static_cast<size_t>(written - 1));
    // 返回经过转换的 UTF-8 路径字符串
    return utf8;

// 其他平台可直接使用标准库提供的 UTF-8 接口
#else
// 如果编译器支持 char8_t 则返回 std::string 需要显式转换
#    if defined(__cpp_lib_char8_t)
    // 获取 char8_t 形式的 UTF-8 数据
    const std::u8string u8 = p.u8string();
    // 将 char8_t 范围转换为标准的 std::string
    return std::string(u8.begin(), u8.end());
// 对于 C++17 及不支持 char8_t 的实现可以直接返回 std::string
#    else
    // 直接返回标准库提供的 UTF-8 字符串表示
    return p.u8string();
#    endif  // 结束 char8_t 功能检测
#endif      // 结束平台分支
}

#ifdef _WIN32
// 将路径转换为 Windows 风格的反斜杠表示
std::string toWindowsStyle(const fs::path & p) {
    // 先转换为 UTF-8 字符串
    std::string s = toUtf8(p);
    // 将斜杠替换为反斜杠
    std::replace(s.begin(), s.end(), '/', '\\');
    // 返回转换后的字符串
    return s;
}
#endif  // _WIN32

int main() {
    // Windows 平台预处理块
#ifdef _WIN32
    // 设置控制台输出编码为 UTF-8 以便显示中文
    SetConsoleOutputCP(CP_UTF8);
    // 设置控制台输入编码为 UTF-8
    SetConsoleCP(CP_UTF8);
#endif  // _WIN32

    // 创建测试运行器对象
    qctest::Test   T;
    // 用于接收错误信息的字符串
    std::string    err;
    // 获取测试数据根路径
    const fs::path dataRoot = getDataRoot();
    // 测试用例：针对空路径的测试应该返回 false
    T.check(!validate_existing_path("", err), "针对空路径的测试没有返回 false，说明接口存在问题。");
    err.clear();
    // 测试用例：针对超过 4096 字节路径的测试应该返回 false
    T.check(!validate_existing_path(std::string(5000, 'a'), err),
            "针对超过 4096 字节路径的测试没有返回 false，说明接口存在问题。");
    err.clear();
    // 测试用例：针对 URL 形式路径的测试应该返回 false
    T.check(!validate_existing_path("https://示例.com/资源.txt", err),
            "针对 URL 形式路径的测试没有返回 false，说明接口存在问题。");
    err.clear();
    // 测试用例：针对包含空字节路径的测试应该返回 false
    std::string pathWithNull = toUtf8(dataRoot);   // 使用 toUtf8 提取根目录的 UTF-8 字符串表示
    pathWithNull.push_back('\0');
    pathWithNull += toUtf8(fs::u8path(u8"通用"));  // 追加 UTF-8 编码的“通用”目录名称
    T.check(!validate_existing_path(pathWithNull, err), "针对包含空字节路径的测试没有返回 false，说明接口存在问题。");
    err.clear();
    // 测试用例：针对包含控制字符路径的测试应该返回 false
    std::string pathWithControl = toUtf8(dataRoot);   // 使用 UTF-8 字符串作为路径基础
    pathWithControl.push_back('\t');
    pathWithControl += toUtf8(fs::u8path(u8"通用"));  // 追加包含中文的目录名称
    T.check(!validate_existing_path(pathWithControl, err), "针对包含控制字符路径的测试没有返回 false，说明接口存在问题。");
    err.clear();
    // 测试用例：针对包含..路径的测试应该返回 false
    std::string traversalAttempt = toUtf8(dataRoot);   // 以 UTF-8 字符串形式复制根路径
    traversalAttempt += "../";                         // 追加目录遍历片段
    traversalAttempt += toUtf8(fs::u8path(u8"通用"));  // 追加目标目录名称的 UTF-8 表示
    T.check(!validate_existing_path(traversalAttempt, err), "针对包含..路径的测试没有返回 false，说明接口存在问题。");
    err.clear();
    // 测试用例：针对不存在路径的测试应该返回 false
    const std::string missingFile =
        toUtf8(dataRoot / fs::u8path(u8"通用") / fs::u8path(u8"缺失文件.txt"));  // 通过 u8path 构建包含中文的路径
    T.check(!validate_existing_path(missingFile, err), "针对不存在的路径的测试没有返回 false，说明接口存在问题。");
    err.clear();
    // 测试用例：针对存在路径的测试应该返回 true
    const std::string existingFile =
        toUtf8(dataRoot / fs::u8path(u8"通用") / fs::u8path(u8"已存在文件.txt"));  // 构建指向已存在文件的 UTF-8 路径
    T.check(validate_existing_path(existingFile, err), "针对存在路径的测试没有返回 true，说明接口存在问题。");
    err.clear();
    // 测试用例：针对存在路径（目录）的测试应该返回 true
    const std::string existingDir =
        toUtf8(dataRoot / fs::u8path(u8"通用") / fs::u8path(u8"已存在目录"));  // 构建指向已存在目录的 UTF-8 路径
    T.check(validate_existing_path(existingDir, err), "针对存在路径（目录）的测试没有返回 true，说明接口存在问题。");
    err.clear();
    // 测试用例：针对包含空格路径的测试应该返回 true
    const std::string spacedFile = toUtf8(dataRoot / fs::u8path(u8"通用") / fs::u8path(u8"含空格 目录") /
                                          fs::u8path(u8"含空格 文件.txt"));  // 构建包含空格和中文的路径
    T.check(validate_existing_path(spacedFile, err), "针对包含空格路径的测试没有返回 true，说明接口存在问题。");
    err.clear();
    // 测试用例：针对包含中文字符路径的测试应该返回 true
    const std::string unicodeFile =
        toUtf8(dataRoot / fs::u8path(u8"通用") / fs::u8path(u8"含中文") / fs::u8path(u8"子目录") /
               fs::u8path(u8"文件.txt"));  // 构建包含多级中文目录的路径
    T.check(validate_existing_path(unicodeFile, err), "针对包含中文字符路径的测试没有返回 true，说明接口存在问题。");
    err.clear();

    // Windows 平台特定测试
#ifdef _WIN32
    // 测试用例：针对 Windows 平台反斜杠形式路径的测试应该返回 true
    const std::string windowsStyleFile =
        toWindowsStyle(dataRoot / fs::u8path(u8"Windows特定") /
                       fs::u8path(u8"已存在Windows路径.txt"));  // 使用 u8path 组合 Windows 特定测试路径
    T.check(validate_existing_path(windowsStyleFile, err),
            "针对 Windows 平台反斜杠形式路径的测试没有返回 true，说明接口存在问题。");
    err.clear();
    // 测试用例：针对 Windows 平台包含尖括号形式路径的测试应该返回 false
    const std::string windowsInvalidComponent =
        toWindowsStyle(dataRoot / fs::u8path(u8"Windows特定") /
                       fs::u8path(u8"含有<尖括号>.txt"));  // 构建包含非法字符的 Windows 测试路径
    T.check(!validate_existing_path(windowsInvalidComponent, err),
            "针对 Windows 平台包含尖括号形式路径的测试没有返回 false，说明接口存在问题。");
    err.clear();
    // 测试用例：针对 Windows 平台保留设备名相关路径的测试应该返回 false
    T.check(!validate_existing_path("NUL.txt", err),
            "针对 Windows 平台保留设备名相关路径的测试没有返回 false，说明接口存在问题。");
    err.clear();
    // 测试用例：针对 Windows 平台以空格结尾路径的测试应该返回 false
    const std::string trailingSpace = toWindowsStyle(dataRoot / fs::u8path(u8"Windows特定") /
                                                     fs::u8path(u8"结尾空格 "));  // 构建以空格结尾的 Windows 测试路径
    T.check(!validate_existing_path(trailingSpace, err),
            "针对 Windows 平台以空格结尾路径的测试没有返回 false，说明接口存在问题。");
    err.clear();

    // Linux 平台特定测试
#elif defined(__linux__)
    // 测试用例：针对 Linux 平台存在路径的测试应该返回 true
    const std::string linuxFile =
        toUtf8(dataRoot / fs::u8path(u8"Linux特定") / fs::u8path(u8"已存在Linux路径.txt"));  // 构建 Linux 平台专用路径
    T.check(validate_existing_path(linuxFile, err), "针对 Linux 平台存在路径的测试没有返回 true，说明接口存在问题。");
    err.clear();
    // 测试用例：针对 Linux 平台已存在绝对路径的测试应该返回 true
    const std::string linuxAbsolute =
        toUtf8(fs::absolute(dataRoot / fs::u8path(u8"Linux特定") /
                            fs::u8path(u8"已存在Linux路径.txt")));  // 构建 Linux 绝对路径并转换为 UTF-8
    T.check(validate_existing_path(linuxAbsolute, err), "针对 Linux 平台已存在绝对路径的测试没有返回 true，说明接口存在问题。");
    err.clear();
    // 测试用例：针对 Linux 平台使用 Windows 平台路径的测试应该返回 false
    T.check(!validate_existing_path("C:\\临时\\不存在.txt", err),
            "针对 Linux 平台使用 Windows 平台路径的测试应该返回 false，说明接口存在问题。");
    err.clear();

    // macOS 平台特定测试
#elif defined(__APPLE__)
    // 测试用例：针对 macOS 平台存在路径的测试应该返回 true
    const std::string macFile =
        toUtf8(dataRoot / fs::u8path(u8"macOS特定") / fs::u8path(u8"已存在macOS路径.txt"));  // 构建 macOS 平台专用路径
    T.check(validate_existing_path(macFile, err), "针对 macOS 平台存在路径的测试没有返回 true，说明接口存在问题。");
    err.clear();
    // 测试用例：针对 macOS 平台已存在绝对路径的测试应该返回 true
    const std::string macAbsolute =
        toUtf8(fs::absolute(dataRoot / fs::u8path(u8"macOS特定") /
                            fs::u8path(u8"已存在macOS路径.txt")));  // 构建 macOS 绝对路径并转换为 UTF-8
    T.check(validate_existing_path(macAbsolute, err), "针对 macOS 平台已存在绝对路径的测试没有返回 true，说明接口存在问题。");
    err.clear();
    // 测试用例：针对 macOS 平台使用 Windows 平台路径的测试应该返回 false
    T.check(!validate_existing_path("C:\\临时\\不存在.txt", err),
            "针对 macOS 平台使用 Windows 平台路径的测试没有返回 false，说明接口存在问题。");
    err.clear();
#endif
    // 汇总测试结果并返回退出码
    return T.finish();
}  // main
