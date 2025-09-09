#include "test_support.h"
#include "../builtinTools/common/common_utils.h"

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    auto root = qctest::make_temp_dir("qctest_common_append_");
    auto f = root/"a.txt";
    writeFileContent(f.u8string(), "X");
    T.check(appendFileContent(f.u8string(), "Y"), "appendFileContent ok");
    std::string s; readFileContent(f.u8string(), s);
    T.check(s=="XY", "appended content");
    return T.finish();
}

