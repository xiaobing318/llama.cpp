#include "test_support.h"
#include "../builtin_tools/fileTools/write_text_file.h"

using json = nlohmann::ordered_json;
namespace fs = std::filesystem;

int main() {
    qctest::Test T;
    fs::path root = qctest::make_temp_dir("qctest_write_");

    // 基本写入（中文路径）
    fs::path p = root / u8"写入测试.txt";
    json j = {
        {"path", p.u8string()},
        {"content", "Hello\n世界"},
        {"append", false}
    };
    auto r = BuiltinTools::FileTools::run_write_text_file(j);
    T.check(r.value("success", false) == true, "write_text_file success");
    qctest::expect_json_schema(r, {
        {"success","boolean"},{"path","string"},{"bytes_written","integer"},{"mode","string"},{"file_existed","boolean"}
    }, T, "write_text_file schema");
    T.check(fs::exists(p), "file exists after write");

    // 读取比对
    std::ifstream ifs(p, std::ios::binary);
    std::string buf((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    T.check(buf == std::string("Hello\n世界"), "content matches");

    // 追加
    json j2 = {{"path", p.u8string()}, {"content", "\nAPPEND"}, {"append", true}};
    auto r2 = BuiltinTools::FileTools::run_write_text_file(j2);
    T.check(r2.value("success", false) == true, "append success");
    std::ifstream ifs2(p, std::ios::binary);
    std::string buf2((std::istreambuf_iterator<char>(ifs2)), std::istreambuf_iterator<char>());
    T.check(buf2.size() > buf.size() && buf2.find("APPEND") != std::string::npos, "appended content present");

    // 错误：空路径
    auto r3 = BuiltinTools::FileTools::run_write_text_file({{"path",""},{"content","x"}});
    T.check(r3.value("success", true) == false, "empty path rejected");

    return T.finish();
}
