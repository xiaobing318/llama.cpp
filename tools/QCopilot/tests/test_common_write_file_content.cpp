#include "test_support.h"
#include "../builtin_tools/common/common_utils.h"

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    auto root = qctest::make_temp_dir("qctest_common_write_");
    auto f = root/"a.txt";
    T.check(write_file_content(f.u8string(), "X"), "write_file_content ok");
    std::string s; read_file_content(f.u8string(), s);
    T.check(s=="X", "written content");
    return T.finish();
}

