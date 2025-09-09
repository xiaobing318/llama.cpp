#include "test_support.h"
#include "../builtinTools/common/common_utils.h"

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    T.check(isValidUtf8String("Hello 世界"), "isValidUtf8String ok");
    T.check(!isValidUtf8String(std::string("ASCII") + std::string("\xC0\xAF",2)), "isValidUtf8String detects invalid");
    return T.finish();
}
