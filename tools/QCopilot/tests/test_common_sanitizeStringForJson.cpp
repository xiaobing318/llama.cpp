#include "test_support.h"
#include "../builtinTools/common/common_utils.h"

using namespace BuiltinTools::Utils;
using json = nlohmann::ordered_json;

int main(){
    qctest::Test T;

    // sanitizeStringForJson: 控制字符替换为空格，非法 UTF-8 替换为 U+FFFD
    {
        std::string in = std::string("ABC\x01\x02\x7F\n\tZ") + std::string("\xC0\xAF",2);
        auto out = sanitizeStringForJson(in);
        T.check(out.find('\n')!=std::string::npos && out.find('\t')!=std::string::npos, "sanitize keeps LF/TAB");
        T.check(out.find('\x01')==std::string::npos && out.find('\x7F')==std::string::npos, "sanitize removes control chars");
        T.check(out.find("\xEF\xBF\xBD")!=std::string::npos, "sanitize inserts replacement char for invalid utf-8");
    }

    return T.finish();
}
