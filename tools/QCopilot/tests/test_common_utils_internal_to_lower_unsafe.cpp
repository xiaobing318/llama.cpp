#include "test_support.h"
#include "../builtin_tools/common/common_utils_internal.h"

using namespace BuiltinTools::Utils::Internal;

int main(){
    qctest::Test T;
    T.check(to_lower_unsafe('A')=='a', "to_lower_unsafe A->a");
    T.check(to_lower_unsafe('z')=='z', "to_lower_unsafe z->z");
    return T.finish();
}

