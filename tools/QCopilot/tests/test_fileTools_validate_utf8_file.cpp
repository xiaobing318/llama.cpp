#include "test_support.h"
#include "../builtin_tools/fileTools/validate_utf8_file.h"

using json = nlohmann::ordered_json;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    fs::path root = qctest::make_temp_dir("qctest_valutf8_");

    // 合法 UTF-8
    fs::path ok = root / u8"合法utf8.txt";
    qctest::write_binary(ok, std::string("Hello 世界"));
    auto r1 = BuiltinTools::FileTools::run_validate_utf8_file({{"path", ok.u8string()}});
    T.check(r1.value("success", false) == true, "validate_utf8_file JSON success (ok)");
    T.check(r1.value("is_utf8", false) == true, "is_utf8 true");
    qctest::expect_json_schema(r1, {{"success","boolean"},{"is_utf8","boolean"},{"path","string"},{"file_size","integer"}}, T, "validate_utf8_file schema");

    // 非法 UTF-8（构造过长编码样式）
    fs::path bad = root / u8"非法utf8.bin";
    std::string bytes = std::string("ASCII") + std::string("\xC0\xAF",2) + "tail";
    qctest::write_binary(bad, bytes);
    auto r2 = BuiltinTools::FileTools::run_validate_utf8_file({{"path", bad.u8string()}});
    T.check(r2.value("success", false) == true, "validate_utf8_file JSON success (bad)");
    T.check(r2.value("is_utf8", true) == false, "is_utf8 false");

    // 错误：目录路径
    auto r3 = BuiltinTools::FileTools::run_validate_utf8_file({{"path", root.u8string()}});
    T.check(r3.value("success", true) == false, "directory rejected");

    return T.finish();
}
