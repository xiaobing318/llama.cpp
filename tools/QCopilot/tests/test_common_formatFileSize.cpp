#include "test_support.h"
#include "../builtinTools/common/common_utils.h"

using namespace BuiltinTools::Utils;

int main(){
    qctest::Test T;
    T.check(formatFileSize(0)    == "0.0 B",  "formatFileSize 0 B");
    T.check(formatFileSize(1023) == "1023.0 B", "formatFileSize 1023 B");
    T.check(formatFileSize(1024) == "1.0 KB",  "formatFileSize 1 KB");
    T.check(formatFileSize(1048576) == "1.0 MB", "formatFileSize 1 MB");
    return T.finish();
}

