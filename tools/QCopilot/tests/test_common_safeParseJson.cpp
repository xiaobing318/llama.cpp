#include "test_support.h"
#include "../builtinTools/common/common_utils.h"

using namespace BuiltinTools::Utils;
using json = nlohmann::ordered_json;

int main(){
    qctest::Test T;
    auto ok = safeParseJson("{\"a\":1}");
    T.check(ok.is_object() && ok["a"]==1, "safeParseJson ok");
    auto err = safeParseJson("{bad}");
    T.check(err.value("success", true)==false, "safeParseJson error");
    return T.finish();
}

