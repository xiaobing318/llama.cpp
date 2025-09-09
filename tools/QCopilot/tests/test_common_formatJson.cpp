#include "test_support.h"
#include "../builtinTools/common/common_utils.h"

using namespace BuiltinTools::Utils;
using json = nlohmann::ordered_json;

int main(){
    qctest::Test T;
    auto s = formatJson(json{{"x",1}});
    T.check(!s.empty() && s.front()=='{' && s.back()=='}', "formatJson pretty");
    return T.finish();
}

