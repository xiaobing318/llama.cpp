#include "test_support.h"
#include "../builtinTools/common/common_utils.h"

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    auto root = qctest::make_temp_dir("qctest_common_ifstream_");
    auto f = root / u8"中文.txt"; writeFileContent(f.u8string(), "data");
    std::ifstream ifs = open_ifstream_unicode(f.u8string(), std::ios::binary);
    std::string s((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    T.check(s=="data", "open_ifstream_unicode read");
    return T.finish();
}

