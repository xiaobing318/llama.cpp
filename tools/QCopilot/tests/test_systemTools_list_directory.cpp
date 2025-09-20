#include "test_support.h"
#include "../builtin_tools/systemTools/list_directory.h"
#include "../builtin_tools/fileTools/write_text_file.h"

using json = nlohmann::ordered_json;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    fs::path root = qctest::make_temp_dir("qctest_listdir_");
    fs::create_directories(root / "sub");
    BuiltinTools::FileTools::run_write_text_file({{"path", (root/"a.txt").u8string()}, {"content","x"}});
    BuiltinTools::FileTools::run_write_text_file({{"path", (root/".secret").u8string()}, {"content","x"}});

    auto r1 = BuiltinTools::SystemTools::run_list_directory({{"path", root.u8string()}, {"recursive", false}, {"show_hidden", false}});
    T.check(r1.value("success", false) == true, "list_directory success");
    qctest::expect_json_schema(r1, {{"success","boolean"},{"path","string"},{"files","array"},{"count","integer"}}, T, "list_directory schema");
    bool seen_hidden=false; for (auto &it: r1["files"]) if (it["name"].get<std::string>()==".secret") seen_hidden=true;
    T.check(!seen_hidden, "dot file hidden");

    auto r2 = BuiltinTools::SystemTools::run_list_directory({{"path", root.u8string()}, {"recursive", true}, {"show_hidden", true}});
    T.check(r2.value("success", false) == true, "list_directory recursive success");
    bool seen_sub=false; for (auto &it: r2["files"]) if (it["name"].get<std::string>()=="sub") seen_sub=true;
    T.check(seen_sub, "subdir enumerated");

    return T.finish();
}
