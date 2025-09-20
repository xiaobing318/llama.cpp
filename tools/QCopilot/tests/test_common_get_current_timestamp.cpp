#include "test_support.h"
#include "../builtin_tools/common/common_utils.h"
#include <thread>

using namespace BuiltinTools::Utils;

static bool looks_like_ts(const std::string &s) {
    // YYYY-MM-DD HH:MM:SS
    return s.size() >= 19 && s[4]=='-' && s[7]=='-' && (s[10]==' '||s[10]=='T') && s[13]==':' && s[16]==':' ;
}

int main(){
    qctest::Test T;

    auto ts = get_current_timestamp();
    T.check(looks_like_ts(ts), "get_current_timestamp format");

    auto t0 = get_current_time_ms();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    auto t1 = get_current_time_ms();
    T.check(t1 >= t0, "get_current_time_ms monotonic");

    auto now = std::chrono::system_clock::now();
    auto s = format_timestamp(now);
    T.check(looks_like_ts(s), "format_timestamp format");

    return T.finish();
}

