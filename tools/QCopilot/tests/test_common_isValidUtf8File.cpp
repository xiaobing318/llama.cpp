#include "test_support.h"
#include "../builtinTools/common/common_utils.h"

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    auto root = qctest::make_temp_dir("qctest_common_utf8file_");
    auto ok = root/"ok.txt"; writeFileContent(ok.u8string(), "Hello 世界");
    auto bad = root/"bad.dat"; writeFileContent(bad.u8string(), std::string("A\xC0\xAFB",4));
    T.check(isValidUtf8File(ok.u8string()), "isValidUtf8File true");
    T.check(!isValidUtf8File(bad.u8string()), "isValidUtf8File false");
    return T.finish();
}

