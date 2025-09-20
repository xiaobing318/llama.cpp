#include "test_support.h"
#include "../builtin_tools/common/common_utils.h"

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    auto root = qctest::make_temp_dir("qctest_common_read_");
    auto f = root/"a.txt"; write_file_content(f.u8string(), "L1\nL2");
    std::string content;
    T.check(read_file_content(f.u8string(), content) && content==std::string("L1\nL2"), "read_file_content ok");
    return T.finish();
}

