#include "test_support.h"
#include "../builtinTools/common/common_utils.h"
#include "../builtinTools/common/common_utils_internal.h"

using namespace BuiltinTools::Utils;
using namespace BuiltinTools::Utils::Internal;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    auto root = qctest::make_temp_dir("qctest_common_range_");
    auto f = root/"a.txt"; writeFileContent(f.u8string(), "A\nB\nC\n");
    std::string out; int lines=0, endline=0;
    bool ok = readTextFileWithRange(f.u8string(), 2, 3, out, lines, endline);
    T.check(ok && lines==2 && endline==3 && out==std::string("B\nC"), "readTextFileWithRange 2..3");
    return T.finish();
}
