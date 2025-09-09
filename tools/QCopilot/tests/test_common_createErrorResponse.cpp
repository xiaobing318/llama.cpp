#include "test_support.h"
#include "../builtinTools/common/common_utils.h"

using nlohmann::ordered_json;
using namespace BuiltinTools::Utils;

int main(){
    qctest::Test T;
    auto j = createErrorResponse("oops");
    T.check(j.value("success", true)==false && j.value("error","") == "oops", "createErrorResponse fields");
    return T.finish();
}

