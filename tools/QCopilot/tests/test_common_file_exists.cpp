#include "test_support.h"
#include "../builtin_tools/common/common_utils.h"

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    fs::path root = qctest::make_temp_dir("qctest_common_exists_");

    fs::path f = root/"a.txt";
    T.check(!file_exists(f.u8string()), "file_exists false before create");
    write_file_content(f.u8string(), "x");
    T.check(file_exists(f.u8string()), "file_exists true after create");

    return T.finish();
}

