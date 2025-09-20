#include "test_support.h"
#include "../builtin_tools/common/common_utils.h"

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    auto root = qctest::make_temp_dir("qctest_common_utf8file_");
    auto ok = root/"ok.txt"; write_file_content(ok.u8string(), "Hello 世界");
    auto bad = root/"bad.dat"; write_file_content(bad.u8string(), std::string("A\xC0\xAFB",4));
    T.check(is_valid_utf8_file(ok.u8string()), "is_valid_utf8_file true");
    T.check(!is_valid_utf8_file(bad.u8string()), "is_valid_utf8_file false");
    return T.finish();
}

