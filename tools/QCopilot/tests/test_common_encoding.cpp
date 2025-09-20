#include "test_support.h"
#include "../builtin_tools/common/common_utils.h"

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    fs::path root = qctest::make_temp_dir("qctest_common_encoding_");

    // is_valid_utf8_string
    T.check(is_valid_utf8_string("Hello 世界"), "is_valid_utf8_string ok");
    T.check(!is_valid_utf8_string(std::string("ASCII") + std::string("\xC0\xAF",2)), "is_valid_utf8_string detects invalid");

    // is_likely_binaryString
    T.check(!is_likely_binaryString("abc"), "is_likely_binaryString false on text");
    T.check(is_likely_binaryString(std::string("A\0B",3)), "is_likely_binaryString true on NUL");

    // files
    fs::path ok = root/"ok.txt"; write_file_content(ok.u8string(), "Hello 世界");
    fs::path bin = root/"bin.dat"; write_file_content(bin.u8string(), std::string("AB\0CD",5));

    T.check(is_valid_utf8_file(ok.u8string()), "is_valid_utf8_file ok");
    T.check(!is_valid_utf8_file(bin.u8string()), "is_valid_utf8_file false");
    T.check(!is_likely_binary(ok), "is_likely_binary false on text file");
    T.check(is_likely_binary(bin), "is_likely_binary true on binary file");

    return T.finish();
}

