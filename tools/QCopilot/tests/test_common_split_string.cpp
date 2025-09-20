#include "test_support.h"
#include "../builtin_tools/common/common_utils.h"

using namespace BuiltinTools::Utils;

int main(){
    qctest::Test T;
    auto v = split_string(" a, b ,c ", ',');
    T.check(v.size()==3, "split size 3");
    T.check(v[0]==" a" && v[1]==" b " && v[2]=="c ", "split keeps raw tokens");
    return T.finish();
}

