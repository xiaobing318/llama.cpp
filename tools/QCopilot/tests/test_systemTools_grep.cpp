#include "test_support.h"
#include "../builtin_tools/systemTools/grep.h"
#include "../builtin_tools/fileTools/write_text_file.h"
#include "json.hpp"

using json = nlohmann::ordered_json;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    fs::path root = qctest::make_temp_dir("qctest_grep_");
    BuiltinTools::FileTools::run_write_text_file({{"path", (root/"a.txt").u8string()}, {"content","Hello\nWorld\n"}});
    BuiltinTools::FileTools::run_write_text_file({{"path", (root/"b.txt").u8string()}, {"content","alpha\nbeta\n"}});

    // 单文件搜索
    auto r1 = BuiltinTools::SystemTools::run_grep({{"path", (root/"a.txt").u8string()}, {"pattern","Hello"}, {"line_numbers", true}});
    T.check(r1.value("success", false) == true, "grep single-file success");
    qctest::expect_json_schema(r1, {{"success","boolean"},{"path","string"},{"pattern","string"},{"results","array"}}, T, "grep schema");
    T.check(r1.value("matches_count", 0) >= 1, "grep found Hello");

    // 目录递归 + file_glob
    auto r2 = BuiltinTools::SystemTools::run_grep({
        {"path", root.u8string()},
        {"pattern", "beta"},
        {"recursive", true},
        {"file_glob", "**/*.txt"},
        {"max_matches", 10}
    });
    T.check(r2.value("success", false) == true, "grep recursive success");
    T.check(r2.value("matches_count", 0) >= 1, "grep found beta");

    // 正则
    auto r3 = BuiltinTools::SystemTools::run_grep({{"path", (root/"a.txt").u8string()}, {"pattern","^W.*d$"}, {"use_regex", true}});
    T.check(r3.value("success", false) == true, "grep regex success");
    T.check(r3.value("matches_count", 0) >= 1, "grep regex matched World");

    return T.finish();
}
