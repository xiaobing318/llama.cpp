# 1 Server tests

Python based server tests scenario using [pytest](https://docs.pytest.org/en/stable/).

Tests target GitHub workflows job runners with 4 vCPU.

Note: If the host architecture inference speed is faster than GitHub runners one, parallel scenario may randomly fail.
To mitigate it, you can increase values in `n_predict`, `kv_size`.
```c
/*
Notes:杨小兵-2025-02-18

1、这部分内容解释的是如何来对llama-server可执行文件进行测试工作。
    1.1 这段内容说明在基于 Python 和 pytest 的服务器测试场景中，针对 GitHub workflows 的作业执行器（配置为4个 vCPU）进行测试时可能会遇到的问题。由于本地主机（或测试机）的推理速度可能比 GitHub 运行器快，在并行执行测试时可能会随机失败。为了缓解这种情况，建议增加参数 n_predict 和 kv_size 的值，从而使测试更加稳定。
2、相关概念解释
    2.1 Python based server tests scenario：指的是使用 Python 编写的服务器测试方案，这里使用的是 pytest 测试框架来组织和执行测试。
    2.2 pytest：一款流行的 Python 测试框架，提供简单易用的语法和丰富的插件支持，用于单元测试和集成测试等。
    2.3 GitHub workflows job runners：指的是 GitHub Actions 中用于运行工作流任务的执行器。这里的执行器配置为拥有 4 个虚拟 CPU（vCPU），即具有一定的计算能力。
    2.4 4 vCPU：vCPU 指的是虚拟中央处理单元，这里的 4 vCPU 意味着测试环境中分配了四个虚拟处理核心，用于并行处理任务。
    2.5 Host architecture inference speed：指的是本地主机或测试环境中进行模型推理（inference）的速度。如果该速度快于 GitHub 的运行器速度，可能会导致测试中并行任务的时间管理出现问题，从而引起随机失败。
    2.6 Parallel scenario：指在测试过程中采用并行（同时）运行多个测试任务的情况。并行执行虽然能提高效率，但如果环境之间的速度不一致，则可能引发竞态条件或时序问题。
    2.7 n_predict 与 kv_size 参数：这些参数与模型推理过程中涉及的计算量或内存分配有关。增大 n_predict 和 kv_size 的值可以调节测试过程中的计算负载，从而减少因速度差异带来的并行测试失败问题。
*/
```

### 1.1 Install dependencies

`pip install -r requirements.txt`
```c
/*
Notes:杨小兵-2025-02-18

1、使用基于python和pytest的服务器测试的时候需要进行安装的依赖库。
2、使用python解释器中的pip模块安装requirements.txt文件中指定的依赖。
*/
```

### 1.2 Run tests

1. Build the server

```shell
cd ../../..
cmake -B build -DLLAMA_CURL=ON
cmake --build build --target llama-server
```
```c
/*
Notes:杨小兵-2025-02-18

1、这部分内容解释的是如何运行llama-server可执行文件。
2、操作步骤
    2.1 在shell中使用cd命令切换当前所在目录。
    2.2 在shell中使用cmake来对llama.cpp项目进行构建。
    2.3 在shell中使用cmake来对llama.cpp项目中的llama-server项目进行构建操作。
3、总体：构建llama-server目标对象。
*/
```

2. Start the test: `./tests.sh`

It's possible to override some scenario steps values with environment variables:

| variable                 | description                                                                                    |
|--------------------------|------------------------------------------------------------------------------------------------|
| `PORT`                   | `context.server_port` to set the listening port of the server during scenario, default: `8080` |
| `LLAMA_SERVER_BIN_PATH`  | to change the server binary path, default: `../../../build/bin/llama-server`                         |
| `DEBUG`                  | to enable steps and server verbose mode `--verbose`                                       |
| `N_GPU_LAYERS`           | number of model layers to offload to VRAM `-ngl --n-gpu-layers`                                |
| `LLAMA_CACHE`            | by default server tests re-download models to the `tmp` subfolder. Set this to your cache (e.g. `$HOME/Library/Caches/llama.cpp` on Mac or `$HOME/.cache/llama.cpp` on Unix) to avoid this |
```c
/*
Notes:杨小兵-2025-02-18

1、这部分内容解释的时候如何使用tests.sh对llama-server进行测试工作。
2、可以使用环境变量覆盖某些场景步骤的值：
    2.1 PORT
    2.2 LLAMA_SERVER_BIN_PATH
    2.3 DEBUG
    2.4 N_GPU_LAYERS
    2.5 LLAMA_CACHE
3、总体：使用脚本文件tests.sh对llama-server进行测试工作。
*/
```

To run slow tests (will download many models, make sure to set `LLAMA_CACHE` if needed):

```shell
SLOW_TESTS=1 ./tests.sh
```
```c
/*
Notes:杨小兵-2025-02-18

1、这部分内容解释的是进行slow tests（这个操作将会下载很多的模型，如果必要的话请确保已经设置了LLAMA_CACHE环境变量）。
2、命令 `SLOW_TESTS=1 ./tests.sh` 的作用是设置环境变量 `SLOW_TESTS` 为 1，然后执行 `tests.sh` 脚本。设置这个变量会启用那些耗时较长的测试用例。
3、总体：通过设置环境变量SLOW_TESTS进行慢速测试。
*/
```

To run with stdout/stderr display in real time (verbose output, but useful for debugging):

```shell
DEBUG=1 ./tests.sh -s -v -x
```
```c
/*
Notes:杨小兵-2025-02-18

1、这部分内容解释的是要实时运行 stdout/stderr 显示（详细输出，但对于调试有用）
2、shell中的命令首先设置DEBUG环境变量，然后使用脚本文件进行测试工作。
3、总体：通过设置DEBUG环境变量进行测试工作。
*/
```

To run all the tests in a file:

```shell
./tests.sh unit/test_chat_completion.py.py -v -x
```
```c
/*
Notes:杨小兵-2025-02-18

1、这部分内容解释的是如何运行某个测试文件中所有的测试用例。命令中调用了 tests.sh 脚本，并传递了具体的测试文件名以及额外的选项参数，目的是执行该文件中的全部测试。
2、TODO:这里应该向llama.cpp上游仓库提一个PR（这里的命令出现问题）
*/
```

To run a single test:

```shell
./tests.sh unit/test_chat_completion.py::test_invalid_chat_completion_req
```
```c
/*
Notes:杨小兵-2025-02-18

1、这部分内容解释的是如何运行单个测试。
2、在shell中使用tests.sh脚本并且传入参数进行测试。
3、命令解释
    3.1 脚本调用：命令以 ./tests.sh 开头，说明使用一个 shell 脚本来启动测试过程。
    3.2 测试文件路径：unit/test_chat_completion.py 指明了要运行的测试文件所在的路径。
    3.3 测试用例选择：使用双冒号 :: 后跟具体的测试函数名称 test_invalid_chat_completion_req，这是一种 pytest 的语法，用于指定只执行这个文件中的某个测试用例。
*/
```

Hint: You can compile and run test in single command, useful for local developement:

```shell
cmake --build build -j --target llama-server && ./examples/server/tests/tests.sh
```

To see all available arguments, please refer to [pytest documentation](https://docs.pytest.org/en/stable/how-to/usage.html)
