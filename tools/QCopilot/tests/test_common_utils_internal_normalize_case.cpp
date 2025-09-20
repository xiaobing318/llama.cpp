#include "test_support.h"
#include "../builtin_tools/common/common_utils_internal.h"

using namespace BuiltinTools::Utils::Internal;

int main(){
    qctest::Test T;
    T.check(normalize_case("AbC", false)=="abc", "normalize_case lower when not case_sensitive");
    T.check(normalize_case("AbC", true)=="AbC",  "normalize_case keep when case_sensitive");
    return T.finish();
}

