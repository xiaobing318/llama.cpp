#include "test_support.h"
#include "../builtin_tools/common/common_utils.h"

using nlohmann::ordered_json;
using namespace BuiltinTools::Utils;

int main(){
    qctest::Test T;
    const std::string tool = "unit_test_tool";
    const std::string message = "oops";

    auto j = make_error(tool, message);

    T.check(j.value("success", true) == false, "make_error sets success=false");
    T.check(j.value("error", "") == message, "make_error propagates error message");
    T.check(j.value("tool", "") == tool, "make_error sets tool name");
    T.check(j.value("truncated", true) == false, "make_error defaults truncated=false");

    auto messages = j.value("messages", ordered_json::array());
    T.check(messages.is_array() && !messages.empty() && messages[0] == message,
            "make_error seeds messages array with error text");

    return T.finish();
}
