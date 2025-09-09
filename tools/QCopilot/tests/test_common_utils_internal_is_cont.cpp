#include "test_support.h"
#include "../builtinTools/common/common_utils_internal.h"

using namespace BuiltinTools::Utils::Internal;

int main(){
    qctest::Test T;
    T.check(is_cont(0x80), "is_cont true for 10xxxxxx");
    T.check(!is_cont(0xC2), "is_cont false for leading byte");
    return T.finish();
}

