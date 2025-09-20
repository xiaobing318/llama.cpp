#include "test_support.h"
#include "../builtin_tools/common/common_utils.h"

using namespace BuiltinTools::Utils;

int main(){
    qctest::Test T;
    T.check(!is_likely_binaryString("ABC"), "is_likely_binaryString false");
    T.check(is_likely_binaryString(std::string("A\0B",3)), "is_likely_binaryString true");
    return T.finish();
}

