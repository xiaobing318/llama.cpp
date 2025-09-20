#include "test_support.h"
#include "../builtin_tools/common/common_utils_internal.h"

using namespace BuiltinTools::Utils::Internal;

int main(){
    qctest::Test T;
    T.check(glob_segment_match("ab.cpp","a?.cpp", true), "glob_segment_match ?");
    T.check(glob_segment_match("HELLO.CPP","*.cpp", false), "glob_segment_match icase");
    T.check(glob_segment_match("a7","a[0-9]", true), "glob_segment_match []");
    T.check(!glob_segment_match("a/b","*", true), "glob_segment_match does not cross slash");
    return T.finish();
}

