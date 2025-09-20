#include "test_support.h"
#include "../builtin_tools/common/common_utils.h"

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    auto root = qctest::make_temp_dir("qctest_common_listdir_");
    fs::create_directories(root/"dir");
    write_file_content((root/"dir"/"x").u8string(), "1");
    auto v = list_directory((root/"dir").u8string());
    bool has=false; for (auto &n: v) if (n=="x") has=true;
    T.check(has, "list_directory has x");
    return T.finish();
}

