#include "test_support.h"
#include "../builtinTools/common/common_utils_internal.h"

using namespace BuiltinTools::Utils::Internal;

int main(){
    qctest::Test T;
    std::string s;
    append_replacement_char(s);
    T.check(s.size()==3 && (unsigned char)s[0]==0xEF && (unsigned char)s[1]==0xBF && (unsigned char)s[2]==0xBD,
            "append_replacement_char adds U+FFFD in UTF-8");
    return T.finish();
}

