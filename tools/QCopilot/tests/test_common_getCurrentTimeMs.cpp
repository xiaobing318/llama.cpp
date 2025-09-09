#include "test_support.h"
#include "../builtinTools/common/common_utils.h"
#include <thread>

using namespace BuiltinTools::Utils;

int main(){
    qctest::Test T;
    auto t0 = getCurrentTimeMs();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    auto t1 = getCurrentTimeMs();
    T.check(t1 >= t0, "getCurrentTimeMs monotonic");
    return T.finish();
}

