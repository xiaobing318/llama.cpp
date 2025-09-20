#include "test_support.h"
#include "../builtin_tools/common/common_utils_internal.h"

using namespace BuiltinTools::Utils::Internal;

int main(){
    qctest::Test T;
    auto v = split_pattern_segments("a/b//c");
    T.check(v.size()==3 && v[0]=="a" && v[1]=="b" && v[2]=="c", "split_pattern_segments basic");
    return T.finish();
}

