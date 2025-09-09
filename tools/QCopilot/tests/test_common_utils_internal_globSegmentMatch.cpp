#include "test_support.h"
#include "../builtinTools/common/common_utils_internal.h"

using namespace BuiltinTools::Utils::Internal;

int main(){
    qctest::Test T;
    T.check(globSegmentMatch("ab.cpp","a?.cpp", true), "globSegmentMatch ?");
    T.check(globSegmentMatch("HELLO.CPP","*.cpp", false), "globSegmentMatch icase");
    T.check(globSegmentMatch("a7","a[0-9]", true), "globSegmentMatch []");
    T.check(!globSegmentMatch("a/b","*", true), "globSegmentMatch does not cross slash");
    return T.finish();
}

