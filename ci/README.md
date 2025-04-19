# CI

In addition to [Github Actions](https://github.com/ggml-org/llama.cpp/actions) `llama.cpp` uses a custom CI framework:

https://github.com/ggml-org/ci
```c
/*
Notes:杨小兵-2025-04-19

1、除了github actions这个CI框架，llama.cpp项目同时提供了一个自定义的CI框架。这个自定义CI框架在ggml-org组织中。
*/
```
It monitors the `master` branch for new commits and runs the
[ci/run.sh](https://github.com/ggml-org/llama.cpp/blob/master/ci/run.sh) script on dedicated cloud instances. This allows us
to execute heavier workloads compared to just using Github Actions. Also with time, the cloud instances will be scaled
to cover various hardware architectures, including GPU and Apple Silicon instances.
```c
/*
Notes:杨小兵-2025-04-19

1、这个自定义的CI框架监控了master分支，如果有新的commit提交到master分支那么将会在专门的云服务器上运行ci/run.sh脚本。相对github actions来说这将允许我们执行更加繁重的负载。随着时间的推移，云服务器将扩展至覆盖各种硬件架构，包括 GPU 和 Apple Silicon 实例。
2、目前提交到master分支之后会进行上述流程从而实现更加全面的构建、测试。
*/
```

Collaborators can optionally trigger the CI run by adding the `ggml-ci` keyword to their commit message.
Only the branches of this repo are monitored for this keyword.
```c
/*
Notes:杨小兵-2025-04-19

1、协作者可以通过向commit信息中添加ggml-ci关键词从而有选择性的触发CI运行。
2、这一点功能还没有尝试过。
*/
```

It is a good practice, before publishing changes to execute the full CI locally on your machine:
```c
/*
Notes:杨小兵-2025-04-19

1、提交代码更改之前在本地执行全量的CI将会是一个好的实践。
*/
```

```bash
mkdir tmp

# CPU-only build
bash ./ci/run.sh ./tmp/results ./tmp/mnt

# with CUDA support
GG_BUILD_CUDA=1 bash ./ci/run.sh ./tmp/results ./tmp/mnt

# with SYCL support
source /opt/intel/oneapi/setvars.sh
GG_BUILD_SYCL=1 bash ./ci/run.sh ./tmp/results ./tmp/mnt
```
