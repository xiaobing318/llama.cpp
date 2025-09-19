#include "test_support.h"
#include "../builtinTools/common/common_utils.h"
#include <vector>
#ifdef _WIN32
  #include <windows.h>
#endif
using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    // 在 Windows 平台上将终端的代码页（字符集编码）设置成 UTF-8 从而方便显示 unicode 字符
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    // 创建一个测试实例
    qctest::Test T;

    // 目前路径是写死的，后续可以看看其他项目是如何进行测试的
    std::string err;
    // 测试用例：针对空的路径，测试接口是否可以返回false
    T.check(!validatePath("", err), "针对空路径的测试，接口应该返回 false 而不是 true 。");
    err.clear();
    // 测试用例：针对超过 4096 个字符的路径，测试接口是否可以返回false
    T.check(!validatePath(std::string(5000, 'a'), err), "针对超过 4096 个字符路径的测试，接口应该返回 false 而不是 true 。");
    err.clear();
    // 测试用例：针对 URL 路径，测试接口是否可以返回false
    T.check(!validatePath("https://translate.google.com/?sl=en&tl=zh-CN&text=project%20synopsis&op=translate", err), "针对 URL 路径的测试，接口应该返回 false 而不是 true 。");
    err.clear();
    // 测试用例：针对包含空格路径，测试接口是否可以返回false
    T.check(!validatePath("F:/github_repos/llama.cpp/tools/QCopilot/ tests", err), "针对包含空格路径的测试，接口应该返回 false 而不是 true 。");
    err.clear();
    // 测试用例：针对包含..路径，测试接口是否可以返回false
    T.check(!validatePath("F:/github_repos/llama.cpp/tools/../QCopilot/tests", err), "针对包含..路径的测试，接口应该返回 false 而不是 true 。");
    err.clear();
    // 测试用例：检查给定路径（文件）是否存在，因为路径是存在的，因此测试接口应该返回 true
    T.check(validatePath("F:/github_repos/llama.cpp/tools/QCopilot/tests/test_common_validatePath.cpp", err), "因为路径（文件）是存在的，接口应该返回 true 而不是 false 。");
    err.clear();
    // 测试用例：检查给定路径（文件）是否存在，因为路径是不存在的，因此测试接口应该返回 false
    T.check(!validatePath("F:/github_repos/llama.cpp/tools/QCopilot/tests/demo_test_common_validatePath.cpp", err), "因为路径（文件）是不存在的，接口应该返回 false 而不是 true 。");
    err.clear();
    // 测试用例：检查给定路径（文件）是否存在，因为路径是存在的，因此测试接口应该返回 true
    T.check(validatePath("F:\\github_repos\\llama.cpp\\tools\\QCopilot\\tests\\test_common_validatePath.cpp", err), "因为路径（文件）是存在的，接口应该返回 true 而不是 false 。");
    err.clear();
    // 测试用例：检查给定路径（文件）是否存在，因为路径是不存在的，因此测试接口应该返回 false
    T.check(!validatePath("F:\\github_repos\\llama.cpp\\tools\\QCopilot\\tests\\demo_test_common_validatePath.cpp", err), "因为路径（文件）是不存在的，接口应该返回 false 而不是 true 。");
    err.clear();
    // 测试用例：检查给定路径（目录）是否存在，因为路径是存在的，因此测试接口应该返回 true
    T.check(validatePath("F:/github_repos/llama.cpp/tools/QCopilot/tests", err), "因为路径（目录）是存在的，接口应该返回 true 而不是 false 。");
    err.clear();
    // 测试用例：检查给定路径（目录）是否存在，因为路径是不存在的，因此测试接口应该返回 false
    T.check(!validatePath("F:/github_repos/llama.cpp/tools/QCopilot/demo_tests", err), "因为路径（目录）是不存在的，接口应该返回 false 而不是 true 。");
    err.clear();
    // 测试用例：检查给定路径（目录）是否存在，因为路径是存在的，因此测试接口应该返回 true
    T.check(validatePath("F:\\github_repos\\llama.cpp\\tools\\QCopilot\\tests", err), "因为路径（目录）是存在的，接口应该返回 true 而不是 false 。");
    err.clear();
    // 测试用例：检查给定路径（目录）是否存在，因为路径是不存在的，因此测试接口应该返回 false
    T.check(!validatePath("F:\\github_repos\\llama.cpp\\tools\\QCopilot\\demo_tests", err), "因为路径（目录）是不存在的，接口应该返回 false 而不是 true 。");
    err.clear();
    // 测试用例：检查给定包含中文路径（文件）是否存在，因为给定包含中文路径是存在的，因此测试接口应该返回 true
    T.check(validatePath("F:/GARBAGE_DATA/新建文本文档.txt", err), "因为给定包含中文路径（文件）是存在的，接口应该返回 true 而不是 false 。");
    err.clear();
    // 测试用例：检查给定包含中文路径（文件）是否存在，因为给定包含中文路径是不存在的，因此测试接口应该返回 false
    T.check(!validatePath("F:/GARBAGE_DATA/你好新建文本文档.txt", err), "因为给定包含中文路径（文件）是存在的，接口应该返回 false 而不是 true 。");
    err.clear();
    // 测试用例：检查给定包含中文路径（文件）是否存在，因为给定包含中文路径是存在的，因此测试接口应该返回 true
    T.check(validatePath("F:\\GARBAGE_DATA\\新建文本文档.txt", err), "因为给定包含中文路径（文件）是存在的，接口应该返回 true 而不是 false 。");
    err.clear();
    // 测试用例：检查给定包含中文路径（文件）是否存在，因为给定包含中文路径是不存在的，因此测试接口应该返回 false
    T.check(!validatePath("F:\\GARBAGE_DATA\\你好新建文本文档.txt", err), "因为给定包含中文路径（文件）是存在的，接口应该返回 false 而不是 true 。");
    err.clear();
    // 测试用例：检查给定包含中文路径（目录）是否存在，因为给定包含中文路径是存在的，因此测试接口应该返回 true
    T.check(validatePath("F:/杂项", err), "因为给定包含中文路径（目录）是存在的，接口应该返回 true 而不是 false 。");
    err.clear();
    // 测试用例：检查给定包含中文路径（目录）是否存在，因为给定包含中文路径是不存在的，因此测试接口应该返回 false
    T.check(validatePath("F:/你好杂项", err), "因为给定包含中文路径（目录）是不存在的，接口应该返回 false 而不是 true 。");
    err.clear();
    // 测试用例：检查给定包含中文路径（目录）是否存在，因为给定包含中文路径是存在的，因此测试接口应该返回 true
    T.check(validatePath("F:\\杂项", err), "因为给定包含中文路径（目录）是存在的，接口应该返回 true 而不是 false 。");
    err.clear();
    // 测试用例：检查给定包含中文路径（目录）是否存在，因为给定包含中文路径是不存在的，因此测试接口应该返回 false
    T.check(validatePath("F:\\你好杂项", err), "因为给定包含中文路径（目录）是不存在的，接口应该返回 false 而不是 true 。");
    err.clear();

    return T.finish();
}
