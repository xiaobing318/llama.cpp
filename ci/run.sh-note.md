*2025-04-19-杨小兵*

## 概要
`ci/run.sh` 是 llama.cpp 项目的**自定义 CI 驱动脚本**，用于在本地或云端专用实例上以不同配置（CPU、GPU、SYCL、Vulkan 等）**构建并测试**项目，可输出详尽日志与验收报告。

---

## 支持的构建后端

1. **CPU‑only 构建**
   ```bash
   bash ./ci/run.sh ./tmp/results ./tmp/mnt
   ```
   不设置任何额外环境变量，即进行纯 CPU 模式编译和测试。

2. **CUDA‑GPU 构建**
   ```bash
   GG_BUILD_CUDA=1 bash ./ci/run.sh ./tmp/results ./tmp/mnt
   ```
   会在 CMake 中添加 `-DGGML_CUDA=ON -DCMAKE_CUDA_ARCHITECTURES=native`，启用 NVIDIA GPU 加速。

3. **Intel oneAPI SYCL 构建**
   ```bash
   source /opt/intel/oneapi/setvars.sh
   GG_BUILD_SYCL=1 bash ./ci/run.sh ./tmp/results ./tmp/mnt
   ```
   需先定义 `ONEAPI_ROOT`，并自动配置 `-DGGML_SYCL=1`、`-DCMAKE_C_COMPILER=icx` 等参数，面向 Intel GPU。

4. **Vulkan 构建**
   ```bash
   GG_BUILD_VULKAN=1 bash ./ci/run.sh ./tmp/results ./tmp/mnt
   ```
   在 CMake 中开启 `-DGGML_VULKAN=1`，调用 Vulkan 后端做推理测试。

5. **附加后端：Metal 与 MUSA**
   - `GG_BUILD_METAL=1` → 启用 macOS Metal（BF16）支持。
   - `GG_BUILD_MUSA=1` → 启用华为 MUSA 加速（云 CI 上可用）。

---

## 脚本执行流程

1. **目录准备**：创建并清理指定的 `output` 与 `mnt` 目录，保存日志和模型缓存。
2. **CMake 配置**：根据后端环境变量拼接 `CMAKE_EXTRA` 标志，然后依次触发
   - **Debug** 版编译与 `ctest -L main -E test-opt`
   - **Release** 版编译与 `ctest -L main`。
3. **脚本测试**：在 Debug/Release 下运行 `examples/gguf-split/tests.sh` 和 `examples/quantize/tests.sh`，验证示例脚本无误。
4. **模型推理验证**：下载指定模型（如 Pythia 1.4B、OpenLLaMA 7B-v2），依次做量化、推理 (`llama-cli`)、困惑度 (`llama-perplexity`) 和矩阵分析 (`llama-imatrix`) 等测试，确保输出正常。
5. **结果汇总**：使用内置 `gg_printf` 将所有子任务的状态、日志与摘要写入 `README.md`，方便一键查看。

---

## 补充说明

- **本地 vs 云端 CI**：虽然可在本机执行，上线时 `ggml-org/ci` 框架会在专用实例上自动触发此脚本，支持更多硬件（如 GPU 群集、Apple Silicon）和并发扩展。
- **灵活性**：通过环境变量即可快速切换后端，不必维护多份脚本；新增后端（如 MUSA、Metal）仅需在脚本开头加对应分支。
- **可扩展性**：脚本内定义了 `gg_run_<ci>` 和 `gg_sum_<ci>` 钩子，可按需新增自定义 CI 阶段，比如覆盖率统计、静态分析等。

> 综上，你的理解**完全正确**：`ci/run.sh` 通过环境变量支持 **CPU-only、CUDA、SYCL、Vulkan**（以及 Metal、MUSA）多种后端的构建与测试，并在最后输出结构化报告。
