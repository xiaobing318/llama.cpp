#include "test_support.h"
#include "../builtinTools/common/common_utils_internal.h"

using namespace BuiltinTools::Utils::Internal;

int main(){
    qctest::Test T;
    std::string in = "a\\b\\c";
    auto out = slashify(in);
    // 在 Windows 上应替换为 '/'; 其他平台不替换（保持无副作用）
#ifdef _WIN32
    T.check(out == "a/b/c", "slashify windows backslash->slash");
#else
    T.check(out == in, "slashify no-op on non-windows");
#endif
    return T.finish();
}

