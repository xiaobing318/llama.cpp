#include "test_support.h"
#include "../builtinTools/common/common_utils.h"

using namespace BuiltinTools::Utils;

int main(){
    qctest::Test T;
    std::vector<std::string> s = {"a","b","c"};
    T.check(joinStrings(s, "+") == "a+b+c", "joinStrings a+b+c");
    return T.finish();
}

