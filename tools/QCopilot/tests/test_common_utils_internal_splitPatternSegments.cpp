#include "test_support.h"
#include "../builtinTools/common/common_utils_internal.h"

using namespace BuiltinTools::Utils::Internal;

int main(){
    qctest::Test T;
    auto v = splitPatternSegments("a/b//c");
    T.check(v.size()==3 && v[0]=="a" && v[1]=="b" && v[2]=="c", "splitPatternSegments basic");
    return T.finish();
}

