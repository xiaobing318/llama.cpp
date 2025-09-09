#include "test_support.h"
#include "../builtinTools/common/common_utils.h"

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    auto root = qctest::make_temp_dir("qctest_common_likelybin_");
    auto text = root/"t.txt"; writeFileContent(text.u8string(), "Hello\n");
    auto bin  = root/"b.bin"; writeFileContent(bin.u8string(), std::string("A\0B",3));
    T.check(!isLikelyBinary(text), "isLikelyBinary false on text");
    T.check(isLikelyBinary(bin), "isLikelyBinary true on bin");
    return T.finish();
}

