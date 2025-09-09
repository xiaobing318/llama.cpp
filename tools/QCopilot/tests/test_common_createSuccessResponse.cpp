#include "test_support.h"
#include "../builtinTools/common/common_utils.h"

using nlohmann::ordered_json;
using namespace BuiltinTools::Utils;

int main(){
    qctest::Test T;
    auto j = createSuccessResponse();
    T.check(j.value("success", false)==true, "createSuccessResponse true");
    return T.finish();
}

