# Build llama.cpp locally
```c
/*
Note:杨小兵-2025-01-11

1、这个文档用来作为一个参考：如何在本地构建llama.cpp项目
2、这里的关键就是如何在本地实现对llama.cpp项目，从而使得开发者可以对llama.cpp项目源代码做出一些修改并且将修改后的内容构建出来然后发布。
*/
```

**To get the Code:**

```bash
git clone https://github.com/ggerganov/llama.cpp
cd llama.cpp
```
```c
/*
Note:杨小兵-2025-01-11

1、可以通过上述git命令获取得到llama.cpp项目的source code
2、命令解释
    2.1 使用git获取得到llama.cpp项目的源代码
    2.2 使用git得到llama.cpp项目源代码之后通过cd进入到llama.cpp目录中（一种特殊的文件）
*/
```

The following sections describe how to build with different backends and options.
```c
/*
Note:杨小兵-2025-01-11

1、下列这些sections描述了如何使用不同的backends和options来进行构建。
2、backends
    2.1 CPU
    2.2 BLAS
    2.3 Metal
    2.4 SYCL
    2.5 CUDA
    2.6 MUSA
    2.7 HIP
    2.8 Vulkan
    2.9 CANN
    2.10 Android
*/
```

## 1 CPU Build

Build llama.cpp using `CMake`:

```bash
cmake -B build
cmake --build build --config Release
```
```c
/*
Note:杨小兵-2025-01-11

1、使用CMake构建llama.cpp项目
2、camke -B build
    2.1 -B：这是 CMake 的一个选项，用于指定构建目录（build directory）。它告诉 CMake 在哪里生成构建系统文件（如 Makefile、Visual Studio 解决方案等）。
    2.2 build：这是指定的构建目录名称。在此例中，build 是一个相对路径，表示在当前工作目录下创建一个名为 build 的子目录。
    2.3 简而言之：cmake -B build 不仅在当前路径中创建一个名为 build 的目录（如果该目录尚不存在），还在其中生成构建系统文件，为后续的编译过程做好准备。
3、cmake --build build --config Release
    3.1 --build：这是 CMake 的一个选项，用于指示 CMake 执行构建操作。
    3.2 build：这是指定的构建目录，与前一个命令中的 -B build 相对应。CMake 会在该目录中查找生成的构建系统文件（如 Makefile、Ninja 文件、Visual Studio 解决方案等）。
    3.3 --config：这个选项用于指定构建配置。在多配置生成器（如 Visual Studio、Xcode）中，您可以有多个构建配置（如 Debug、Release、RelWithDebInfo 等）。
    3.4 Release：这是指定的构建配置名称，表示要以 Release 模式进行构建。Release 配置通常会启用优化，以获得更高的运行时性能，但可能会禁用调试信息。
    3.5 简而言之：cmake --build build --config Release 命令指示 CMake 在先前指定的 build 目录中，使用 Release 配置执行项目的编译和构建过程。这确保了生成的二进制文件具有较高的性能优化，适合生产环境使用。
*/
```

**Notes**:

- For faster compilation, add the `-j` argument to run multiple jobs in parallel, or use a generator that does this automatically such as Ninja. For example, `cmake --build build --config Release -j 8` will run 8 jobs in parallel.
    ```c
    /*
    Note:杨小兵-2025-01-11

    1、为了更快的编译，通过添加-j参数从而并行的运行多个任务，或者使用一个可以自动并行运行多个任务的生成器例如Ninja，例如：cmake --build build --config Release -j 8，这个命令将会并行的运行8个任务。
    2、两种方式加快编译速度
        2.1 通过添加-j选项
        2.2 通过选择可以自动实现相同功能的generator
    */
    ```
