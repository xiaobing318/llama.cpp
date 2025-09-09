#include "test_support.h"
#include "../builtinTools/systemTools/path_stat.h"
#include "../builtinTools/fileTools/write_text_file.h"

using json = nlohmann::ordered_json;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    fs::path root = qctest::make_temp_dir("qctest_pathstat_");
    fs::create_directories(root/"sub");
    auto f = root/"doc.md";
    BuiltinTools::FileTools::executeWriteTextFile({{"path", f.u8string()}, {"content","标题\n内容\n"}});

    auto r1 = BuiltinTools::SystemTools::executePathStat({{"path", f.u8string()}, {"detailed", true}, {"text_analysis", true}});
    T.check(r1.value("success", false) == true, "path_stat file success");
    qctest::expect_json_schema(r1, {{"success","boolean"},{"path","string"},{"exists","boolean"},{"absolute_path","string"},{"filename","string"},{"type","string"}}, T, "path_stat schema");
    T.check(r1.value("type", std::string("")) == std::string("file"), "type=file");
    T.check(r1.contains("size"), "has size");
    T.check(r1.value("extension", std::string("")) == std::string(".md"), "extension .md");
    T.check(r1.contains("last_modified"), "has last_modified");

    // 目录
    auto r2 = BuiltinTools::SystemTools::executePathStat({{"path", root.u8string()}, {"detailed", true}});
    T.check(r2.value("success", false) == true, "path_stat dir success");
    T.check(r2.value("type", std::string("")) == std::string("directory"), "type=directory");

    // 不存在
    auto r3 = BuiltinTools::SystemTools::executePathStat({{"path", (root/"not-exist.txt").u8string()}});
    T.check(r3.value("success", true) == false, "non-existent rejected");

    return T.finish();
}
