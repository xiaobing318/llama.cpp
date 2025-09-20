#include "test_support.h"
#include "../builtin_tools/common/common_utils_internal.h"

using namespace BuiltinTools::Utils::Internal;

int main(){
    qctest::Test T;
    T.check(match_char_class('a', "a-z", true), "match_char_class range");
    T.check(match_char_class('C', "[A-Z]"+std::string(""), true), "match_char_class class");
    T.check(match_char_class('A', "a", false), "match_char_class icase");
    T.check(!match_char_class('x', "!x", true), "match_char_class negate");
    return T.finish();
}

