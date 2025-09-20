#include "test_support.h"
#include "../builtin_tools/common/common_utils.h"

#include <filesystem>
#include <fstream>

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main() {
    qctest::Test T;
    std::string err;

    const fs::path temp_dir = qctest::make_temp_dir("prepare_writable_");

    // 允许在已存在目录下创建新文件
    const fs::path new_file = temp_dir / "new_file.txt";
    err.clear();
    T.check(prepare_writable_path(new_file.string(), err), "prepare_writable_path accepts new file under existing directory");
    T.check(err.empty(), "prepare_writable_path success leaves error empty");

    // 当父目录不存在时应该失败
    const fs::path missing_parent = temp_dir / "missing" / "file.txt";
    err.clear();
    T.check(!prepare_writable_path(missing_parent.string(), err), "prepare_writable_path rejects missing parent directory");
    T.check(!err.empty(), "missing parent returns error message");

    // 现有目录本身不能作为可写文件目标
    const fs::path existing_dir = temp_dir / "existing_dir";
    fs::create_directory(existing_dir);
    err.clear();
    T.check(!prepare_writable_path(existing_dir.string(), err), "prepare_writable_path rejects directories");
    T.check(!err.empty(), "directory rejection returns message");

    // 已存在的文件允许覆盖
    const fs::path existing_file = temp_dir / "existing.txt";
    {
        std::ofstream ofs(existing_file);
        ofs << "data";
    }
    err.clear();
    T.check(prepare_writable_path(existing_file.string(), err), "prepare_writable_path accepts existing regular file");

    return T.finish();
}
