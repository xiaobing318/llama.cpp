#include "test_support.h"
#include "../builtinTools/common/common_utils.h"

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    auto root = qctest::make_temp_dir("qctest_common_read_");
    auto f = root/"a.txt"; writeFileContent(f.u8string(), "L1\nL2");
    std::string content;
    T.check(readFileContent(f.u8string(), content) && content==std::string("L1\nL2"), "readFileContent ok");
    return T.finish();
}

