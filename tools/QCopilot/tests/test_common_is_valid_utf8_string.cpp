#include "test_support.h"
#include "../builtin_tools/common/common_utils.h"

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    T.check(is_valid_utf8_string("Hello 世界"), "is_valid_utf8_string ok");
    T.check(!is_valid_utf8_string(std::string("ASCII") + std::string("\xC0\xAF",2)), "is_valid_utf8_string detects invalid");
    return T.finish();
}
