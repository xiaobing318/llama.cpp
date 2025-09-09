#include "test_support.h"
#include "../builtinTools/common/common_utils.h"

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    auto root = qctest::make_temp_dir("qctest_common_search_");
    auto f = root/"a.txt"; writeFileContent(f.u8string(), "Hello\nWorld\n");
    int total=0; auto v1 = searchInFileRegex(f, "World", /*use_regex=*/false, /*cs=*/true, /*ln=*/true, total, 10);
    T.check(!v1.empty() && total>=1, "searchInFileRegex literal");
    total=0; auto v2 = searchInFileRegex(f, "^H.*o$", /*use_regex=*/true, /*cs=*/true, /*ln=*/true, total, 10);
    T.check(!v2.empty(), "searchInFileRegex regex");
    return T.finish();
}

