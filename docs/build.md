# Build llama.cpp locally
```c
/*
Notes:杨小兵-2025-04-09

1、该文件内容将会解释如何本地化构建llama.cpp项目。
2、GitHub的好处之一是存在GitHub actions，那么当新的特性在本地构建成功之后便可以使用GitHub actions来实现自动化构建流程，如果想要进行调试的话，那么本地构建是比较方便的，如果为了构建、发布、多人协作，还是GitHub actions是比较方便的。
*/
```

**To get the Code:**

```bash
git clone https://github.com/ggml-org/llama.cpp
cd llama.cpp
```

The following sections describe how to build with different backends and options.
```c
/*
Notes:杨小兵-2025-04-09

1、下列内容描述了如何使用不同的backends和options来构建llama.cpp。
  1.1 CPU Build
  1.2 BLAS Build
  1.3 Metal Build
  1.4 SYCL Build
  1.5 CUDA Build
  1.6 MUSA Build
  1.7 HIP Build
  1.8 Vulkan Build
  1.9 CANN Build
  1.10 Android Build
2、llama.cpp 中提到的后端包括 CPU、BLAS（CPU 优化）、Metal、SYCL、CUDA、MUSA、HIP、Vulkan、CANN 和 OpenCL，CPU是一种后端，可以同GPU后端、NPU后端一起理解为不同的计算硬件选择。（可以参考2025-04-09.docx文件内容）
*/
```

## CPU Build
```c
/*
Notes:杨小兵-2025-04-09

1、使用CMake构建以CPU为backend的llama.cpp。
2、具体来说就是llama.cpp全部计算将会运行在CPU上。
3、在需要进行以CPU为backend进行构建的时候再详细查看这部分内容。
*/
```

Build llama.cpp using `CMake`:

```bash
cmake -B build
cmake --build build --config Release
```

**Notes**:

