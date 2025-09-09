#include "test_support.h"
#include "../builtinTools/common/common_utils.h"

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    fs::path root = qctest::make_temp_dir("qctest_common_encoding_");

    // isValidUtf8String
    T.check(isValidUtf8String("Hello 世界"), "isValidUtf8String ok");
    T.check(!isValidUtf8String(std::string("ASCII") + std::string("\xC0\xAF",2)), "isValidUtf8String detects invalid");

    // isLikelyBinaryString
    T.check(!isLikelyBinaryString("abc"), "isLikelyBinaryString false on text");
    T.check(isLikelyBinaryString(std::string("A\0B",3)), "isLikelyBinaryString true on NUL");

    // files
    fs::path ok = root/"ok.txt"; writeFileContent(ok.u8string(), "Hello 世界");
    fs::path bin = root/"bin.dat"; writeFileContent(bin.u8string(), std::string("AB\0CD",5));

    T.check(isValidUtf8File(ok.u8string()), "isValidUtf8File ok");
    T.check(!isValidUtf8File(bin.u8string()), "isValidUtf8File false");
    T.check(!isLikelyBinary(ok), "isLikelyBinary false on text file");
    T.check(isLikelyBinary(bin), "isLikelyBinary true on binary file");

    return T.finish();
}

