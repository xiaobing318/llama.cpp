#include "test_support.h"
#include "../builtinTools/common/common_utils.h"

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;

    T.check(matchPattern("hello.cpp","*.cpp", true), "matchPattern *.cpp");
    T.check(matchPattern("HELLO.CPP","*.cpp", false), "matchPattern icase");
    T.check(matchPattern("ab","a?", true), "matchPattern ?");
    T.check(matchPattern("a7","a[0-9]", true), "matchPattern [] range");

    return T.finish();
}