- For faster compilation, add the `-j` argument to run multiple jobs in parallel, or use a generator that does this automatically such as Ninja. For example, `cmake --build build --config Release -j 8` will run 8 jobs in parallel.
- For faster repeated compilation, install [ccache](https://ccache.dev/)
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
- For static builds, add `-DBUILD_SHARED_LIBS=OFF`:
  ```
  cmake -B build -DBUILD_SHARED_LIBS=OFF
  cmake --build build --config Release
  ```

- Building for Windows (x86, x64 and arm64) with MSVC or clang as compilers:
    - Install Visual Studio 2022, e.g. via the [Community Edition](https://visualstudio.microsoft.com/vs/community/). In the installer, select at least the following options (this also automatically installs the required additional tools like CMake,...):
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

## BLAS Build
```c
/*
Notes:杨小兵-2025-04-09

1、使用CMake构建以CPU为backend的llama.cpp。
2、具体来说就是llama.cpp全部计算将会运行在CPU上。
3、BLAS（基本线性代数子程序）是一个在科学计算和工程领域广泛使用的标准接口，定义了一组低级例程，用于执行常见的线性代数操作，如向量加法、标量乘法、点积、线性组合和矩阵乘法。这些操作是许多算法的核心，尤其在机器学习和深度学习中，如神经网络的训练和推理。BLAS 的设计目标是提供高效、可移植的解决方案，允许开发者利用优化的库来加速计算，而无需从头实现复杂的线性代数操作。
  3.1 BLAS是一个接口规范并不是具体的实现，针对不同的硬件将会存在不同的BLAS具体实现，这样做的目的就是为了追求极致的性能。
  3.2 BLAS具体的实现有
    3.2.1 Accelerate Framework 专为 Mac 设备设计，利用苹果硬件的特性
    3.2.2 OpenBLAS 是一个通用的 CPU 实现，适用于多种平台
    3.2.3 Intel oneMKL 针对 Intel 处理器优化，利用特定指令集如 AVX
    3.2.4 BLIS 是一个开源框架，注重多核性能和可移植性
4、BLAS 解决了高效执行线性代数操作的问题，这些操作在许多计算密集型任务中是基础。例如，在机器学习中，矩阵乘法是训练神经网络的关键步骤，直接影响训练速度和资源利用率。BLAS 通过提供优化的例程，减少了开发者手动优化的负担，同时利用硬件的特定特性（如多核 CPU、矢量指令）来加速计算。

实现          平台          优化目标                      开源/专有
Accelerate    Framework     Mac	苹果硬件（VEC、多核）	     专有
OpenBLAS      通用 CPU      多线程、缓存优化	             开源
BLIS          通用 CPU      多核并行、可移植性	           开源
Intel         oneMKL        Intel 处理器	AVX、AVX512	    专有
cuBLAS        NVIDIA        GPU	GPU 加速、深度学习	      专有
*/
```

Building the program with BLAS support may lead to some performance improvements in prompt processing using batch sizes higher than 32 (the default is 512). Using BLAS doesn't affect the generation performance. There are currently several different BLAS implementations available for build and use:
```c
/*
Notes:杨小兵-2025-04-11

1、BLAS：是用来对基础矩阵运算进行加速的一个规范，存在很多针对不同进行具体实现的库。
2、LLM的处理步骤：分词、词嵌入、模型推理、解码。
  2.1 具体的内容还需要参考、学习更多的资料。
  2.2 这部分内容将会在后续进行补充完善。
*/
```

### Accelerate Framework

This is only available on Mac PCs and it's enabled by default. You can just build using the normal instructions.

```c
/*
Notes:杨小兵-2025-04-11

1、Accelerate Framework：是针对Mac硬件BLAS的具体实现，也是目前Mac上唯一可以用的BLAS库。默认情况下这个库是被自动启用的。可以使用正常的指令进行构建即可。
2、这部分内容的探索可以放在后续进行理解。
*/
```

### OpenBLAS

This provides BLAS acceleration using only the CPU. Make sure to have OpenBLAS installed on your machine.

- Using `CMake` on Linux:

    ```bash
    cmake -B build -DGGML_BLAS=ON -DGGML_BLAS_VENDOR=OpenBLAS
    cmake --build build --config Release
    ```

```c
/*
Notes:杨小兵-2025-04-11

1、OpenBLAS:在仅使用CPU的情况下OpenBLAS提供了BLAS加速。首先确保这个库已经在你的机器上安装了。
2、上述整体的构建方式采用的是CMake。
*/
```

### BLIS

Check [BLIS.md](./backend/BLIS.md) for more information.

```c
/*
Notes:杨小兵-2025-04-11

1、BLIS：对于BLIS相关内容通过查看BLIS.md从而获取更多的信息。
*/
```

### Intel oneMKL

Building through oneAPI compilers will make avx_vnni instruction set available for intel processors that do not support avx512 and avx512_vnni. Please note that this build config **does not support Intel GPU**. For Intel GPU support, please refer to [llama.cpp for SYCL](./backend/SYCL.md).

- Using manual oneAPI installation:
  By default, `GGML_BLAS_VENDOR` is set to `Generic`, so if you already sourced intel environment script and assign `-DGGML_BLAS=ON` in cmake, the mkl version of Blas will automatically been selected. Otherwise please install oneAPI and follow the below steps:
    ```bash
    source /opt/intel/oneapi/setvars.sh # You can skip this step if  in oneapi-basekit docker image, only required for manual installation
    cmake -B build -DGGML_BLAS=ON -DGGML_BLAS_VENDOR=Intel10_64lp -DCMAKE_C_COMPILER=icx -DCMAKE_CXX_COMPILER=icpx -DGGML_NATIVE=ON
    cmake --build build --config Release
    ```

- Using oneAPI docker image:
  If you do not want to source the environment vars and install oneAPI manually, you can also build the code using intel docker container: [oneAPI-basekit](https://hub.docker.com/r/intel/oneapi-basekit). Then, you can use the commands given above.

Check [Optimizing and Running LLaMA2 on Intel® CPU](https://www.intel.com/content/www/us/en/content-details/791610/optimizing-and-running-llama2-on-intel-cpu.html) for more information.
```c
/*
Notes:杨小兵-2025-04-11

1、Intel oneMKL：是针对intel硬件的BLAS的具体实现。
2、通过使用oneAPI编译器，开发者可以在不支持AVX512和AVX512_VNNI指令集的Intel处理器上启用AVX_VNNI指令集，从而提升应用程序的性能，同时提供了实现这一目标的具体方法和注意事项。
3、AVX_VNNI指令集的作用与启用方式
  3.1 AVX_VNNI是一种指令集扩展，它允许在不支持AVX512的Intel处理器上执行部分AVX512指令，从而优化计算任务的性能。
  3.2 通过oneAPI编译器构建项目时，即使处理器的硬件不支持AVX512或AVX512_VNNI，开发者仍然可以利用AVX_VNNI指令集来提升应用程序的执行效率。
4、不支持Intel GPU的限制
  4.1 这种构建配置不支持Intel GPU。如果开发者需要Intel GPU的支持，需要参考llama.cpp for SYCL的相关文档（具体链接为./backend/SYCL.md）
5、两种使用oneAPI启用AVX_VNNI指令集的方式
  5.1 开发者需要先安装oneAPI，并通过上述命令设置环境变量
  5.2 为了避免手动安装和配置环境变量的麻烦，开发者可以直接使用预配置的oneAPI-basekit docker镜像（镜像地址：https://hub.docker.com/r/intel/oneapi-basekit），然后运行上述cmake命令构建项目。
6、上述内容的重点在于指导开发者如何利用oneAPI编译器在不支持AVX512的Intel处理器上启用AVX_VNNI指令集，以提升性能。同时，它提醒了这种配置不支持Intel GPU，并提供了手动安装和docker镜像两种实现途径，方便开发者根据需求选择适合的方式。
*/
```

### Other BLAS libraries

Any other BLAS library can be used by setting the `GGML_BLAS_VENDOR` option. See the [CMake documentation](https://cmake.org/cmake/help/latest/module/FindBLAS.html#blas-lapack-vendors) for a list of supported vendors.
```c
/*
Notes:杨小兵-2025-04-11

1、任何其他的BLAS库可以通过设置GGML_BLAS_VENDOR设置来使用。查看具体的CMake文档查看具体支持的vendor列表。
2、后续使用其他BLAS构建llama.cpp项目的时候再详细查看这部分内容。
*/
```

## Metal Build

On MacOS, Metal is enabled by default. Using Metal makes the computation run on the GPU.
To disable the Metal build at compile time use the `-DGGML_METAL=OFF` cmake option.

When built with Metal support, you can explicitly disable GPU inference with the `--n-gpu-layers 0` command-line argument.

## SYCL

SYCL is a higher-level programming model to improve programming productivity on various hardware accelerators.

llama.cpp based on SYCL is used to **support Intel GPU** (Data Center Max series, Flex series, Arc series, Built-in GPU and iGPU).

For detailed info, please refer to [llama.cpp for SYCL](./backend/SYCL.md).
```c
/*
Notes:杨小兵-2025-04-11

1、基于SYCL的llama.cpp是用来支持 Intel GPU 加速计算的。
2、这部分内容需要重点关注一下，可以在内网环境中部署体量更大的推理模型，从而验证该项目中的RPC子项目的能力，这部分内容还需要深入了解。
*/
```

## CUDA

This provides GPU acceleration using an NVIDIA GPU. Make sure to have the [CUDA toolkit](https://developer.nvidia.com/cuda-toolkit) installed.
```c
/*
Notes:杨小兵-2025-04-11

1、这部分内容将会解释如何在llama.cpp项目中使用CUDA这个backend，目前这部分介绍的是使用CUDA后端构建llama.cpp项目。
2、可以使用CUDA来指挥NVIDIA GPU干活从而加速计算，确保已经安装了CUDA toolkit相关内容。
3、这部分在需要构建的时候可以详细理解这部分内容，目前只是理解起作用即可。
*/
```

#### Download directly from NVIDIA
You may find the official downloads here: [NVIDIA developer site](https://developer.nvidia.com/cuda-downloads).


#### Compile and run inside a Fedora Toolbox Container
We also have a [guide](./cuda-fedora.md) for setting up CUDA toolkit in a Fedora [toolbox container](https://containertoolbx.org/).

**Recommended for:**

- ***Particularly*** *convenient* for users of [Atomic Desktops for Fedora](https://fedoraproject.org/atomic-desktops/); such as: [Silverblue](https://fedoraproject.org/atomic-desktops/silverblue/) and [Kinoite](https://fedoraproject.org/atomic-desktops/kinoite/).
- Toolbox is installed by default: [Fedora Workstation](https://fedoraproject.org/workstation/) or [Fedora KDE Plasma Desktop](https://fedoraproject.org/spins/kde).
- *Optionally* toolbox packages are available: [Arch Linux](https://archlinux.org/), [Red Hat Enterprise Linux >= 8.5](https://www.redhat.com/en/technologies/linux-platforms/enterprise-linux), or [Ubuntu](https://ubuntu.com/download)


### Compilation
```bash
cmake -B build -DGGML_CUDA=ON
cmake --build build --config Release
```

### Override Compute Capability Specifications

If `nvcc` cannot detect your gpu, you may get compile-warnings such as:
 ```text
nvcc warning : Cannot find valid GPU for '-arch=native', default arch is used
```

To override the `native` GPU detection:

#### 1. Take note of the `Compute Capability` of your NVIDIA devices: ["CUDA: Your GPU Compute > Capability"](https://developer.nvidia.com/cuda-gpus).

```text
GeForce RTX 4090      8.9
GeForce RTX 3080 Ti   8.6
GeForce RTX 3070      8.6
```

#### 2. Manually list each varying `Compute Capability` in the `CMAKE_CUDA_ARCHITECTURES` list.

```bash
cmake -B build -DGGML_CUDA=ON -DCMAKE_CUDA_ARCHITECTURES="86;89"
```

### Runtime CUDA environmental variables

You may set the [cuda environmental variables](https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#env-vars) at runtime.

```bash
# Use `CUDA_VISIBLE_DEVICES` to hide the first compute device.
CUDA_VISIBLE_DEVICES="-0" ./build/bin/llama-server --model /srv/models/llama.gguf
```

### Unified Memory

The environment variable `GGML_CUDA_ENABLE_UNIFIED_MEMORY=1` can be used to enable unified memory in Linux. This allows swapping to system RAM instead of crashing when the GPU VRAM is exhausted. In Windows this setting is available in the NVIDIA control panel as `System Memory Fallback`.

### Performance Tuning

The following compilation options are also available to tweak performance:

| Option                        | Legal values           | Default | Description                                                                                                                                                                                                                                                                             |
|-------------------------------|------------------------|---------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| GGML_CUDA_FORCE_MMQ           | Boolean                | false   | Force the use of custom matrix multiplication kernels for quantized models instead of FP16 cuBLAS even if there is no int8 tensor core implementation available (affects V100, RDNA3). MMQ kernels are enabled by default on GPUs with int8 tensor core support. With MMQ force enabled, speed for large batch sizes will be worse but VRAM consumption will be lower.                       |
| GGML_CUDA_FORCE_CUBLAS        | Boolean                | false   | Force the use of FP16 cuBLAS instead of custom matrix multiplication kernels for quantized models                                                                                                                                                                                       |
| GGML_CUDA_F16                 | Boolean                | false   | If enabled, use half-precision floating point arithmetic for the CUDA dequantization + mul mat vec kernels and for the q4_1 and q5_1 matrix matrix multiplication kernels. Can improve performance on relatively recent GPUs.                                                           |
| GGML_CUDA_PEER_MAX_BATCH_SIZE | Positive integer       | 128     | Maximum batch size for which to enable peer access between multiple GPUs. Peer access requires either Linux or NVLink. When using NVLink enabling peer access for larger batch sizes is potentially beneficial.                                                                         |
| GGML_CUDA_FA_ALL_QUANTS       | Boolean                | false   | Compile support for all KV cache quantization type (combinations) for the FlashAttention CUDA kernels. More fine-grained control over KV cache size but compilation takes much longer.                                                                                                  |

## MUSA

This provides GPU acceleration using the MUSA cores of your Moore Threads MTT GPU. Make sure to have the MUSA SDK installed. You can download it from here: [MUSA SDK](https://developer.mthreads.com/sdk/download/musa).

- Using `CMake`:

  ```bash
  cmake -B build -DGGML_MUSA=ON
  cmake --build build --config Release
  ```

The environment variable [`MUSA_VISIBLE_DEVICES`](https://docs.mthreads.com/musa-sdk/musa-sdk-doc-online/programming_guide/Z%E9%99%84%E5%BD%95/) can be used to specify which GPU(s) will be used.

The environment variable `GGML_CUDA_ENABLE_UNIFIED_MEMORY=1` can be used to enable unified memory in Linux. This allows swapping to system RAM instead of crashing when the GPU VRAM is exhausted.

Most of the compilation options available for CUDA should also be available for MUSA, though they haven't been thoroughly tested yet.

## HIP

This provides GPU acceleration on HIP-supported AMD GPUs.
Make sure to have ROCm installed.
You can download it from your Linux distro's package manager or from here: [ROCm Quick Start (Linux)](https://rocm.docs.amd.com/projects/install-on-linux/en/latest/tutorial/quick-start.html#rocm-install-quick).

- Using `CMake` for Linux (assuming a gfx1030-compatible AMD GPU):
  ```bash
  HIPCXX="$(hipconfig -l)/clang" HIP_PATH="$(hipconfig -R)" \
      cmake -S . -B build -DGGML_HIP=ON -DAMDGPU_TARGETS=gfx1030 -DCMAKE_BUILD_TYPE=Release \
      && cmake --build build --config Release -- -j 16
  ```
  On Linux it is also possible to use unified memory architecture (UMA) to share main memory between the CPU and integrated GPU by setting `-DGGML_HIP_UMA=ON`.
  However, this hurts performance for non-integrated GPUs (but enables working with integrated GPUs).

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

## Vulkan

**Windows**

### w64devkit

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

### Git Bash MINGW64

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

### MSYS2
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
docker build -t llama-cpp-vulkan --target light -f .devops/vulkan.Dockerfile .

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

## CANN
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

## Android

To read documentation for how to build on Android, [click here](./android.md)

## Notes about GPU-accelerated backends

The GPU may still be used to accelerate some parts of the computation even when using the `-ngl 0` option. You can fully disable GPU acceleration by using `--device none`.

In most cases, it is possible to build and use multiple backends at the same time. For example, you can build llama.cpp with both CUDA and Vulkan support by using the `-DGGML_CUDA=ON -DGGML_VULKAN=ON` options with CMake. At runtime, you can specify which backend devices to use with the `--device` option. To see a list of available devices, use the `--list-devices` option.

Backends can be built as dynamic libraries that can be loaded dynamically at runtime. This allows you to use the same llama.cpp binary on different machines with different GPUs. To enable this feature, use the `GGML_BACKEND_DL` option when building.
```c
/*
Notes:杨小兵-2025-04-11

1、GPU加速的控制
  1.1 部分启用：即使在使用 -ngl 0 选项（通常用于禁用 GPU 层）时，GPU 仍可能被用于加速计算的某些部分（也就是说这种方式不能完全禁止GPU的使用，但具体应该不会存在这种类似的需求）。
  1.2 完全禁用：若需彻底禁用 GPU 加速，可以使用 --device none 选项。
2、多种后端支持
  2.1 同时构建：llama.cpp 支持同时构建多个 GPU 后端。例如，通过在 CMake 中设置 -DGGML_CUDA=ON -DGGML_VULKAN=ON，可以启用 CUDA 和 Vulkan 支持（影响一个主机可能包含两个不同的 GPU 后端，另外 GPU集群也可能是由不同的 GPU 硬件组成的）。
  2.2 灵活性：这种设计允许用户根据需求选择不同的后端技术。
3、运行时配置
  3.1 设备选择：在运行时，可通过 --device 选项指定使用哪个后端设备。
  3.2 设备查询：使用 --list-devices 选项可以查看当前系统中可用的设备列表，方便管理和调试。
4、动态库支持
  4.1 动态加载：后端可以构建为动态库，在运行时动态加载。这种方式使得同一份 llama.cpp 二进制文件能够在不同机器和 GPU 环境下使用。
  4.2 启用方式：构建时需使用 GGML_BACKEND_DL 选项以启用此功能。
*/
```