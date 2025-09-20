#include "test_support.h"
#include "../builtin_tools/common/common_utils.h"

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    auto root = qctest::make_temp_dir("qctest_common_likelybin_");
    auto text = root/"t.txt"; write_file_content(text.u8string(), "Hello\n");
    auto bin  = root/"b.bin"; write_file_content(bin.u8string(), std::string("A\0B",3));
    T.check(!is_likely_binary(text), "is_likely_binary false on text");
    T.check(is_likely_binary(bin), "is_likely_binary true on bin");
    return T.finish();
}

