#include "test_support.h"
#include "../builtin_tools/common/common_utils.h"

using namespace BuiltinTools::Utils;

int main(){
    qctest::Test T;
    T.check(trim_string("  a ") == "a", "trim_string trims spaces");
    T.check(trim_string("\t\na\r") == "a", "trim_string trims whitespace set");
    T.check(trim_string("") == "", "trim_string empty");
    return T.finish();
}

