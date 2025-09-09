#include "test_support.h"
#include "../builtinTools/common/common_utils.h"

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    auto root = qctest::make_temp_dir("qctest_common_write_");
    auto f = root/"a.txt";
    T.check(writeFileContent(f.u8string(), "X"), "writeFileContent ok");
    std::string s; readFileContent(f.u8string(), s);
    T.check(s=="X", "written content");
    return T.finish();
}

