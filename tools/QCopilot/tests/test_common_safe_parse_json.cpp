#include "test_support.h"
#include "../builtin_tools/common/common_utils.h"

using namespace BuiltinTools::Utils;
using json = nlohmann::ordered_json;

int main(){
    qctest::Test T;
    auto ok = safe_parse_json("{\"a\":1}");
    T.check(ok.is_object() && ok["a"]==1, "safe_parse_json ok");
    auto err = safe_parse_json("{bad}");
    T.check(err.value("success", true)==false, "safe_parse_json error");
    return T.finish();
}

