#include "test_support.h"
#include "../builtin_tools/common/common_utils.h"

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    auto root = qctest::make_temp_dir("qctest_common_search_");
    auto f = root/"a.txt"; write_file_content(f.u8string(), "Hello\nWorld\n");
    int total=0; auto v1 = search_in_file_regex(f, "World", /*use_regex=*/false, /*cs=*/true, /*ln=*/true, total, 10);
    T.check(!v1.empty() && total>=1, "search_in_file_regex literal");
    total=0; auto v2 = search_in_file_regex(f, "^H.*o$", /*use_regex=*/true, /*cs=*/true, /*ln=*/true, total, 10);
    T.check(!v2.empty(), "search_in_file_regex regex");
    return T.finish();
}

