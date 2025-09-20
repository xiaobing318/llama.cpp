#include "test_support.h"
#include "../builtin_tools/common/tool_response.h"

using namespace builtin_tools::common;

int main() {
    qctest::Test T;
    auto resp = make_success("unit_test");
    T.check(resp.value("success", false) == true, "make_success true");
    T.check(resp.value("tool", "") == "unit_test", "tool set");
    T.check(resp.value("truncated", true) == false, "truncated default false");
    T.check(resp.value("messages", qctest::ordered_json::array()).empty(), "messages empty by default");

    append_message(resp, "note");
    set_truncated(resp, true);
    T.check(resp["messages"].size() == 1 && resp["messages"][0] == "note", "append_message works");
    T.check(resp.value("truncated", false) == true, "set_truncated true");

    auto err = make_error("unit_test", "boom");
    T.check(err.value("success", true) == false, "make_error false");
    T.check(err.value("tool", "") == "unit_test", "error tool set");
    T.check(err.value("error", "") == "boom", "error message stored");
    T.check(err["messages"].is_array() && !err["messages"].empty(), "error messages seeded");

    return T.finish();
}
