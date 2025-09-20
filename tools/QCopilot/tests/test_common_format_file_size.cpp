#include "test_support.h"
#include "../builtin_tools/common/common_utils.h"

using namespace BuiltinTools::Utils;

int main(){
    qctest::Test T;
    T.check(format_file_size(0)    == "0.0 B",  "format_file_size 0 B");
    T.check(format_file_size(1023) == "1023.0 B", "format_file_size 1023 B");
    T.check(format_file_size(1024) == "1.0 KB",  "format_file_size 1 KB");
    T.check(format_file_size(1048576) == "1.0 MB", "format_file_size 1 MB");
    return T.finish();
}

