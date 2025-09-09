#include "test_support.h"
#include "../builtinTools/common/common_utils.h"

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    auto root = qctest::make_temp_dir("qctest_common_listdir_");
    fs::create_directories(root/"dir");
    writeFileContent((root/"dir"/"x").u8string(), "1");
    auto v = listDirectory((root/"dir").u8string());
    bool has=false; for (auto &n: v) if (n=="x") has=true;
    T.check(has, "listDirectory has x");
    return T.finish();
}

