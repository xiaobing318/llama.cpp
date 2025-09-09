#include "test_support.h"
#include "../builtinTools/common/common_utils.h"

using namespace BuiltinTools::Utils;

int main(){
    qctest::Test T;
    T.check(!isLikelyBinaryString("ABC"), "isLikelyBinaryString false");
    T.check(isLikelyBinaryString(std::string("A\0B",3)), "isLikelyBinaryString true");
    return T.finish();
}