- For faster repeated compilation, install [ccache](https://ccache.dev/)
    ```c
    /*
    Note:杨小兵-2025-01-11

    1、ccache（Compiler Cache）是一个开源的编译器缓存工具，旨在加速 C/C++ 等编程语言的重复编译过程。通过缓存先前编译过的对象文件，ccache 可以在源代码未发生变化的情况下，避免重复编译，从而显著缩短编译时间，提高开发效率。
    2、ccache 的作用是什么？
        2.1 加快编译速度：当相同的源文件在相同的编译选项下被多次编译时，ccache 会直接从缓存中提取已编译的对象文件，而无需再次调用编译器。这大大减少了编译时间，特别是在大型项目中效果显著。
        2.2 减少编译器负担：通过减少重复编译，ccache 降低了编译器的负载，有助于延长硬件的使用寿命，降低能耗。
    */
    ```
- For debug builds, there are two cases:

    1. Single-config generators (e.g. default = `Unix Makefiles`; note that they just ignore the `--config` flag):

       ```bash
       cmake -B build -DCMAKE_BUILD_TYPE=Debug
       cmake --build build
       ```

    2. Multi-config generators (`-G` param set to Visual Studio, XCode...):

       ```bash
       cmake -B build -G "Xcode"
       cmake --build build --config Debug
       ```

    For more details and a list of supported generators, see the [CMake documentation](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html).
    ```c
    /*
    Note:杨小兵-2025-01-11

    1、对于调试版本有两种情况：单配置的generator、多配置的generator
    2、Single-config generators
        2.1 默认情况下Unix Makefiles生成器，这种类型的生成器将会自动忽略--config参数
        2.2 在使用cmake进行构llama.cpp项目的时候需要通过传递参数来实现debug版本的构建
    3、Multi-config generators
        3.1 可以通过-G参数来设置生成器，例如可以是Visual Studio, XCode...
    4、对于更多的细节和支持的generators，查看对应的cmake文档。
    */
    ```

- For static builds, add `-DBUILD_SHARED_LIBS=OFF`:
  ```
  cmake -B build -DBUILD_SHARED_LIBS=OFF
  cmake --build build --config Release
  ```
  ```c
  /*
  Note:杨小兵-2025-01-11

  1、对于静态构建，可以通过向cmake添加BUILD_SHARED_LIBS参数来实现
  */
  ```
- Building for Windows (x86, x64 and arm64) with MSVC or clang as compilers:
    - Install Visual Studio 2022, e.g. via the [Community Edition](https://visualstudio.microsoft.com/de/vs/community/). In the installer, select at least the following options (this also automatically installs the required additional tools like CMake,...):
    - Tab Workload: Desktop-development with C++
    - Tab Components (select quickly via search): C++-_CMake_ Tools for Windows, _Git_ for Windows, C++-_Clang_ Compiler for Windows, MS-Build Support for LLVM-Toolset (clang)
    - Please remember to always use a Developer Command Prompt / PowerShell for VS2022 for git, build, test
    - For Windows on ARM (arm64, WoA) build with:
    ```bash
    cmake --preset arm64-windows-llvm-release -D GGML_OPENMP=OFF
    cmake --build build-arm64-windows-llvm-release
    ```
    Building for arm64 can also be done with the MSVC compiler with the build-arm64-windows-MSVC preset, or the standard CMake build instructions. However, note that the MSVC compiler does not support inline ARM assembly code, used e.g. for the accelerated Q4_0_N_M CPU kernels.

    For building with ninja generator and clang compiler as default:
      -set path:set LIB=C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\um\x64;C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.41.34120\lib\x64\uwp;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\ucrt\x64
      ```bash
      cmake --preset x64-windows-llvm-release
      cmake --build build-x64-windows-llvm-release
      ```

## 2 BLAS Build

Building the program with BLAS support may lead to some performance improvements in prompt processing using batch sizes higher than 32 (the default is 512). Using BLAS doesn't affect the generation performance. There are currently several different BLAS implementations available for build and use:
```c
/*
Note:杨小兵-2025-01-11

1、上述内容想要表达的观点：
    1.1 性能提升：使用BLAS支持可以在处理批量大小（batch size）高于32时（默认批量大小为512），提升prompt processing的性能。这意味着在处理较大批量数据时，程序的效率可能会有所改善。
    1.2 生成性能不受影响：尽管BLAS在处理大批量数据时带来性能提升，但它并不会影响生成性能。这表明BLAS的优化主要针对特定的数据处理环节，而不会对整体生成过程造成负面影响。
    1.3 多种BLAS实现可供选择：当前有多种不同的BLAS实现可用于构建和使用，这为开发者提供了灵活性，可以根据具体需求选择最合适的BLAS版本。
2. BLAS介绍
    BLAS（Basic Linear Algebra Subprograms，基础线性代数子程序） 是一组标准化的基本线性代数操作子程序，旨在为高性能数学计算提供基础功能。BLAS 被广泛应用于科学计算、工程、数据分析和机器学习等领域，作为更复杂线性代数运算的基础组件。

    2.1 主要特点和功能：
        2.1.1 分级结构：
            - 级别1（Level 1）：向量操作，如向量加法、点积等。
            - 级别2（Level 2）：矩阵-向量操作，如矩阵乘以向量。
            - 级别3（Level 3）：矩阵-矩阵操作，如矩阵乘法。
        2.1.2 高性能优化：
            - BLAS 实现通常针对特定硬件架构进行高度优化，充分利用缓存、并行处理和向量化指令，以实现最大计算效率。
            - 许多高性能计算库（如 LAPACK、Eigen、TensorFlow、PyTorch 等）都基于 BLAS 构建，借助其优化的底层操作提升整体性能。
        2.1.3 多种实现版本：
            - OpenBLAS：开源实现，支持多种硬件架构，广泛应用于各种开源项目。
            - Intel MKL（Math Kernel Library）：英特尔提供的高性能数学库，针对英特尔处理器进行了深度优化。
            - ATLAS（Automatically Tuned Linear Algebra Software）：自动调优的BLAS实现，通过自动优化过程适应不同硬件。
            - cuBLAS：NVIDIA 为 GPU 提供的 BLAS 实现，专门用于加速在 NVIDIA GPU 上的线性代数计算。
        2.1.4 跨平台支持：
            - BLAS 库支持多种操作系统和硬件平台，包括 Windows、Linux、macOS，以及各种 CPU 和 GPU 架构。
        2.1.5 接口标准化：
            - BLAS 定义了一套标准化的接口，使得不同的 BLAS 实现可以无缝替换，开发者可以根据需要选择最适合的版本，而无需修改上层应用代码。
    2.2 应用场景：
        2.2.1 科学计算与工程模拟：解决线性方程组、特征值问题、最优化等。
        2.2.2 机器学习与数据分析：加速矩阵运算、优化算法中的梯度计算等。
        2.2.3 图形处理与计算机视觉：处理大量矩阵和向量数据，实现高效的图像和视频处理算法。
*/
```

### 2.1 Accelerate Framework

This is only available on Mac PCs and it's enabled by default. You can just build using the normal instructions.
```c
/*
Note:杨小兵-2025-01-11

1、Accelerate Framework 是苹果公司为 macOS 提供的高性能数学计算库，包含了优化过的线性代数、数字信号处理、图像处理等功能。
2、Accelerate Framework是一个高性能的数学计算库，仅适用于 Mac 电脑（macOS）。
3、在 macOS 上，Accelerate Framework 已默认启用，因此无需额外配置即可使用。对于使用 macOS 的用户，Accelerate Framework 提供了开箱即用的 BLAS 支持，无需额外安装和配置，简化了构建过程。
*/
```

### 2.2 OpenBLAS

This provides BLAS acceleration using only the CPU. Make sure to have OpenBLAS installed on your machine.

- Using `CMake` on Linux:

    ```bash
    cmake -B build -DGGML_BLAS=ON -DGGML_BLAS_VENDOR=OpenBLAS
    cmake --build build --config Release
    ```
    ```c
    /*
    Note:杨小兵-2025-01-11

    1、OpenBLAS 是一个开源的高性能 BLAS 实现，支持多种 CPU 架构。
    2、确保你的机器上已安装 OpenBLAS。可以通过包管理器（如 apt、brew 等）进行安装，或从源代码编译安装。
    3、命令解释
        3.1 cmake -B build: 指定构建目录为 build。
        3.2 -DGGML_BLAS=ON: 启用 BLAS 支持。
        3.3 -DGGML_BLAS_VENDOR=OpenBLAS: 指定 BLAS 供应商为 OpenBLAS。
        3.4 cmake --build build --config Release: 在 build 目录下以 Release 配置进行构建。
    4、OpenBLAS 是一个广泛使用且性能优秀的 BLAS 实现，通过简单的 CMake 配置即可集成到项目中，适用于需要高性能 CPU 线性代数运算的用户。
    */
    ```
### 2.3 BLIS

Check [BLIS.md](./backend/BLIS.md) for more information.
```c
/*
Note:杨小兵-2025-01-11

1、查看[BLIS.md](./backend/BLIS.md)获取更多的信息
*/
```

### 2.4 Intel oneMKL

Building through oneAPI compilers will make avx_vnni instruction set available for intel processors that do not support avx512 and avx512_vnni. Please note that this build config **does not support Intel GPU**. For Intel GPU support, please refer to [llama.cpp for SYCL](./backend/SYCL.md).
```c
/*
Note:杨小兵-2025-01-11

1、Intel oneMKL（Math Kernel Library） 是英特尔提供的高性能数学库，包含优化过的 BLAS、FFT、随机数生成等功能。
2、使用 oneAPI 编译器构建：通过使用 Intel 的 oneAPI 编译器进行构建，可以在不支持 AVX-512 和 AVX-512 VNNI 指令集的 Intel 处理器上启用 AVX-VNNI 指令集。AVX-VNNI 指令集的启用旨在提升线性代数运算的性能，尤其是在处理大型矩阵和向量计算时，提高计算效率。
3、不支持 Intel GPU：当前通过 oneAPI 编译器构建的配置 不支持 Intel GPU。这意味着，如果项目需要在 Intel GPU 上运行或加速计算，不能依赖于此构建配置。对于需要 Intel GPU 支持的用户，建议参考[llama.cpp for SYCL](./backend/SYCL.md)文档。这表明存在另一种构建或实现方式（基于 SYCL），专门用于在 Intel GPU 上运行或加速程序。
4、通过使用 Intel 的 oneAPI 编译器进行构建，可以在不具备 AVX-512 和 AVX-512 VNNI 指令集的 Intel 处理器上启用 AVX-VNNI 指令集，从而优化 CPU 端的线性代数运算性能。然而，此种构建配置不支持 Intel GPU。如果需要在 Intel GPU 上进行加速，用户应参考基于 SYCL 的实现方法。
*/
```

- Using manual oneAPI installation:
  By default, `GGML_BLAS_VENDOR` is set to `Generic`, so if you already sourced intel environment script and assign `-DGGML_BLAS=ON` in cmake, the mkl version of Blas will automatically been selected. Otherwise please install oneAPI and follow the below steps:
    ```bash
    source /opt/intel/oneapi/setvars.sh # You can skip this step if  in oneapi-basekit docker image, only required for manual installation
    cmake -B build -DGGML_BLAS=ON -DGGML_BLAS_VENDOR=Intel10_64lp -DCMAKE_C_COMPILER=icx -DCMAKE_CXX_COMPILER=icpx -DGGML_NATIVE=ON
    cmake --build build --config Release
    ```
- Using oneAPI docker image:
  If you do not want to source the environment vars and install oneAPI manually, you can also build the code using intel docker container: [oneAPI-basekit](https://hub.docker.com/r/intel/oneapi-basekit). Then, you can use the commands given above.
    ```c
    /*
    Note:杨小兵-2025-01-11

    1、通过手动安装和配置 Intel oneAPI，用户可以利用 Intel 优化的 BLAS 实现（如 MKL）和编译器（icx、icpx），以提升项目中线性代数运算的性能。具体步骤包括加载 oneAPI 环境变量、使用 CMake 指定 Intel 的 BLAS 供应商和编译器，以及启用本地优化。对于不想手动安装和配置的用户，可以选择使用预配置的 oneAPI Docker 镜像，以简化构建过程。
    2、关键点包括：
        2.1 自动与手动配置：
            - 如果环境已配置且 BLAS 启用，系统会自动选择 Intel MKL。
            - 否则，用户需手动安装 oneAPI 并按照指定步骤配置构建。
        2.2 使用 Intel 编译器和优化：
            - 通过指定 Intel 的编译器和 BLAS 供应商，确保项目能够利用 Intel 的硬件优化，提升性能。
        2.3 灵活的安装选项：
            - 提供了手动安装和使用 Docker 镜像两种方式，满足不同用户的需求和偏好。
    */
    ```

Check [Optimizing and Running LLaMA2 on Intel® CPU](https://www.intel.com/content/www/us/en/content-details/791610/optimizing-and-running-llama2-on-intel-cpu.html) for more information.

### 2.5 Other BLAS libraries

Any other BLAS library can be used by setting the `GGML_BLAS_VENDOR` option. See the [CMake documentation](https://cmake.org/cmake/help/latest/module/FindBLAS.html#blas-lapack-vendors) for a list of supported vendors.
```c
/*
Note:杨小兵-2025-01-11

1、这部分内容介绍的是BLAS其他的一些实现库
2、任何其他的BLAS库可以通过设置GGML_BLAS_VENDOR选项来使用。查看[CMake documentation](https://cmake.org/cmake/help/latest/module/FindBLAS.html#blas-lapack-vendors)文档来查看支持的vendors列表。
*/
```

## 3 Metal Build

On MacOS, Metal is enabled by default. Using Metal makes the computation run on the GPU.
To disable the Metal build at compile time use the `-DGGML_METAL=OFF` cmake option.

When built with Metal support, you can explicitly disable GPU inference with the `--n-gpu-layers 0` command-line argument.
```c
/*
Note:杨小兵-2025-01-11

1、在MacOS上，Metal默认情况下是可用的。使用Metal将会使得计算运行在GPU上而不是CPU。
2、为了在编译时期禁用Metal构建可以通过使用-DGGML_METAL=OFF选项。
3、如果已经使用Metal构建了项目，可以通过显式的--n-gpu-layers 0命令行参数来禁止GPU inference。
*/
```

## 4 SYCL

SYCL is a higher-level programming model to improve programming productivity on various hardware accelerators.

llama.cpp based on SYCL is used to **support Intel GPU** (Data Center Max series, Flex series, Arc series, Built-in GPU and iGPU).

For detailed info, please refer to [llama.cpp for SYCL](./backend/SYCL.md).
```c
/*
Note:杨小兵-2025-01-12

1、编程模型：SYCL（Single-source Yet Compilable Language）是一种基于 C++ 的异构并行编程标准，旨在简化在不同硬件加速器（如 CPU、GPU、FPGA）上编写高性能并行代码的过程。
2、库与标准：SYCL 由Khronos Group 制定，作为 OpenCL 的高级封装，提供了更高层次的抽象，使开发者能够使用现代 C++ 特性编写跨平台的并行代码。它不是一个具体的库，而是一个规范，具体的实现由不同的供应商和开源项目提供，例如 Intel 的 DPC++（Data Parallel C++）。
3、llama.cpp 采用了基于 SYCL 的实现，以支持多种 Intel GPU，包括 Data Center Max 系列、Flex 系列、Arc 系列以及内置 GPU 和集成显卡（iGPU）。
4、对于更多的信息，可以参考[llama.cpp for SYCL](./backend/SYCL.md)中的内容。
*/
```

## 5 CUDA

This provides GPU acceleration using an NVIDIA GPU. Make sure to have the CUDA toolkit installed. You can download it from your Linux distro's package manager (e.g. `apt install nvidia-cuda-toolkit`) or from the [NVIDIA developer site](https://developer.nvidia.com/cuda-downloads).
```c
/*
Note:杨小兵-2025-01-12

1、CUDA使用了NVIDIA GPU提供了GPU上的加速。确保在开发环境中已经有了安装的CUDA toolkit。你可以从你的linux发行版管理器中下载CUDA toolkit（例如可以使用apt install nvidia-cuda-toolkit）或者从[NVIDIA developer site](https://developer.nvidia.com/cuda-downloads)下载CUDA toolkit。
*/
```

- Using `CMake`:

  ```bash
  cmake -B build -DGGML_CUDA=ON
  cmake --build build --config Release
  ```

The environment variable [`CUDA_VISIBLE_DEVICES`](https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#env-vars) can be used to specify which GPU(s) will be used.
```c
/*
Note:杨小兵-2025-01-12

1、使用cmake对llama.cpp项目进行构建
2、CUDA_VISIBLE_DEVICE环境变量可以指定哪一个GPU将会被使用
3、CUDA_VISIBLE_DEVICES 是一个用于控制 CUDA 程序可见 GPU 设备的环境变量。通过设置该变量，用户可以指定程序在运行时将使用哪些 GPU。
*/
```

The environment variable `GGML_CUDA_ENABLE_UNIFIED_MEMORY=1` can be used to enable unified memory in Linux. This allows swapping to system RAM instead of crashing when the GPU VRAM is exhausted. In Windows this setting is available in the NVIDIA control panel as `System Memory Fallback`.
```c
/*
Note:杨小兵-2025-01-12

1、GGML_CUDA_ENABLE_UNIFIED_MEMORY环境变量被设置成1的时候可以在linux中被用作启用unified memory的开关。
2、GGML_CUDA_ENABLE_UNIFIED_MEMORY：这是一个环境变量，用于控制程序在使用 CUDA（Compute Unified Device Architecture）进行 GPU 加速计算时的内存管理策略。
3、统一内存（Unified Memory）：统一内存是一种内存管理技术，允许程序在 GPU 和系统 RAM 之间共享内存资源。当 GPU 的专用内存（VRAM）不足时，统一内存机制可以将部分数据迁移到系统 RAM，以避免程序崩溃。
4、启用统一内存：将 GGML_CUDA_ENABLE_UNIFIED_MEMORY 设置为 1，可以启用统一内存管理。这意味着当 GPU 的 VRAM 不足以容纳所需的数据时，系统会自动将部分数据交换到系统 RAM 中，而不是导致程序因内存不足而崩溃。
5、通过设置环境变量 GGML_CUDA_ENABLE_UNIFIED_MEMORY=1（在 Linux 上）或在 NVIDIA 控制面板中启用 System Memory Fallback（在 Windows 上），可以启用统一内存管理机制，使程序在 GPU 专用内存（VRAM）不足时自动使用系统 RAM 作为后备内存，从而避免程序因内存耗尽而崩溃。
*/
```

The following compilation options are also available to tweak performance:
```c
/*
Note:杨小兵-2025-01-12

1、下列这些编译选项对于调整性能也是可用的。
2、通过灵活的设置下列的这些编译选项我们可以调整可执行文件的性能。
*/
```

| Option                        | Legal values           | Default | Description                                                                                                                                                                                                                                                                             |
|-------------------------------|------------------------|---------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| GGML_CUDA_FORCE_MMQ           | Boolean                | false   | Force the use of custom matrix multiplication kernels for quantized models instead of FP16 cuBLAS even if there is no int8 tensor core implementation available (affects V100, RDNA3). MMQ kernels are enabled by default on GPUs with int8 tensor core support. With MMQ force enabled, speed for large batch sizes will be worse but VRAM consumption will be lower.<br/>【即使没有可用的 int8 张量核心实现，也强制对量化模型使用自定义矩阵乘法内核，而不是 FP16 cuBLAS（影响 V100、RDNA3）。在支持 int8 张量核心的 GPU 上，MMQ 内核默认启用。启用 MMQ 强制后，大批量处理的速度会变差，但 VRAM 消耗会降低。】                       |
| GGML_CUDA_FORCE_CUBLAS        | Boolean                | false   | Force the use of FP16 cuBLAS instead of custom matrix multiplication kernels for quantized models<br/>【强制使用 FP16 cuBLAS 代替量化模型的自定义矩阵乘法核】                                                                                                                                                                                       |
| GGML_CUDA_F16                 | Boolean                | false   | If enabled, use half-precision floating point arithmetic for the CUDA dequantization + mul mat vec kernels and for the q4_1 and q5_1 matrix matrix multiplication kernels. Can improve performance on relatively recent GPUs.<br/>【如果启用，则对 CUDA 反量化 + mul mat vec 内核以及 q4_1 和 q5_1 矩阵乘法内核使用半精度浮点运算。可以提高相对较新的 GPU 上的性能。】                                                           |
| GGML_CUDA_PEER_MAX_BATCH_SIZE | Positive integer       | 128     | Maximum batch size for which to enable peer access between multiple GPUs. Peer access requires either Linux or NVLink. When using NVLink enabling peer access for larger batch sizes is potentially beneficial.<br/>【启用多个 GPU 之间的对等访问的最大批处理大小。对等访问需要 Linux 或 NVLink。使用 NVLink 时，启用较大批处理大小的对等访问可能会带来好处。】                                                                         |
| GGML_CUDA_FA_ALL_QUANTS       | Boolean                | false   | Compile support for all KV cache quantization type (combinations) for the FlashAttention CUDA kernels. More fine-grained control over KV cache size but compilation takes much longer.<br/>【为 FlashAttention CUDA 内核编译所有 KV 缓存量化类型（组合）的支持。对 KV 缓存大小的控制更加细粒度，但编译时间更长。】                                                                                                  |

## 6 MUSA

This provides GPU acceleration using the MUSA cores of your Moore Threads MTT GPU. Make sure to have the MUSA SDK installed. You can download it from here: [MUSA SDK](https://developer.mthreads.com/sdk/download/musa).
```c
/*
Note:杨小兵-2025-01-12

1、MUSA 是 Moore Threads 提供的一个软件开发工具包（Software Development Kit，SDK），用于在其 MTT GPU 上实现 GPU 加速。具体来说，MUSA 作为一个软件库，提供了一系列的 API（应用程序接口）和工具，帮助开发者有效地利用 MTT GPU 的计算能力进行并行处理和加速计算任务。因此，MUSA 不仅仅是一个接口规范，而是一个完整的软件库，包含了实现 GPU 加速所需的各种组件和功能。
2、MUSA 的应用场景和作用
    2.1 在科学研究、工程模拟、气候预测等需要大量计算资源的领域，MUSA 可以利用 MTT GPU 的并行计算能力，加速复杂计算任务的执行，提高计算效率。
    2.2 训练深度神经网络通常需要大量的矩阵运算和并行计算，MUSA 提供的 GPU 加速能力可以显著缩短训练时间，提升模型开发效率。
    2.3 在计算机图形学、游戏开发和虚拟现实等领域，MUSA 可以加速图像渲染、实时图形处理等任务，提升用户体验和系统性能。
3、Moore Threads是国内一家设计、生产GPU的公司
*/
```

- Using `CMake`:

  ```bash
  cmake -B build -DGGML_MUSA=ON
  cmake --build build --config Release
  ```

The environment variable [`MUSA_VISIBLE_DEVICES`](https://docs.mthreads.com/musa-sdk/musa-sdk-doc-online/programming_guide/Z%E9%99%84%E5%BD%95/) can be used to specify which GPU(s) will be used.
```c
/*
Note:杨小兵-2025-01-12

1、环境变量MUSA_VISIBLE_DEVICES可以被用来指定哪些GPU将会被使用。
2、和NVIDIA公司相同的手法。
*/
```

The environment variable `GGML_CUDA_ENABLE_UNIFIED_MEMORY=1` can be used to enable unified memory in Linux. This allows swapping to system RAM instead of crashing when the GPU VRAM is exhausted.
```c
/*
Note:杨小兵-2025-01-12

1、环境变量GGML_CUDA_ENABLE_UNIFIED_MEMORY可以在linux中被用来启动unified memory。
2、当GPU VRAM耗尽的时候这个环境变量允许将一部分内容切换到system RAM中，这样做可以防止系统crashing。
*/
```

Most of the compilation options available for CUDA should also be available for MUSA, though they haven't been thoroughly tested yet.
```c
/*
Note:杨小兵-2025-01-12

1、CUDA上的大多数编译选项对MUSA也是可用的，尽管还没有进行充分的测试。
*/
```

## 7 HIP

This provides GPU acceleration on HIP-supported AMD GPUs.
Make sure to have ROCm installed.
You can download it from your Linux distro's package manager or from here: [ROCm Quick Start (Linux)](https://rocm.docs.amd.com/projects/install-on-linux/en/latest/tutorial/quick-start.html#rocm-install-quick).
```c
/*
Note:杨小兵-2025-01-12

1、这为 HIP 支持的 AMD GPU 提供了 GPU 加速。确保已安装 ROCm。可以通过Linux发布版的包管理在[ROCm Quick Start (Linux)](https://rocm.docs.amd.com/projects/install-on-linux/en/latest/tutorial/quick-start.html#rocm-install-quick)网址上下载ROCm。
2、HIP（Heterogeneous-Compute Interface for Portability）是一个 软件开发工具包（Software Development Kit，SDK），由 AMD 提供并作为 ROCm（Radeon Open Compute）生态系统的一部分。HIP 提供了一套完整的编程接口和工具，旨在帮助开发者编写可移植的高性能并行计算代码，使其能够在支持 HIP 的 AMD GPU 上运行。通过 HIP，开发者可以利用现有的 CUDA 代码库，较为容易地将其迁移到 AMD 的硬件平台上，从而实现跨平台的 GPU 加速。
3、HIP 的主要作用和必要性
    3.1 统一编程模型：HIP 提供了一套统一的编程接口，使得相同的代码可以在不同的 GPU 架构（如 AMD 和 NVIDIA）上编译和运行。这大大降低了针对不同硬件平台开发和维护多份代码的复杂性。
    3.2 简化代码迁移：开发者可以将现有的 CUDA 代码较为便捷地迁移到 HIP，从而在不大幅修改代码的情况下，支持 AMD GPU 的加速计算。
    3.3 HIP 作为一个强大的软件开发工具包，旨在简化高性能并行计算的开发过程，提供跨平台的代码可移植性，并充分发挥 AMD GPU 的计算能力。
*/
```

- Using `CMake` for Linux (assuming a gfx1030-compatible AMD GPU):
  ```bash
  HIPCXX="$(hipconfig -l)/clang" HIP_PATH="$(hipconfig -R)" \
      cmake -S . -B build -DGGML_HIP=ON -DAMDGPU_TARGETS=gfx1030 -DCMAKE_BUILD_TYPE=Release \
      && cmake --build build --config Release -- -j 16
  ```
  On Linux it is also possible to use unified memory architecture (UMA) to share main memory between the CPU and integrated GPU by setting `-DGGML_HIP_UMA=ON`.
  However, this hurts performance for non-integrated GPUs (but enables working with integrated GPUs).
    ```c
    /*
    Note:杨小兵-2025-01-12

    1、在 Linux 上启用统一内存架构可以实现 CPU 与集成 GPU 的内存共享，但会降低非集成 GPU 的性能。
    */
    ```
  Note that if you get the following error:
  ```
  clang: error: cannot find ROCm device library; provide its path via '--rocm-path' or '--rocm-device-lib-path', or pass '-nogpulib' to build without ROCm device library
  ```
  Try searching for a directory under `HIP_PATH` that contains the file
  `oclc_abi_version_400.bc`. Then, add the following to the start of the
  command: `HIP_DEVICE_LIB_PATH=<directory-you-just-found>`, so something
  like:
  ```bash
  HIPCXX="$(hipconfig -l)/clang" HIP_PATH="$(hipconfig -p)" \
  HIP_DEVICE_LIB_PATH=<directory-you-just-found> \
      cmake -S . -B build -DGGML_HIP=ON -DAMDGPU_TARGETS=gfx1030 -DCMAKE_BUILD_TYPE=Release \
      && cmake --build build -- -j 16
  ```

- Using `CMake` for Windows (using x64 Native Tools Command Prompt for VS, and assuming a gfx1100-compatible AMD GPU):
  ```bash
  set PATH=%HIP_PATH%\bin;%PATH%
  cmake -S . -B build -G Ninja -DAMDGPU_TARGETS=gfx1100 -DGGML_HIP=ON -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
  cmake --build build
  ```
  Make sure that `AMDGPU_TARGETS` is set to the GPU arch you want to compile for. The above example uses `gfx1100` that corresponds to Radeon RX 7900XTX/XT/GRE. You can find a list of targets [here](https://llvm.org/docs/AMDGPUUsage.html#processors)
  Find your gpu version string by matching the most significant version information from `rocminfo | grep gfx | head -1 | awk '{print $2}'` with the list of processors, e.g. `gfx1035` maps to `gfx1030`.


The environment variable [`HIP_VISIBLE_DEVICES`](https://rocm.docs.amd.com/en/latest/understand/gpu_isolation.html#hip-visible-devices) can be used to specify which GPU(s) will be used.
If your GPU is not officially supported you can use the environment variable [`HSA_OVERRIDE_GFX_VERSION`] set to a similar GPU, for example 10.3.0 on RDNA2 (e.g. gfx1030, gfx1031, or gfx1035) or 11.0.0 on RDNA3.
```c
/*
Note:杨小兵-2025-01-12

1、目前这部分内容是有关如何利用AMD GPU相关内容，暂时不是特别的重要，不必深入了解。
*/
```

## 8 Vulkan
```c
/*
Note:杨小兵-2025-01-12

1、Vulkan 是由 Khronos Group 组织开发和维护的一个跨平台、高效能的图形和计算 API。Vulkan 自2013年启动开发以来，通过持续的规范更新和生态系统建设，已经成为一个功能强大、性能优越的图形和计算 API，广泛应用于多个高性能计算和图形处理领域。
2、Vulkan 是一个接口规范（API 规范）
    2.1 接口规范：Vulkan 定义了一套标准化的编程接口和函数，允许开发者与图形处理单元（GPU）进行高效的通信和控制。这些接口涵盖了图形渲染、计算任务、资源管理等多个方面。
    2.2 实现与驱动：虽然 Vulkan 本身是一个规范，但其功能需要通过具体的实现来发挥作用。这些实现通常由 GPU 制造商（如 NVIDIA、AMD、Intel）提供的驱动程序来完成，驱动程序根据 Vulkan 规范与底层硬件交互。
    2.3 软件包与工具：在开发过程中，开发者可能会使用 Vulkan SDK（软件开发工具包），它包含了编译器、示例代码、文档和调试工具等，帮助开发者更方便地使用 Vulkan 进行开发。但 Vulkan 本身作为规范，不包含这些工具。
3、官方厂商驱动实现
    3.1 NVIDIA Vulkan 驱动
        3.1.1 支持平台：主要支持 Windows 和 Linux。
        3.1.2 特点：NVIDIA 为其 GeForce、Quadro 和 Tesla 系列显卡提供优化的 Vulkan 驱动，确保高性能渲染和计算任务的高效执行。
    3.2 AMD Vulkan 驱动
        3.2.1 支持平台：支持 Windows 和 Linux。
        3.2.2 特点：通过 Radeon 软件驱动，AMD 提供对其 Radeon 和 Radeon Pro 显卡的全面 Vulkan 支持，利用其 RDNA 和其他架构的优势进行优化。
    3.3 Intel Vulkan 驱动
        3.3.1 支持平台：支持 Windows 和 Linux。
        3.3.2 特点：Intel 为其集成显卡（如 Iris Xe）和部分独立显卡提供 Vulkan 驱动，确保在各种应用场景下的兼容性和性能。
*/
```

**Windows**

### 8.1 w64devkit

Download and extract [`w64devkit`](https://github.com/skeeto/w64devkit/releases).

Download and install the [`Vulkan SDK`](https://vulkan.lunarg.com/sdk/home#windows) with the default settings.

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

Switch into the `llama.cpp` directory and build using CMake.
```sh
cmake -B build -DGGML_VULKAN=ON
cmake --build build --config Release
```

### 8.2 Git Bash MINGW64

Download and install [`Git-SCM`](https://git-scm.com/downloads/win) with the default settings

Download and install [`Visual Studio Community Edition`](https://visualstudio.microsoft.com/) and make sure you select `C++`

Download and install [`CMake`](https://cmake.org/download/) with the default settings

Download and install the [`Vulkan SDK`](https://vulkan.lunarg.com/sdk/home#windows) with the default settings.

Go into your `llama.cpp` directory and right click, select `Open Git Bash Here` and then run the following commands

```
cmake -B build -DGGML_VULKAN=ON
cmake --build build --config Release
```

Now you can load the model in conversation mode using `Vulkan`

```sh
build/bin/Release/llama-cli -m "[PATH TO MODEL]" -ngl 100 -c 16384 -t 10 -n -2 -cnv
```

### 8.3 MSYS2
Install [MSYS2](https://www.msys2.org/) and then run the following commands in a UCRT terminal to install dependencies.
```sh
pacman -S git \
    mingw-w64-ucrt-x86_64-gcc \
    mingw-w64-ucrt-x86_64-cmake \
    mingw-w64-ucrt-x86_64-vulkan-devel \
    mingw-w64-ucrt-x86_64-shaderc
```

Switch into the `llama.cpp` directory and build using CMake.
```sh
cmake -B build -DGGML_VULKAN=ON
cmake --build build --config Release
```

**With docker**:

You don't need to install Vulkan SDK. It will be installed inside the container.

```sh
# Build the image
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

## 9 CANN
This provides NPU acceleration using the AI cores of your Ascend NPU. And [CANN](https://www.hiascend.com/en/software/cann) is a hierarchical APIs to help you to quickly build AI applications and service based on Ascend NPU.

For more information about Ascend NPU in [Ascend Community](https://www.hiascend.com/en/).

Make sure to have the CANN toolkit installed. You can download it from here: [CANN Toolkit](https://www.hiascend.com/developer/download/community/result?module=cann)

Go to `llama.cpp` directory and build using CMake.
```bash
cmake -B build -DGGML_CANN=on -DCMAKE_BUILD_TYPE=release
cmake --build build --config release
```

You can test with:

```bash
./build/bin/llama-cli -m PATH_TO_MODEL -p "Building a website can be done in 10 steps:" -ngl 32
```

If the following info is output on screen, you are using `llama.cpp` with the CANN backend:
```bash
llm_load_tensors:       CANN model buffer size = 13313.00 MiB
llama_new_context_with_model:       CANN compute buffer size =  1260.81 MiB
```

For detailed info, such as model/device supports, CANN install, please refer to [llama.cpp for CANN](./backend/CANN.md).

## 10 Android

To read documentation for how to build on Android, [click here](./android.md)

## 11 Notes about GPU-accelerated backends

The GPU may still be used to accelerate some parts of the computation even when using the `-ngl 0` option. You can fully disable GPU acceleration by using `--device none`.

In most cases, it is possible to build and use multiple backends at the same time. For example, you can build llama.cpp with both CUDA and Vulkan support by using the `-DGGML_CUDA=ON -DGGML_VULKAN=ON` options with CMake. At runtime, you can specify which backend devices to use with the `--device` option. To see a list of available devices, use the `--list-devices` option.

Backends can be built as dynamic libraries that can be loaded dynamically at runtime. This allows you to use the same llama.cpp binary on different machines with different GPUs. To enable this feature, use the `GGML_BACKEND_DL` option when building.
```c
/*
Note:杨小兵-2025-01-12

1、这部分内容是讲述一些关于 GPU 加速后端的说明
2、即使使用 `-ngl 0` 选项，GPU 仍可用于加速部分计算。您可以使用 `--device none` 完全禁用 GPU 加速。
3、在大多数情况下，可以同时构建和使用多个后端。例如，您可以使用 CMake 中的 `-DGGML_CUDA=ON -DGGML_VULKAN=ON` 选项构建同时支持 CUDA 和 Vulkan 的 llama.cpp。在运行时，您可以使用 `--device` 选项指定要使用的后端设备。要查看可用设备列表，请使用 `--list-devices` 选项。
4、后端可以构建为动态库，可以在运行时动态加载。这允许您在具有不同 GPU 的不同机器上使用相同的 llama.cpp 二进制文件。要启用此功能，请在构建时使用“GGML_BACKEND_DL”选项。（这个功能在分发软件的时候将会变得十分有用）
*/
```
