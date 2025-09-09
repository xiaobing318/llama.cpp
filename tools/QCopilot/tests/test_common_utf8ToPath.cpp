#include "test_support.h"
#include "../builtinTools/common/common_utils.h"

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    auto root = qctest::make_temp_dir("qctest_common_utf8topath_");
    std::string s = (root / u8"子目录" / u8"文件.txt").u8string();
    fs::path p = utf8ToPath(s);
    T.check(pathToUtf8String(p) == s, "utf8ToPath/pathToUtf8String roundtrip");
    return T.finish();
}

