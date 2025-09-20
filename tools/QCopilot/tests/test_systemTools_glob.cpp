#include "test_support.h"
#include "../builtin_tools/systemTools/glob.h"
#include "../builtin_tools/fileTools/write_text_file.h"

using json = nlohmann::ordered_json;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    fs::path root = qctest::make_temp_dir("qctest_glob_");
    fs::create_directories(root / "sub");
    BuiltinTools::FileTools::run_write_text_file({{"path", (root/"a.txt").u8string()}, {"content","x"}});
    BuiltinTools::FileTools::run_write_text_file({{"path", (root/"b.cpp").u8string()}, {"content","int main(){}\n"}});
    BuiltinTools::FileTools::run_write_text_file({{"path", (root/".hidden").u8string()}, {"content","secret"}});
    BuiltinTools::FileTools::run_write_text_file({{"path", (root/"sub"/u8"中文.md").u8string()}, {"content","标题"}});

    // 匹配 cpp
    auto r1 = BuiltinTools::SystemTools::run_glob({
        {"base_dir", root.u8string()},
        {"pattern", "**/*.cpp"},
        {"include_directories", false}
    });
    T.check(r1.value("success", false) == true, "glob success");
    qctest::expect_json_schema(r1, {{"success","boolean"},{"base_dir","string"},{"pattern","string"},{"items","array"}}, T, "glob schema");
    bool has_cpp=false; for (auto &it: r1["items"]) if (it["name"].get<std::string>()=="b.cpp") has_cpp=true;
    T.check(has_cpp, "glob finds b.cpp");

    // 不显示隐藏
    auto r2 = BuiltinTools::SystemTools::run_glob({
        {"base_dir", root.u8string()},
        {"pattern", "*"},
        {"show_hidden", false}
    });
    bool seen_hidden=false; for (auto &it: r2["items"]) if (it["name"].get<std::string>()==".hidden") seen_hidden=true;
    T.check(!seen_hidden, "glob hides dot files by default");

    // 显式要求显示隐藏
    auto r3 = BuiltinTools::SystemTools::run_glob({
        {"base_dir", root.u8string()},
        {"pattern", "*"},
        {"show_hidden", true}
    });
    bool has_hidden=false; for (auto &it: r3["items"]) if (it["name"].get<std::string>()==".hidden") has_hidden=true;
    T.check(has_hidden, "glob with show_hidden=true exposes dot files");

    return T.finish();
}
