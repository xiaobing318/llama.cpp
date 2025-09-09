#include "test_support.h"
#include "../builtinTools/timeTools/get_current_time.h"
#include <algorithm>

using json = nlohmann::ordered_json;

int main(){
    qctest::Test T;

    auto j1 = BuiltinTools::TimeTools::executeGetCurrentTime({{"format","ISO8601"},{"timezone","UTC"}});
    T.check(j1.value("success", false) == true, "ISO8601 UTC success");
    qctest::expect_json_schema(j1, {{"success","boolean"},{"time","string"},{"format","string"},{"timezone","string"}}, T, "get_current_time schema");
    auto t1 = j1.value("time", std::string(""));
    T.check(!t1.empty() && t1.back()=='Z', "ISO8601 UTC ends with Z");

    auto j2 = BuiltinTools::TimeTools::executeGetCurrentTime({{"format","unix"}});
    T.check(j2.value("success", false) == true, "unix success");
    auto t2 = j2.value("time", std::string("0"));
    T.check(!t2.empty() && std::all_of(t2.begin(), t2.end(), ::isdigit), "unix numeric");

    auto j3 = BuiltinTools::TimeTools::executeGetCurrentTime({{"format","default"}});
    T.check(j3.value("success", false) == true, "default success");
    auto t3 = j3.value("time", std::string(""));
    T.check(t3.find('T') == std::string::npos && t3.find(' ') != std::string::npos, "default has space, no T");

    return T.finish();
}
