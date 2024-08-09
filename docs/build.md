# Build llama.cpp locally（本地构建llama.cpp项目）

**To get the Code（获得源代码）:**

```bash
# 通过git命令从github上获取仓库中main branch
git clone https://github.com/ggerganov/llama.cpp
# 通过cd命令进入到llama.cpp文件目录中
cd llama.cpp
```

In order to build llama.cpp you have four different options.（为了build llama.cpp项目，你有四种不同的选择）

- Using `make`（1、使用make构建llama.cpp项目）:
  - On Linux or MacOS:

      ```bash
      # 直接使用make命令对llama.cpp项目进行build之前已经进入到llama.cpp的目录中了，在llama.cpp目录中存在一个makefile文件，make命令将会读取这个makefile文件对整个llama.cpp项目进行构建
      make
      ```

  - On Windows (x86/x64 only, arm64 requires cmake):

    1. Download the latest fortran version of （下载最新的fortran版本w64devkit）[w64devkit](https://github.com/skeeto/w64devkit/releases).
    2. Extract `w64devkit` on your pc（在本地计算机上解压）.
    3. Run `w64devkit.exe`（运行w64devkit.exe程序）.
    4. Use the `cd` command to reach the `llama.cpp` folder.（使用cd命令进入到llama.cpp文件夹中）
    5. From here you can run（现在运行make对llama.cpp项目进行build）:
        ```bash
        make
        ```

  - Notes（注意）:
    - For `Q4_0_4_4` quantization type build, add the `GGML_NO_LLAMAFILE=1` flag. For example, use `make GGML_NO_LLAMAFILE=1`.（对于Q4_0_4_4量化类型的构建，添加GGML_NO_LLAMAFILE=1标签。例如：make GGML_NO_LLAMAFILE=1）
    - For faster compilation, add the `-j` argument to run multiple jobs in parallel. For example, `make -j 8` will run 8 jobs in parallel.(为了更快的编译，通过添加-j参数并行化build。例如：make -j 8)
    - For faster repeated compilation, install [ccache](https://ccache.dev/).（为了更快的重复编译，安装ccache工具，能够加快编译速度）
    - For debug builds, run `make LLAMA_DEBUG=1`（为了能够build debug，使用make LLAMA_DEBUG=1对llama.cpp项目进行构建）

- Using `CMake`（2、使用CMake构建llama.cpp项目）:

  ```bash
  #  `-B build`：这个参数指定了构建文件夹的位置。`-B` 选项后跟的 `build` 是一个目录名，意味着CMake将创建（如果不存在的话）并使用名为 `build` 的目录来存放所有生成的构建文件。
  cmake -B build
  # 这是使用CMake来启动构建过程的命令，`--build` 选项后面跟的是构建目录，这里指的是前面创建的 `build` 目录。这个命令告诉CMake在指定的目录中启动构建过程，而无需直接使用make、Visual Studio或其他构建工具，CMake会调用相应的构建系统。
  cmake --build build --config Release
  ```

  **Notes（注意）**:

    - For `Q4_0_4_4` quantization type build, add the `-DGGML_LLAMAFILE=OFF` cmake option. For example, use `cmake -B build -DGGML_LLAMAFILE=OFF`.（对于Q4_0_4_4量化类型build,需要给cmake添加-DGGML_LLAMAFILE=OFF选项。例如：cmake -B build -DGGML_LLAMAFILE=OFF）
    - For faster compilation, add the `-j` argument to run multiple jobs in parallel. For example, `cmake --build build --config Release -j 8` will run 8 jobs in parallel.（并行编译提高编译速度，例如：cmake --build build --config Release -j 8）
    - For faster repeated compilation, install [ccache](https://ccache.dev/).(为了能够更快的重复编译，安装ccache从而提高重复编译的速度)
    - For debug builds, there are two cases(为了构建debug版本，这里有两种情况):

      1. Single-config generators (e.g. default = `Unix Makefiles`; note that they just ignore the `--config` flag)（单配置生成器（例如默认值 = `Unix Makefiles`；请注意，它们只是忽略了 `--config` 标志）:

      ```bash
      # 使用cmake构建debug版本的llama.cpp项目
      cmake -B build -DCMAKE_BUILD_TYPE=Debug
      cmake --build build
      ```

      2. Multi-config generators (`-G` param set to Visual Studio, XCode...)（多配置生成器，-G参数将会对VS，XCode有效）:

      ```bash
      # 在Xcode环境中进行build
      cmake -B build -G "Xcode"
      # 构建debug版本
      cmake --build build --config Debug
      ```
    - Building for Windows (x86, x64 and arm64) with MSVC or clang as compilers（使用MSVC或者clang作为编译器在windows上构建）:
      - Install Visual Studio 2022, e.g. via the [Community Edition](https://visualstudio.microsoft.com/de/vs/community/). In the installer, select at least the following options (this also automatically installs the required additional tools like CMake,...)（1、安装VS2022；2、在VS2022的安装器中选择至少下列的可选项（这将会自动安装额外需要的工具例如CMake等等））:
        - Tab Workload: Desktop-development with C++
        - Tab Components (select quickly via search): C++-_CMake_ Tools for Windows, _Git_ for Windows, C++-_Clang_ Compiler for Windows, MS-Build Support for LLVM-Toolset (clang)
      - Please remember to always use a Developer Command Prompt / PowerShell for VS2022 for git, build, test（请记得总是使用一个developer command prompt/powershell进行git\build\test使用）
      - For Windows on ARM (arm64, WoA) build with（硬件是ARM架构的使用下列指令进行build）:
        ```bash
        cmake --preset arm64-windows-llvm-release -D GGML_OPENMP=OFF
        cmake --build build-arm64-windows-llvm-release
        ```
        Note: Building for arm64 could also be done just with MSVC (with the build-arm64-windows-MSVC preset, or the standard CMake build instructions). But MSVC does not support inline ARM assembly-code, used e.g. for the accelerated Q4_0_4_8 CPU kernels.（注意：也可以仅使用 MSVC（使用 build-arm64-windows-MSVC 预设或标准 CMake 构建说明）为 arm64 进行构建。但 MSVC 不支持内联 ARM 汇编代码，例如用于加速的 Q4_0_4_8 CPU 内核。）

-   Using `gmake` (FreeBSD)（3、使用gmake构建llama.cpp项目）:

    1. Install and activate [DRM in FreeBSD](https://wiki.freebsd.org/Graphics)（安装和激活工具）
    2. Add your user to **video** group
    3. Install compilation dependencies.

        ```bash
        sudo pkg install gmake automake autoconf pkgconf llvm15 openblas

        gmake CC=/usr/local/bin/clang15 CXX=/usr/local/bin/clang++15 -j4
        ```






## Metal Build

On MacOS, Metal is enabled by default. Using Metal makes the computation run on the GPU.（在MacOS平台上，Metal默认是可用的。使用Metal将会使得计算是在GPU上进行的）
To disable the Metal build at compile time use the `GGML_NO_METAL=1` flag or the `GGML_METAL=OFF` cmake option.（为了在编译的时候disable Metal的build需要使用GGML_NO_METAL=1或者GGML_METAL=OFF的cmake选项）

When built with Metal support, you can explicitly disable GPU inference with the `--n-gpu-layers|-ngl 0` command-line
argument.（当使用 Metal 支持构建时，您可以使用 `--n-gpu-layers|-ngl 0` 命令行参数明确禁用 GPU 推理。）

## BLAS Build

Building the program with BLAS support may lead to some performance improvements in prompt processing using batch sizes higher than 32 (the default is 512). Support with CPU-only BLAS implementations doesn't affect the normal generation performance. We may see generation performance improvements with GPU-involved BLAS implementations, e.g. cuBLAS, hipBLAS. There are currently several different BLAS implementations available for build and use:（使用 BLAS 支持构建程序可能会导致使用高于 32（默认值为 512）的批处理大小的提示处理性能有所提高。仅支持 CPU 的 BLAS 实现不会影响正常的生成性能。我们可能会看到涉及 GPU 的 BLAS 实现（例如 cuBLAS、hipBLAS）的生成性能有所提高。目前有几种不同的 BLAS 实现可供构建和使用：）

这段话讨论了在程序中加入BLAS（Basic Linear Algebra Subprograms，基础线性代数子程序）支持可能带来的性能提升，尤其是在处理大批量数据时。以下是对这段描述的详细解释以及一些具体例子：

### 解释

1. **性能提升的条件**：
   - 提到使用BLAS支持时，如果处理的批次大小超过32（默认值为512），可能会看到性能改善。这是因为BLAS库优化了多种线性代数运算（如矩阵乘法），这些运算在处理大量数据时尤为重要。

2. **CPU与GPU的BLAS实现**：
   - 文中区分了CPU和GPU两种BLAS实现的影响：
     - **CPU-only BLAS implementations**：例如OpenBLAS或Intel MKL，这些实现主要优化了在没有GPU支持的系统上的性能，对于常规生成任务可能没有显著影响。
     - **GPU-involved BLAS implementations**：例如cuBLAS（NVIDIA的CUDA BLAS库）或hipBLAS（用于AMD GPU的BLAS库），这些实现能够利用GPU加速计算，特别是在需要处理大规模矩阵运算时，可以显著提升性能。

### 具体例子

- **使用OpenBLAS加速CPU计算**：
  - 假设一个程序需要执行大量的矩阵乘法操作，如在神经网络的前向传播中。在不使用BLAS库的情况下，程序可能直接使用基本的嵌套循环来计算矩阵乘法，这在大规模数据处理时效率较低。通过集成OpenBLAS，这些矩阵乘法可以得到显著加速，尤其是在多核CPU环境下。

- **使用cuBLAS加速GPU计算**：
  - 考虑一个深度学习应用，该应用需要处理大批量的图像数据。使用TensorFlow或PyTorch这类框架时，如果后端配置为使用cuBLAS，那么在NVIDIA GPU上进行的矩阵运算将得到加速，从而减少了训练和推理时间。例如，一个使用卷积神经网络（CNN）的图像分类任务，在处理包含数百张图像的批次时，通过cuBLAS优化的矩阵运算将大幅提高处理速度。

### 总结

BLAS库为各种大小和类型的线性代数运算提供了高效的实现，可以显著提升数据处理任务的性能，尤其是在使用大批量数据时。根据具体的硬件配置（CPU或GPU），选择合适的BLAS实现（如OpenBLAS、Intel MKL、cuBLAS或hipBLAS）是提升程序性能的关键。在设计和优化性能敏感的应用程序时，合理利用这些库是非常重要的。

### Accelerate Framework（加速框架）:

This is only available on Mac PCs and it's enabled by default. You can just build using the normal instructions.（这是在Mac PCs上唯一可用的，并且这个加速框架默认是可用的。你可以使用常规指令进行构建）

### OpenBLAS:

This provides BLAS acceleration using only the CPU. Make sure to have OpenBLAS installed on your machine.（这仅使用 CPU 即可提供 BLAS 加速。请确保您的机器上安装了 OpenBLAS。）

- Using `make`（使用make）:
  - On Linux:
    ```bash
    # 通过flag添加openblas的编译
    make GGML_OPENBLAS=1
    ```

  - On Windows:

    1. Download the latest fortran version of [w64devkit](https://github.com/skeeto/w64devkit/releases)（下载最新的w64devkit fortran版本）.
    2. Download the latest version of [OpenBLAS for Windows](https://github.com/xianyi/OpenBLAS/releases)（下载最新的OpenBLAS for Windows版本）.
    3. Extract `w64devkit` on your pc（在pc上解压）.
    4. From the OpenBLAS zip that you just downloaded copy `libopenblas.a`, located inside the `lib` folder, inside `w64devkit\x86_64-w64-mingw32\lib`.（从刚刚下载的 OpenBLAS zip 中复制位于“lib”文件夹内的“libopenblas.a”，拷贝到“w64devkit\x86_64-w64-mingw32\lib”内。）
    5. From the same OpenBLAS zip copy the content of the `include` folder inside `w64devkit\x86_64-w64-mingw32\include`.（使用相同的方式将include拷贝到对应位置）
    6. Run `w64devkit.exe`.
    7. Use the `cd` command to reach the `llama.cpp` folder.
    8. From here you can run:

        ```bash
        make GGML_OPENBLAS=1
        ```

- Using `CMake` on Linux:

    ```bash
    cmake -B build -DGGML_BLAS=ON -DGGML_BLAS_VENDOR=OpenBLAS
    cmake --build build --config Release
    ```

### BLIS

Check [BLIS.md](./backend/BLIS.md) for more information.（查看BLIS.md获得更多的信息）

### SYCL

SYCL is a higher-level programming model to improve programming productivity on various hardware accelerators.（SYCL 是一种更高级别的编程模型，用于提高各种硬件加速器的编程效率。）

llama.cpp based on SYCL is used to **support Intel GPU** (Data Center Max series, Flex series, Arc series, Built-in GPU and iGPU).(基于 SYCL 的 llama.cpp 用于**支持英特尔 GPU**（Data Center Max 系列、Flex 系列、Arc 系列、内置 GPU 和 iGPU）)

For detailed info, please refer to [llama.cpp for SYCL](./backend/SYCL.md).(对于详细信息，请查看SYCL.md文件内容)

 - 这段话提到的内容涉及到使用基于SYCL的`llama.cpp`程序来支持Intel的不同系列GPU。
 -  SYCL是一个基于C++的单一源异步并行编程模型，主要用于开发跨平台的高性能计算（HPC）应用。SYCL允许编写可以在多种硬件设备上执行的代码，包括CPU、GPU和其他加速器。
 -  这里特别提到`llama.cpp`程序能够支持Intel的多个GPU系列，这意味着程序已经优化或配置为能够利用这些具体型号的GPU硬件特性，进行高效的计算。
 -  **Intel GPU系列**： 
     - **Data Center Max series**：这是针对数据中心设计的高性能GPU系列，通常用于大规模并行计算任务，如深度学习训练和高性能计算。(Intel Xe-HPC (Ponte Vecchio))
     - **Flex series**：可能指的是为更灵活的计算需求设计的GPU，适用于各种数据中心环境。("Flex"系列并不是Intel正式发布的GPU系列名称。Intel的数据中心GPU主要以Xeon处理器内集成的GPU或者独立的Xe系列产品为主，例如Xe-LP、Xe-HP等。)
     - **Arc series**：这是Intel针对高性能图形和计算任务设计的GPU系列，适用于游戏、内容创建和AI计算。(Intel Arc A380, A750等，这些是Intel推出的针对游戏和内容创建市场的独立显卡产品。它们支持高级图形特性，如光线追踪和AI驱动的超级采样技术。)
     - **Built-in GPU and iGPU (Integrated GPU)**：内置GPU和集成GPU，通常集成在Intel的处理器中，适合处理一些不需要独立显卡的图形和计算任务。(**Intel Iris Xe Graphics**: 这是Intel较新的集成显卡，提供较高的图形性能，适用于日常图形处理和一些轻度游戏。 **Intel UHD Graphics**: 在更多入门级和主流Intel处理器中找到，适合基本的日常使用和一些较为简单的图形任务。)


### Intel oneMKL

Building through oneAPI compilers will make avx_vnni instruction set available for intel processors that do not support avx512 and avx512_vnni. Please note that this build config **does not support Intel GPU**. For Intel GPU support, please refer to [llama.cpp for SYCL](./backend/SYCL.md).

通过oneAPI编译器构建，对不支持avx512和avx512_vnni的intel处理器而言avx_vnni指令集变得可用。清楚一这个构建配置不会支持intel GPU.对于intel GPU的支持，请查看SYCL.md文档内容

- Using manual oneAPI installation（手动安装oneAPI）:
  By default, `GGML_BLAS_VENDOR` is set to `Generic`, so if you already sourced intel environment script and assign `-DGGML_BLAS=ON` in cmake, the mkl version of Blas will automatically been selected. Otherwise please install oneAPI and follow the below steps（默认情况下，`GGML_BLAS_VENDOR` 设置为 `Generic`，因此如果您已经获取了英特尔环境脚本并在 cmake 中分配了 `-DGGML_BLAS=ON`，则会自动选择 Blas 的 mkl 版本。否则，请安装 oneAPI 并按照以下步骤操作：）:
    ```bash
    source /opt/intel/oneapi/setvars.sh # You can skip this step if  in oneapi-basekit docker image, only required for manual installation
    cmake -B build -DGGML_BLAS=ON -DGGML_BLAS_VENDOR=Intel10_64lp -DCMAKE_C_COMPILER=icx -DCMAKE_CXX_COMPILER=icpx -DGGML_NATIVE=ON
    cmake --build build --config Release
    ```

- Using oneAPI docker image:
  If you do not want to source the environment vars and install oneAPI manually, you can also build the code using intel docker container: [oneAPI-basekit](https://hub.docker.com/r/intel/oneapi-basekit). Then, you can use the commands given above.（如果你不想手动获取环境变量并安装 oneAPI，也可以使用 intel docker 容器构建代码：[oneAPI-basekit](https://hub.docker.com/r/intel/oneapi-basekit)。然后，你可以使用上面给出的命令）

Check [Optimizing and Running LLaMA2 on Intel® CPU](https://www.intel.com/content/www/us/en/content-details/791610/optimizing-and-running-llama2-on-intel-cpu.html) for more information.（查看相关内容获得更多信息）

### CUDA

This provides GPU acceleration using the CUDA cores of your Nvidia GPU. Make sure to have the CUDA toolkit installed. You can download it from your Linux distro's package manager (e.g. `apt install nvidia-cuda-toolkit`) or from here: [CUDA Toolkit](https://developer.nvidia.com/cuda-downloads).（这使用 Nvidia GPU 的 CUDA 核心提供 GPU 加速。确保已安装 CUDA 工具包。您可以从 Linux 发行版的包管理器（例如 `apt install nvidia-cuda-toolkit`）或从此处下载：[CUDA 工具包](https://developer.nvidia.com/cuda-downloads)。）

For Jetson user, if you have Jetson Orin, you can try this: [Offical Support](https://www.jetson-ai-lab.com/tutorial_text-generation.html). If you are using an old model(nano/TX2), need some additional operations before compiling.（对于 Jetson 用户，如果您有 Jetson Orin，您可以尝试这个：[官方支持](https://www.jetson-ai-lab.com/tutorial_text-generation.html)。如果您使用的是旧型号（nano/TX2），则在编译之前需要一些额外的操作。）


- Using `make`:
  ```bash
  # 添加GGML_CUDA=1标签进行构建
  make GGML_CUDA=1
  ```
- Using `CMake`:

  ```bash
  # 添加-DGGML_CUDA=ON标签
  cmake -B build -DGGML_CUDA=ON
  cmake --build build --config Release
  ```

The environment variable [`CUDA_VISIBLE_DEVICES`](https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#env-vars) can be used to specify which GPU(s) will be used.（环境变量CUDA_VISIBLE_DEVICES可以被使用从而指定多少个GPU被使用）

The environment variable `GGML_CUDA_ENABLE_UNIFIED_MEMORY=1` can be used to enable unified memory in Linux. This allows swapping to system RAM instead of crashing when the GPU VRAM is exhausted. In Windows this setting is available in the NVIDIA control panel as `System Memory Fallback`.（环境变量“GGML_CUDA_ENABLE_UNIFIED_MEMORY=1”可用于在 Linux 中启用统一内存。这允许交换到系统 RAM，而不是在 GPU VRAM 耗尽时崩溃。在 Windows 中，此设置在 NVIDIA 控制面板中作为“系统内存回退”提供。）

The following compilation options are also available to tweak performance（以下编译选项也可用于调整性​​能）:

| Option                        | Legal values           | Default | Description                                                                                                                                                                                                                                                                             |
|-------------------------------|------------------------|---------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| GGML_CUDA_FORCE_DMMV          | Boolean                | false   | Force the use of dequantization + matrix vector multiplication kernels instead of using kernels that do matrix vector multiplication on quantized data. By default the decision is made based on compute capability (MMVQ for 6.1/Pascal/GTX 1000 or higher). Does not affect k-quants.（强制使用反量化 + 矩阵向量乘法核，而不是使用对量化数据进行矩阵向量乘法的核。默认情况下，决策基于计算能力（MMVQ 适用于 6.1/Pascal/GTX 1000 或更高版本）。不影响 k-quant。） |
| GGML_CUDA_DMMV_X              | Positive integer >= 32 | 32      | Number of values in x direction processed by the CUDA dequantization + matrix vector multiplication kernel per iteration. Increasing this value can improve performance on fast GPUs. Power of 2 heavily recommended. Does not affect k-quants.（每次迭代中 CUDA 反量化 + 矩阵向量乘法核处理的 x 方向值的数量。增加此值可以提高快速 GPU 的性能。强烈建议使用 2 的幂。不影响 k-quants。）                                         |
| GGML_CUDA_MMV_Y               | Positive integer       | 1       | Block size in y direction for the CUDA mul mat vec kernels. Increasing this value can improve performance on fast GPUs. Power of 2 recommended.（CUDA mul mat vec 内核在 y 方向上的块大小。增加此值可以提高快速 GPU 的性能。建议使用 2 的幂。）                                                                                                                                         |
| GGML_CUDA_FORCE_MMQ           | Boolean                | false   | Force the use of custom matrix multiplication kernels for quantized models instead of FP16 cuBLAS even if there is no int8 tensor core implementation available (affects V100, RDNA3). MMQ kernels are enabled by default on GPUs with int8 tensor core support. With MMQ force enabled, speed for large batch sizes will be worse but VRAM consumption will be lower.（即使没有可用的 int8 张量核心实现，也强制对量化模型使用自定义矩阵乘法内核，而不是 FP16 cuBLAS（影响 V100、RDNA3）。在支持 int8 张量核心的 GPU 上，MMQ 内核默认启用。启用 MMQ 强制后，大批量处理的速度会变差，但 VRAM 消耗会降低。）                       |
| GGML_CUDA_FORCE_CUBLAS        | Boolean                | false   | Force the use of FP16 cuBLAS instead of custom matrix multiplication kernels for quantized models（强制使用 FP16 cuBLAS 代替量化模型的自定义矩阵乘法核）                                                                                                                                                                                       |
| GGML_CUDA_F16                 | Boolean                | false   | If enabled, use half-precision floating point arithmetic for the CUDA dequantization + mul mat vec kernels and for the q4_1 and q5_1 matrix matrix multiplication kernels. Can improve performance on relatively recent GPUs.（如果启用，则对 CUDA 反量化 + mul mat vec 内核以及 q4_1 和 q5_1 矩阵乘法内核使用半精度浮点运算。可以提高相对较新的 GPU 上的性能。）                                                           |
| GGML_CUDA_KQUANTS_ITER        | 1 or 2                 | 2       | Number of values processed per iteration and per CUDA thread for Q2_K and Q6_K quantization formats. Setting this value to 1 can improve performance for slow GPUs.（对于 Q2_K 和 Q6_K 量化格式，每次迭代和每个 CUDA 线程处理的值的数量。将此值设置为 1 可以提高慢速 GPU 的性能。）                                                                                                                     |
| GGML_CUDA_PEER_MAX_BATCH_SIZE | Positive integer       | 128     | Maximum batch size for which to enable peer access between multiple GPUs. Peer access requires either Linux or NVLink. When using NVLink enabling peer access for larger batch sizes is potentially beneficial.（启用多个 GPU 之间的对等访问的最大批处理大小。对等访问需要 Linux 或 NVLink。使用 NVLink 时，启用较大批处理大小的对等访问可能会带来好处。）                                                                         |
| GGML_CUDA_FA_ALL_QUANTS       | Boolean                | false   | Compile support for all KV cache quantization type (combinations) for the FlashAttention CUDA kernels. More fine-grained control over KV cache size but compilation takes much longer.（为 FlashAttention CUDA 内核编译所有 KV 缓存量化类型（组合）的支持。对 KV 缓存大小的控制更加细粒度，但编译时间更长。）                                                                                                  |

### MUSA

- Using `make`:
  ```bash
  make GGML_MUSA=1
  ```
- Using `CMake`:

  ```bash
  cmake -B build -DGGML_MUSA=ON
  cmake --build build --config Release
  ```

### hipBLAS

This provides BLAS acceleration on HIP-supported AMD GPUs.
Make sure to have ROCm installed.
You can download it from your Linux distro's package manager or from here: [ROCm Quick Start (Linux)](https://rocm.docs.amd.com/projects/install-on-linux/en/latest/tutorial/quick-start.html#rocm-install-quick).（这为 HIP 支持的 AMD GPU 提供了 BLAS 加速。确保已安装 ROCm。您可以从 Linux 发行版的包管理器或从此处下载它）

- Using `make`:
  ```bash
  make GGML_HIPBLAS=1
  ```
- Using `CMake` for Linux (assuming a gfx1030-compatible AMD GPU):
  ```bash
  HIPCXX="$(hipconfig -l)/clang" HIP_PATH="$(hipconfig -R)" \
      cmake -S . -B build -DGGML_HIPBLAS=ON -DAMDGPU_TARGETS=gfx1030 -DCMAKE_BUILD_TYPE=Release \
      && cmake --build build --config Release -- -j 16
  ```
  On Linux it is also possible to use unified memory architecture (UMA) to share main memory between the CPU and integrated GPU by setting `-DGGML_HIP_UMA=ON`.（在 Linux 上，还可以通过设置 `-DGGML_HIP_UMA=ON`，使用​​统一内存架构 (UMA) 在 CPU 和集成 GPU 之间共享主内存。）
  However, this hurts performance for non-integrated GPUs (but enables working with integrated GPUs).（但是，这会损害非集成 GPU 的性能（但可以与集成 GPU 配合使用）。）

  Note that if you get the following error（注意如果你遇到了下列的错误）:
  ```
  clang: error: cannot find ROCm device library; provide its path via '--rocm-path' or '--rocm-device-lib-path', or pass '-nogpulib' to build without ROCm device library
  ```
  Try searching for a directory under `HIP_PATH` that contains the file
  `oclc_abi_version_400.bc`. Then, add the following to the start of the
  command: `HIP_DEVICE_LIB_PATH=<directory-you-just-found>`, so something
  like（尝试在“HIP_PATH”下搜索包含文件“oclc_abi_version_400.bc”的目录。然后，将以下内容添加到命令的开头：“HIP_DEVICE_LIB_PATH=<directory-you-just-found>”）:
  ```bash
  HIPCXX="$(hipconfig -l)/clang" HIP_PATH="$(hipconfig -p)" \
  HIP_DEVICE_LIB_PATH=<directory-you-just-found> \
      cmake -S . -B build -DGGML_HIPBLAS=ON -DAMDGPU_TARGETS=gfx1030 -DCMAKE_BUILD_TYPE=Release \
      && cmake --build build -- -j 16
  ```

- Using `make` (example for target gfx1030, build with 16 CPU threads):
  ```bash
  make -j16 GGML_HIPBLAS=1 GGML_HIP_UMA=1 AMDGPU_TARGETS=gfx1030
  ```

- Using `CMake` for Windows (using x64 Native Tools Command Prompt for VS, and assuming a gfx1100-compatible AMD GPU):
  ```bash
  set PATH=%HIP_PATH%\bin;%PATH%
  cmake -S . -B build -G Ninja -DAMDGPU_TARGETS=gfx1100 -DGGML_HIPBLAS=ON -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
  cmake --build build
  ```
  Make sure that `AMDGPU_TARGETS` is set to the GPU arch you want to compile for. The above example uses `gfx1100` that corresponds to Radeon RX 7900XTX/XT/GRE. You can find a list of targets [here](https://llvm.org/docs/AMDGPUUsage.html#processors)
  Find your gpu version string by matching the most significant version information from `rocminfo | grep gfx | head -1 | awk '{print $2}'` with the list of processors, e.g. `gfx1035` maps to `gfx1030`.


The environment variable [`HIP_VISIBLE_DEVICES`](https://rocm.docs.amd.com/en/latest/understand/gpu_isolation.html#hip-visible-devices) can be used to specify which GPU(s) will be used.（环境变量HIP_VISIBLE_DEVICES可以被用来指定哪些GPU可以被使用）
If your GPU is not officially supported you can use the environment variable [`HSA_OVERRIDE_GFX_VERSION`] set to a similar GPU, for example 10.3.0 on RDNA2 (e.g. gfx1030, gfx1031, or gfx1035) or 11.0.0 on RDNA3.(如果您的 GPU 不受官方支持，您可以使用环境变量 [`HSA_OVERRIDE_GFX_VERSION`] 设置为类似的 GPU，例如 RDNA2 上的 10.3.0（例如 gfx1030、gfx1031 或 gfx1035）或 RDNA3 上的 11.0.0。)
The following compilation options are also available to tweak performance (yes, they refer to CUDA, not HIP, because it uses the same code as the cuBLAS version above)(以下编译选项也可用于调整性​​能（是的，它们指的是 CUDA，而不是 HIP，因为它使用与上面的 cuBLAS 版本相同的代码）：):

| Option                 | Legal values           | Default | Description                                                                                                                                                                                                                                    |
|------------------------|------------------------|---------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| GGML_CUDA_DMMV_X       | Positive integer >= 32 | 32      | Number of values in x direction processed by the HIP dequantization + matrix vector multiplication kernel per iteration. Increasing this value can improve performance on fast GPUs. Power of 2 heavily recommended. Does not affect k-quants. |
| GGML_CUDA_MMV_Y        | Positive integer       | 1       | Block size in y direction for the HIP mul mat vec kernels. Increasing this value can improve performance on fast GPUs. Power of 2 recommended. Does not affect k-quants.                                                                       |
| GGML_CUDA_KQUANTS_ITER | 1 or 2                 | 2       | Number of values processed per iteration and per HIP thread for Q2_K and Q6_K quantization formats. Setting this value to 1 can improve performance for slow GPUs.                                                                             |

### Vulkan

**Windows**

#### w64devkit

Download and extract [w64devkit](https://github.com/skeeto/w64devkit/releases).

Download and install the [Vulkan SDK](https://vulkan.lunarg.com/sdk/home#windows). When selecting components, only the Vulkan SDK Core is required.

Launch `w64devkit.exe` and run the following commands to copy Vulkan dependencies:
```sh
SDK_VERSION=1.3.283.0
cp /VulkanSDK/$SDK_VERSION/Bin/glslc.exe $W64DEVKIT_HOME/bin/
cp /VulkanSDK/$SDK_VERSION/Lib/vulkan-1.lib $W64DEVKIT_HOME/x86_64-w64-mingw32/lib/
cp -r /VulkanSDK/$SDK_VERSION/Include/* $W64DEVKIT_HOME/x86_64-w64-mingw32/include/
cat > $W64DEVKIT_HOME/x86_64-w64-mingw32/lib/pkgconfig/vulkan.pc <<EOF
Name: Vulkan-Loader
Description: Vulkan Loader
Version: $SDK_VERSION
Libs: -lvulkan-1
EOF

```
Switch into the `llama.cpp` directory and run `make GGML_VULKAN=1`.

#### MSYS2
Install [MSYS2](https://www.msys2.org/) and then run the following commands in a UCRT terminal to install dependencies.
  ```sh
  pacman -S git \
      mingw-w64-ucrt-x86_64-gcc \
      mingw-w64-ucrt-x86_64-cmake \
      mingw-w64-ucrt-x86_64-vulkan-devel \
      mingw-w64-ucrt-x86_64-shaderc
  ```
Switch into `llama.cpp` directory and build using CMake.
```sh
cmake -B build -DGGML_VULKAN=ON
cmake --build build --config Release
```

**With docker**:

You don't need to install Vulkan SDK. It will be installed inside the container.(你不需要安装Vulkan SDK,这将会在容器中安装)

```sh
# Build the image（构建镜像）
docker build -t llama-cpp-vulkan -f .devops/llama-cli-vulkan.Dockerfile .

# Then, use it:
docker run -it --rm -v "$(pwd):/app:Z" --device /dev/dri/renderD128:/dev/dri/renderD128 --device /dev/dri/card1:/dev/dri/card1 llama-cpp-vulkan -m "/app/models/YOUR_MODEL_FILE" -p "Building a website can be done in 10 simple steps:" -n 400 -e -ngl 33
```

**Without docker**:

Firstly, you need to make sure you have installed [Vulkan SDK](https://vulkan.lunarg.com/doc/view/latest/linux/getting_started_ubuntu.html)

For example, on Ubuntu 22.04 (jammy), use the command below:

```bash
wget -qO - https://packages.lunarg.com/lunarg-signing-key-pub.asc | apt-key add -
wget -qO /etc/apt/sources.list.d/lunarg-vulkan-jammy.list https://packages.lunarg.com/vulkan/lunarg-vulkan-jammy.list
apt update -y
apt-get install -y vulkan-sdk
# To verify the installation, use the command below:
vulkaninfo
```

Alternatively your package manager might be able to provide the appropriate libraries.
For example for Ubuntu 22.04 you can install `libvulkan-dev` instead.
For Fedora 40, you can install `vulkan-devel`, `glslc` and `glslang` packages.

Then, build llama.cpp using the cmake command below:

```bash
cmake -B build -DGGML_VULKAN=1
cmake --build build --config Release
# Test the output binary (with "-ngl 33" to offload all layers to GPU)
./bin/llama-cli -m "PATH_TO_MODEL" -p "Hi you how are you" -n 50 -e -ngl 33 -t 4

# You should see in the output, ggml_vulkan detected your GPU. For example:
# ggml_vulkan: Using Intel(R) Graphics (ADL GT2) | uma: 1 | fp16: 1 | warp size: 32
```

### Android

To read documentation for how to build on Android, [click here](./android.md)（查看android.md文件来学习如何在android上构建）
