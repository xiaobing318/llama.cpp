#include "test_support.h"
#include "../builtin_tools/common/common_utils.h"

using namespace BuiltinTools::Utils;

static bool looks_like_ts(const std::string &s) {
    return s.size() >= 19 && s[4]=='-' && s[7]=='-' && (s[10]==' '||s[10]=='T') && s[13]==':' && s[16]==':';
}

int main(){
    qctest::Test T;
    auto now = std::chrono::system_clock::now();
    auto s = format_timestamp(now);
    T.check(looks_like_ts(s), "format_timestamp format");
    return T.finish();
}

