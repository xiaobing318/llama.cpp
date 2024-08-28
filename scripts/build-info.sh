#!/bin/sh

# `CC=$1`: 将脚本的第一个参数（$1）赋值给变量 CC，通常这个参数是编译器的命令，例如 `gcc` 或 `clang`。
CC=$1

# `build_number="0"`: 初始化变量 `build_number`，默认为 "0"。
build_number="0"
# `build_commit="unknown"`: 初始化变量 `build_commit`，默认值为 "unknown"
build_commit="unknown"
# `build_compiler="unknown"`: 初始化变量 `build_compiler`，默认值为 "unknown"
build_compiler="unknown"
# `build_target="unknown"`: 初始化变量 `build_target`，默认值为 "unknown"
build_target="unknown"

# `if out=$(git rev-list --count HEAD); then`: 检查 `git rev-list --count HEAD` 命令的执行结果。此命令用于计算当前 HEAD 指向的分支从开始到现在的提交数量
if out=$(git rev-list --count HEAD); then
    # git is broken on WSL so we need to strip extra newlines
    #  `# git is broken on WSL so we need to strip extra newlines`: 注释说明，由于在 Windows 子系统（WSL）中 Git 可能存在问题，需要删除额外的换行符
    build_number=$(printf '%s' "$out" | tr -d '\n')
fi

# `if out=$(git rev-parse --short HEAD); then`: 检查 `git rev-parse --short HEAD` 命令的执行结果，此命令用于获取最后一次提交的短格式哈希值
if out=$(git rev-parse --short HEAD); then
    #  `build_commit=$(printf '%s' "$out" | tr -d '\n')`: 同样使用 `printf` 和 `tr` 来处理输出，删除所有换行符，并将结果赋值给 `build_commit`
    build_commit=$(printf '%s' "$out" | tr -d '\n')
fi

# `if out=$($CC --version | head -1); then`: 使用变量 `CC` 指定的编译器命令获取编译器的版本信息，`head -1` 用于获取第一行的内容
if out=$($CC --version | head -1); then
    # `build_compiler=$out`: 将获取到的编译器版本信息赋值给 `build_compiler`
    build_compiler=$out
fi

# `if out=$($CC -dumpmachine); then`: 使用变量 `CC` 指定的编译器命令获取编译目标平台
if out=$($CC -dumpmachine); then
    # `build_target=$out`: 将获取到的目标平台信息赋值给 `build_target`
    build_target=$out
fi

# `echo "int LLAMA_BUILD_NUMBER = ${build_number};"` 等: 将所有收集到的版本信息以 C 语言的变量形式输出，这些输出可以被复制到 C 代码中，从而在程序中使用这些编译时确定的版本信息。
echo "int LLAMA_BUILD_NUMBER = ${build_number};"
echo "char const *LLAMA_COMMIT = \"${build_commit}\";"
echo "char const *LLAMA_COMPILER = \"${build_compiler}\";"
echo "char const *LLAMA_BUILD_TARGET = \"${build_target}\";"
