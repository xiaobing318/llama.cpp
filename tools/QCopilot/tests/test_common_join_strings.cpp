#include "test_support.h"
#include "../builtin_tools/common/common_utils.h"

using namespace BuiltinTools::Utils;

int main(){
    qctest::Test T;
    std::vector<std::string> s = {"a","b","c"};
    T.check(join_strings(s, "+") == "a+b+c", "join_strings a+b+c");
    return T.finish();
}

