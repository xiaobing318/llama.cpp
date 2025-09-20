#include "test_support.h"
#include "../builtin_tools/common/common_utils.h"
#include "../builtin_tools/common/common_utils_internal.h"
#include <vector>

using namespace BuiltinTools::Utils;
using namespace BuiltinTools::Utils::Internal;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    auto root = qctest::make_temp_dir("qctest_common_range_");
    auto f = root/"a.txt"; write_file_content(f.u8string(), "A\nB\nC\n");
    std::string out; int lines=0, endline=0; std::vector<std::string> selected;
    bool ok = read_text_with_range(f.u8string(), 2, 3, out, lines, endline, &selected);
    T.check(ok && lines==2 && endline==3 && out==std::string("B\nC"), "read_text_with_range 2..3 content");
    T.check(selected.size()==2 && selected[0]=="B" && selected[1]=="C", "read_text_with_range lines_out");
    return T.finish();
}
