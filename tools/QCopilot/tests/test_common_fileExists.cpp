#include "test_support.h"
#include "../builtinTools/common/common_utils.h"

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    fs::path root = qctest::make_temp_dir("qctest_common_exists_");

    fs::path f = root/"a.txt";
    T.check(!fileExists(f.u8string()), "fileExists false before create");
    writeFileContent(f.u8string(), "x");
    T.check(fileExists(f.u8string()), "fileExists true after create");

    return T.finish();
}

