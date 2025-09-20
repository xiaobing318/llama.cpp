#include "test_support.h"
#include "../builtin_tools/common/common_utils.h"

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    auto root = qctest::make_temp_dir("qctest_common_pathtoutf8_");
    fs::path p = root / u8"中文.txt";
    T.check(!path_to_utf8_string(p).empty(), "path_to_utf8_string non-empty");
    return T.finish();
}

