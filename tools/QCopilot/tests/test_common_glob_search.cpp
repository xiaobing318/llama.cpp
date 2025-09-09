#include "test_support.h"
#include "../builtinTools/common/common_utils.h"
#include <vector>

using namespace BuiltinTools::Utils;
namespace fs = std::filesystem;

int main(){
    qctest::Test T;
    fs::path root = qctest::make_temp_dir("qctest_common_glob_");
    fs::create_directories(root/"sub");
    writeFileContent((root/"a.txt").u8string(), "Hello\nworld\n");
    writeFileContent((root/"b.cpp").u8string(), "int main(){}\n");
    writeFileContent((root/"sub"/"c.hpp").u8string(), "#pragma once\n");

    // matchPattern 单段
    T.check(matchPattern("hello.cpp","*.cpp", true), "matchPattern *.cpp");
    T.check(matchPattern("HELLO.CPP","*.cpp", false), "matchPattern icase");
    T.check(matchPattern("ab","a?", true), "matchPattern ?");
    T.check(matchPattern("a7","a[0-9]", true), "matchPattern [] range");

    // globFiles 两段
    auto paths = globFiles(root, "**/*.cpp", /*include_directories=*/false, /*follow_symlinks=*/false, /*case_sensitive=*/true);
    bool has_bcpp=false; for (auto &p: paths) if (p.filename()=="b.cpp") has_bcpp=true; T.check(has_bcpp, "globFiles finds b.cpp");

    // searchInFileRegex：literal & regex
    int total=0; auto v1 = searchInFileRegex(root/"a.txt", "world", /*use_regex=*/false, /*cs=*/true, /*ln=*/true, total, 10);
    T.check(!v1.empty() && total>=1, "searchInFileRegex literal");
    total=0; auto v2 = searchInFileRegex(root/"a.txt", "^H.*o$", /*use_regex=*/true, /*cs=*/true, /*ln=*/true, total, 10);
    T.check(!v2.empty(), "searchInFileRegex regex");

    return T.finish();
}

