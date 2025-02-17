#!/bin/bash
# 杨小兵-2025-02-18：指定解释器为 Bash，确保脚本在 Bash 环境下运行。

# make sure we are in the right directory
# 杨小兵-2025-02-18：确保位于正确的目录中
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
# 杨小兵-2025-02-18：获取脚本自身所在的目录（第一步通过dirname获取当前脚本文件的目录路径；第二步：切换到该目录并获取绝对路径，结果赋值给变量 SCRIPT_DIR）
cd $SCRIPT_DIR
# 杨小兵-2025-02-18：切换当前工作目录到脚本所在目录，确保后续命令以脚本目录为基准。

set -eu
# 杨小兵-2025-02-18：开启两个 Bash 选项（-e：当某个命令返回非零状态时，立即退出脚本；-u：使用未定义的变量时报错退出，从而避免潜在错误。）

if [[ "${SLOW_TESTS:-0}" == 1 ]]; then
    # 杨小兵-2025-02-18：检查环境变量 SLOW_TESTS 是否被设置为 1，如果未定义则默认设置成0。

    # Slow tests for tool calls need quite a few models ahead of time to avoid timing out.
    # 杨小兵-2025-02-18：注释说明慢速测试需要预先下载大量模型，以防测试超时。
    python $SCRIPT_DIR/../../../scripts/fetch_server_test_models.py
    # 杨小兵-2025-02-18：使用python解释器运行特定位置中的python脚本（用来预先下载模型文件）
fi
# 杨小兵-2025-02-18：结束检查 SLOW_TESTS 的 if 块

if [ $# -lt 1 ]
# 杨小兵-2025-02-18：判断传递给脚本的参数数量是否少于 1，即是否没有传入任何参数,$# 表示传递给脚本的参数个数。
then
# 杨小兵-2025-02-18： 如果没有传入参数，则进入以下分支。
    if [[ "${SLOW_TESTS:-0}" == 1 ]]; then
    # 杨小兵-2025-02-18：在没有传入参数的情况下，再次检查 SLOW_TESTS 是否等于 1。
        pytest -v -x
        # 杨小兵-2025-02-18： 如果 SLOW_TESTS 为 1，直接运行 pytest，选项说明--->-v：详细模式（verbose）;-x：一旦遇到错误，立即退出测试。
    else
    # 杨小兵-2025-02-18：如果 SLOW_TESTS 不为 1，则进入 else 分支。
        pytest -v -x -m "not slow"
        # 杨小兵-2025-02-18：运行 pytest，但排除标记为 slow 的测试，-m "not slow"：只运行没有 slow 标记的测试用例。
    fi
    # 杨小兵-2025-02-18：结束内部的 if-else 块。
else
# 杨小兵-2025-02-18：如果传入了参数（即参数个数不少于 1），则执行以下命令。
    pytest "$@"
    # 杨小兵-2025-02-18：使用传入的所有参数直接调用 pytest，"$@" 表示将所有命令行参数原样传递给 pytest，允许用户自定义测试的运行方式。
fi
# 杨小兵-2025-02-18：判断传递给脚本的参数数量是否少于 1，即是否没有传入任何参数
