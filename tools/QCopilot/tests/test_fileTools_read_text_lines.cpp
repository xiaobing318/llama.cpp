#include "test_support.h"
#include "../builtin_tools/fileTools/read_text_lines.h"
#include "json.hpp"

using json = nlohmann::ordered_json;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    fs::path root = qctest::make_temp_dir("qctest_readlines_");

    // 带 BOM + CRLF
    fs::path p = root / u8"带BOM_CRLF.txt";
    std::string bom = std::string("\xEF\xBB\xBF", 3);
    std::string content = bom + "L1\r\nL2\r\nL3\r\n";
    qctest::write_binary(p, content);

    auto r1 = BuiltinTools::FileTools::run_read_text_lines({
        {"path", p.u8string()},
        {"start_line", 2},
        {"end_line", 99},
        {"include_line_numbers", true},
        {"enforce_utf8", true}
    });
    T.check(r1.value("success", false) == true, "read_text_lines success");
    qctest::expect_json_schema(r1, {
        {"success","boolean"}, {"tool","string"}, {"path","string"}, {"encoding","string"},
        {"enforce_utf8","boolean"}, {"include_line_numbers","boolean"}, {"total_lines","integer"},
        {"range","object"}, {"content","string"}
    }, T, "read_text_lines schema");
    T.check(r1.value("total_lines", 0) == 3, "total_lines=3");
    T.check(r1["range"]["end_line"].get<int>() == 3, "end_line clipped to 3");
    T.check(r1.value("content", "") == std::string("L2\nL3\n"), "CRLF normalized, BOM skipped");
    T.check(r1["lines"].size() == 2, "lines size=2");

    // 错误：start_line < 1
    auto r2 = BuiltinTools::FileTools::run_read_text_lines({{"path", p.u8string()}, {"start_line", 0}});
    T.check(r2.value("success", true) == false, "invalid start_line rejected");

    // 空文件
    fs::path empty = root / "empty.txt";
    qctest::write_binary(empty, "");
    auto r3 = BuiltinTools::FileTools::run_read_text_lines({{"path", empty.u8string()}});
    T.check(r3.value("success", false) == true && r3.value("total_lines", 1) == 0, "empty file handled");

    // 二进制片段（含 NUL）
    fs::path pb = root / "nul.txt";
    qctest::write_binary(pb, std::string("A\nB\0C\n", 6));
    auto r4 = BuiltinTools::FileTools::run_read_text_lines({{"path", pb.u8string()}, {"start_line", 1}, {"end_line", 2}});
    T.check(r4.value("success", true) == false, "binary-like segment rejected");

    return T.finish();
}
