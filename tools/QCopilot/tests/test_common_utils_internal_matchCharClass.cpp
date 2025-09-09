#include "test_support.h"
#include "../builtinTools/common/common_utils_internal.h"

using namespace BuiltinTools::Utils::Internal;

int main(){
    qctest::Test T;
    T.check(matchCharClass('a', "a-z", true), "matchCharClass range");
    T.check(matchCharClass('C', "[A-Z]"+std::string(""), true), "matchCharClass class");
    T.check(matchCharClass('A', "a", false), "matchCharClass icase");
    T.check(!matchCharClass('x', "!x", true), "matchCharClass negate");
    return T.finish();
}

