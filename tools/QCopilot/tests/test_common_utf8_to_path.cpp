#include "test_support.h"
#include "../builtin_tools/common/common_utils.h"

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    auto root = qctest::make_temp_dir("qctest_common_utf8topath_");
    std::string s = (root / u8"子目录" / u8"文件.txt").u8string();
    fs::path p = utf8_to_path(s);
    T.check(path_to_utf8_string(p) == s, "utf8_to_path/path_to_utf8_string roundtrip");
    return T.finish();
}

