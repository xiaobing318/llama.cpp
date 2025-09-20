#include "test_support.h"
#include "../builtin_tools/common/common_utils.h"

using nlohmann::ordered_json;
using namespace BuiltinTools::Utils;

int main(){
    qctest::Test T;
    const std::string tool = "unit_test_tool";

    auto j = make_success(tool);

    T.check(j.value("success", false) == true, "make_success sets success=true");
    T.check(j.value("tool", "") == tool, "make_success sets tool name");
    T.check(j.value("truncated", true) == false, "make_success defaults truncated=false");
    T.check(j.value("messages", ordered_json::array()).empty(),
            "make_success starts with empty messages array");

    return T.finish();
}
