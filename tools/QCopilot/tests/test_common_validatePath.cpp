#include "test_support.h"
#include "../builtinTools/common/common_utils.h"
#include <vector>

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    fs::path root = qctest::make_temp_dir("qctest_common_fs_");

    // validatePath
    {
        std::string err;
        T.check(!validatePath("", err) && !err.empty(), "validatePath rejects empty");
        err.clear();
        T.check(!validatePath("../escape", err), "validatePath rejects traversal");
    }

    return T.finish();
}
