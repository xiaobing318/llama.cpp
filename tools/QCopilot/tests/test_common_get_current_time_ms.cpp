#include "test_support.h"
#include "../builtin_tools/common/common_utils.h"
#include <thread>

using namespace BuiltinTools::Utils;

int main(){
    qctest::Test T;
    auto t0 = get_current_time_ms();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    auto t1 = get_current_time_ms();
    T.check(t1 >= t0, "get_current_time_ms monotonic");
    return T.finish();
}

