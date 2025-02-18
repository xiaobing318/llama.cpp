import pytest
# 导入pytest模块，为后续使用pytest的fixture和测试功能做准备。
from utils import *
# 从当前文件所在目录中名为utils.py文件中导入所有的内容，包含全局变量、函数、类，其中server_instances就是其中一个关键对象。

# ref: https://stackoverflow.com/questions/22627659/run-code-before-and-after-each-test-in-py-test
# 提供了这个参考链接，说明该fixture的模式参考了Stack Overflow上的相关讨论，便于开发者了解使用场景和设计思路。
@pytest.fixture(autouse=True)
def stop_server_after_each_test():
    # do nothing before each test
    yield
    # stop all servers after each test
    instances = set(
        server_instances
    )  # copy the set to prevent 'Set changed size during iteration'
    for server in instances:
        server.stop()
"""
Notes:杨小兵-2025-02-18

1、装饰器 @pytest.fixture(autouse=True)：定义一个 pytest fixture，并设置 autouse=True 表示该 fixture 会自动应用于每个测试函数，无需显式引用。
2、函数定义：stop_server_after_each_test 是一个自动运行的 fixture，用于在每个测试的前后执行特定操作。
3、该文件内容目前不理解，了解大概。
"""
