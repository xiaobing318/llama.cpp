#include "test_support.h"
#include "../builtinTools/common/common_utils.h"
#include <thread>

using namespace BuiltinTools::Utils;

static bool looks_like_ts(const std::string &s) {
    // YYYY-MM-DD HH:MM:SS
    return s.size() >= 19 && s[4]=='-' && s[7]=='-' && (s[10]==' '||s[10]=='T') && s[13]==':' && s[16]==':' ;
}

int main(){
    qctest::Test T;

    auto ts = getCurrentTimestamp();
    T.check(looks_like_ts(ts), "getCurrentTimestamp format");

    auto t0 = getCurrentTimeMs();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    auto t1 = getCurrentTimeMs();
    T.check(t1 >= t0, "getCurrentTimeMs monotonic");

    auto now = std::chrono::system_clock::now();
    auto s = formatTimeStamp(now);
    T.check(looks_like_ts(s), "formatTimeStamp format");

    return T.finish();
}

