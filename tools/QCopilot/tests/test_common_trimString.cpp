#include "test_support.h"
#include "../builtinTools/common/common_utils.h"

using namespace BuiltinTools::Utils;

int main(){
    qctest::Test T;
    T.check(trimString("  a ") == "a", "trimString trims spaces");
    T.check(trimString("\t\na\r") == "a", "trimString trims whitespace set");
    T.check(trimString("") == "", "trimString empty");
    return T.finish();
}

