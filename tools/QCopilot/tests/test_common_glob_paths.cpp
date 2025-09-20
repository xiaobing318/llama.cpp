#include "test_support.h"
#include "../builtin_tools/common/common_utils.h"

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    auto root = qctest::make_temp_dir("qctest_common_globfiles_");
    fs::create_directories(root/"sub");
    write_file_content((root/"sub"/"a.cpp").u8string(), "int main(){}\n");
    auto v = glob_paths(root, "**/*.cpp", false, false, true);
    bool has=false; for (auto &p: v) if (p.filename()=="a.cpp") has=true;
    T.check(has, "glob_paths finds a.cpp");
    return T.finish();
}

