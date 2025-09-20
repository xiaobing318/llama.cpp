#include "test_support.h"
#include "../builtin_tools/common/common_utils.h"

using namespace BuiltinTools::Utils;
using json = nlohmann::ordered_json;

int main(){
    qctest::Test T;
    auto s = format_json(json{{"x",1}});
    T.check(!s.empty() && s.front()=='{' && s.back()=='}', "format_json pretty");
    return T.finish();
}

