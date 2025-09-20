#include "test_support.h"
#include "../builtin_tools/common/common_utils.h"

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    auto root = qctest::make_temp_dir("qctest_common_readable_");
    auto f = root/"a.txt"; write_file_content(f.u8string(), "x");
    std::string err;
    T.check(is_regular_readable_file(f.u8string(), err), "is_regular_readable_file ok");
    auto d = root/"dir"; fs::create_directories(d);
    err.clear();
    T.check(!is_regular_readable_file(d.u8string(), err), "dir is not a regular file");
    return T.finish();
}

