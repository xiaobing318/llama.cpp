cmake_minimum_required(VERSION 3.14) # for add_link_options and implicit target directories.
project("ggml" C CXX)
include(CheckIncludeFileCXX)
#[[
Note:杨小兵-2025-01-09

1、指令1
    1.1 作用：指定 CMake 的最低版本要求为 3.14，对于CMake版本大于等于3.14的都是符合要求的，其中VERSION是cmake中的一个关键词
    1.2 注释解释：支持两个Features（在目前实际情况中没有遇到类似的需求）
        1.2.1 `add_link_options`
        1.2.2 隐式目标目录
    1.2 原因：在当前项目的构建过程中使用到了cmake高版本中的功能，但是在cmake低版本中没有对应的功能，因此需要指定cmake的版本
2、指令2
    2.1 定义项目信息：project 命令为整个 CMake 配置过程设定了基础信息，如项目名称、版本和支持的编程语言。
    2.2 启用语言支持：指定的编程语言（如 C、C++）将启用对应的编译器和语言特性。CMake 会自动设置编译器、编译选项等相关配置。
    2.3 配置生成器：许多 CMake 功能和模块依赖于 project 命令中定义的信息。例如，测试框架、安装规则等都可能依赖项目名称和语言。
    2.4 全局变量设置：project 命令会设置一些全局变量，如 PROJECT_NAME, PROJECT_VERSION, CMAKE_C_COMPILER, CMAKE_CXX_COMPILER 等，这些变量在后续的配置和构建过程中可供使用。
    2.5 依赖管理：在多项目构建（例如使用 add_subdirectory）中，project 命令有助于管理不同子项目的独立性和依赖关系。
3、指令3
    3.1 在 CMake 中，include 命令用于包含和执行另一个 CMake 脚本或模块。通过 include，您可以引入外部定义的函数、宏、变量和其他设置，从而在当前的 CMakeLists.txt 文件中复用这些内容
        3.1.1 引入模块和功能：CMake 提供了许多内置模块，包含常见的功能检查、编译选项设置等。通过 include，您可以使用这些模块提供的功能。
        3.1.2 复用代码： 如果您有多个项目或多个部分需要共享相同的 CMake 代码，可以将这些代码放在一个独立的 CMake 脚本中，然后在需要的地方通过 include 引入。
        3.1.3 组织项目结构： 对于大型项目，可以将不同的构建配置和逻辑分割到多个 CMake 脚本中，通过 include 进行组合，提升可读性和维护性。
    3.2 这里提到的模块指的就是对应的*.cmake文件，本质上就是一个脚本文件，只不过这里的脚本文件是被cmake解析器来进行解析的。
    3.3 示例：引入自定义的 CMake 脚本（include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/custom_settings.cmake)）
    3.4 CheckIncludeFileCXX.cmake该模块用于检查系统中是否存在特定的 C++ 头文件。它通过尝试编译包含该头文件的简单测试程序来确定头文件的可用性。
]]


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#[[
Note:杨小兵-2025-01-10

1、指令1
    1.1 在 CMake 中，set 命令用于定义或修改变量的值。这些变量可以在整个 CMake 配置过程中被引用（可以在子级cmake脚本中使用父级设置的变量），以控制构建过程的各种方面。
    1.2 在 CMake 中，CMAKE_EXPORT_COMPILE_COMMANDS 是一个用于控制是否生成 compile_commands.json 文件的变量。通过设置 CMAKE_EXPORT_COMPILE_COMMANDS 为 ON，CMake 会在构建目录中生成一个 compile_commands.json 文件。这个文件详细记录了项目中每个源文件的编译命令和编译选项。
    1.3 set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 的主要作用是让 CMake 生成一个包含所有源文件编译命令的 compile_commands.json 文件。这对于静态分析工具、代码编辑器的智能功能以及其他依赖编译信息的工具非常有用。通过启用这一选项，可以显著提升开发过程中的代码质量检查和开发效率。
    1.4 Visual Studio 生成器不支持 compile_commands.json：CMake 的 CMAKE_EXPORT_COMPILE_COMMANDS 选项在大多数 Unix-like 生成器（如 Ninja 和 Unix Makefiles）中是支持的，但对于 Visual Studio 生成器，它并不生成 compile_commands.json 文件。
]]

if (NOT XCODE AND NOT MSVC AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()
#[[
Note:杨小兵-2025-01-10

1、指令1
    1.1 检查当前生成器是否不是 Xcode
    1.2 检查当前生成器是否不是 Microsoft Visual Studio（MSVC）
    1.3 检查是否未设置 CMAKE_BUILD_TYPE 变量
    1.4 XCODE：这是一个由 CMake 自动定义的变量，当使用 Xcode 生成器时，其值为 TRUE。
    1.5 MSVC：这是另一个由 CMake 自动定义的变量，当使用 Microsoft Visual Studio 生成器时，其值为 TRUE。
    1.6 CMAKE_BUILD_TYPE：这是一个用于指定构建类型的变量，常见值包括 Debug、Release、MinSizeRel、RelWithDebInfo。它主要用于单配置生成器（如 Unix Makefiles 和 Ninja），而对于多配置生成器（如 Visual Studio 和 Xcode），此变量通常不使用。
2、指令2
    2.1 设置 CMAKE_BUILD_TYPE 变量的默认值为 Release。
3、指令3
    3.1 为缓存变量 CMAKE_BUILD_TYPE 定义可选的构建类型。这不仅提供了用户友好的选项列表，还有助于防止用户输入错误的构建类型。
4、指令4
    4.1 结束条件分支
]]

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(GGML_STANDALONE ON)

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

    # configure project version
    # TODO
else()
    set(GGML_STANDALONE OFF)
endif()
#[[
Note:杨小兵-2025-01-10

1、指令1
    1.1 判断当前目录是否是项目的顶层目录，若是，则启用独立构建模式（GGML_STANDALONE），并加载 git-vars 配置；否则禁用独立模式（GGML_STANDALONE）
    1.2 独立构建模式：开发独立工具或库时，用于单独测试、发布或调试。
    1.3 非独立构建模式：项目被集成到更大系统中，如作为一个子模块或插件时，采用这种模式简化管理。
]]

if (EMSCRIPTEN)
    set(BUILD_SHARED_LIBS_DEFAULT OFF)

    option(GGML_WASM_SINGLE_FILE "ggml: embed WASM inside the generated ggml.js" ON)
else()
    if (MINGW)
        set(BUILD_SHARED_LIBS_DEFAULT OFF)
    else()
        set(BUILD_SHARED_LIBS_DEFAULT ON)
    endif()
endif()
#[[
Note:杨小兵-2025-01-10

1、总体解释
    这段命令主要根据不同的构建环境设置共享库（shared libraries）的默认构建选项，并为特定环境（如 EMSCRIPTEN）启用额外选项：
2、判断是否使用 EMSCRIPTEN 工具链：
    2.1 如果 EMSCRIPTEN 被定义：
        2.1.1 禁用共享库的默认构建（BUILD_SHARED_LIBS_DEFAULT OFF）。
        2.1.2 定义一个选项 LLAMA_WASM_SINGLE_FILE，用于控制是否将 WebAssembly 文件嵌入生成的 JavaScript 文件中，默认值为 ON。
    2.2 非 EMSCRIPTEN 环境：
        2.2.1 如果当前环境是 MinGW（Windows 的 GCC 工具链）：
            2.2.1.1 禁用共享库的默认构建（BUILD_SHARED_LIBS_DEFAULT OFF）。
        2.2.2 否则（其他环境，例如 Linux、visual studio、macOS）：
            2.2.2.1 启用共享库的默认构建（BUILD_SHARED_LIBS_DEFAULT ON）。
3、EMSCRIPTEN 是什么？
EMSCRIPTEN 是一个开源工具链，主要用于将 C/C++ 代码编译为 WebAssembly（WASM）或 asm.js，从而使这些代码能够在 Web 浏览器中运行。其特点包括：
    3.1 支持将原生应用程序的代码迁移到浏览器环境。
    3.2 允许开发者运行性能接近原生的应用程序。
    3.3 提供与 Web 环境的交互功能（如文件系统、网络 API 等）。
4、LLAMA_WASM_SINGLE_FILE 是什么？
LLAMA_WASM_SINGLE_FILE 是一个自定义选项，用于控制生成的 WebAssembly（WASM）文件的打包方式。其作用是决定是否将 WASM 文件嵌入到生成的 JavaScript 文件中。
    4.1 如果启用（ON）：
        4.1.1 将 WASM 文件嵌入到 llama.js 中，生成一个单一的输出文件，便于分发和部署。
        4.1.2 适合需要减少文件数量的场景，如在浏览器中直接加载。
    4.2 如果禁用（OFF）：
        4.2.1 WASM 文件作为单独的文件输出，适合需要分开管理文件的场景。
5、option 命令的作用是什么？
option 是 CMake 提供的一个命令，用于定义布尔类型（ON/OFF）的配置选项，供用户在项目配置时选择。
    5.1 语法：option(<option_name> "<help_text>" <default_value>)
    5.2 <option_name>：选项名称，用户可以通过命令行或配置工具（如 ccmake 或 cmake-gui）设置其值。
    5.3 <help_text>：选项的说明文本，便于用户理解选项的功能。
    5.4 <default_value>：选项的默认值（ON 或 OFF）。
]]

# remove the lib prefix on win32 mingw
if (WIN32)
    set(CMAKE_STATIC_LIBRARY_PREFIX "")
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
    set(CMAKE_SHARED_MODULE_PREFIX  "")
endif()
#[[
Note:杨小兵-2025-01-10

1、注释1
    1.1 在 Windows 的 MinGW 环境下，默认情况下库文件名会带有前缀 lib（例如 libmylib.a）。这行注释表明接下来的设置将移除这种前缀。
2、指令1
    2.1 开始一个条件判断块。
    2.2 WIN32 是 CMake 的一个内置变量，当在 Windows 平台上运行 CMake 时，该变量为真（TRUE）。这行代码的意思是“如果当前构建平台是 Windows，则执行接下来的指令”。
    2.3 WIN32 是用来判断当前平台是否为windows平台，包含32位和64位并不单指32位
3、指令2
    3.1 设置静态库的前缀：默认情况下，CMake 在生成静态库时会添加前缀 lib（例如 libmylib.a）。这条指令将 CMAKE_STATIC_LIBRARY_PREFIX 变量设置为空字符串，意味着生成的静态库将不再带有 lib 前缀（例如 mylib.a）。
    3.2 设置动态库的前缀：默认情况下，CMake 在生成共享库时也会添加前缀 lib（例如 libmylib.dll 或 libmylib.so）。这条指令将 CMAKE_SHARED_LIBRARY_PREFIX 变量设置为空字符串，生成的共享库将不带有 lib 前缀（例如 mylib.dll 或 mylib.so）。
    3.3 设置共享模块的前缀：共享模块（通常用于插件或动态加载的模块）默认也会带有 lib 前缀。这条指令将 CMAKE_SHARED_MODULE_PREFIX 变量设置为空字符串，使生成的共享模块不带有 lib 前缀。
4、指令3
    4.1 标志着 if (WIN32) 条件块的结束，表示只有在 Windows 平台下，前面的 set 指令才会被执行。
]]

option(BUILD_SHARED_LIBS "ggml: build shared libraries" ${BUILD_SHARED_LIBS_DEFAULT})
option(GGML_BACKEND_DL   "ggml: build backends as dynamic libraries (requires BUILD_SHARED_LIBS)" OFF)
#[[
Note:杨小兵-2025-01-10

1、指令1
    1.1 提供一个Boolean类型的选项从而用户可以进行选择，相当于提供了一个开关
2、指令2
    2.1 提供一个Boolean类型的选项从而用户可以进行选择，相当于提供了一个开关
]]


#
# option list
#

# TODO: mark all options as advanced when not GGML_STANDALONE

if (APPLE)
    set(GGML_METAL_DEFAULT ON)
    set(GGML_BLAS_DEFAULT ON)
    set(GGML_BLAS_VENDOR_DEFAULT "Apple")
else()
    set(GGML_METAL_DEFAULT OFF)
    set(GGML_BLAS_DEFAULT OFF)
    set(GGML_BLAS_VENDOR_DEFAULT "Generic")
endif()
#[[
Note:杨小兵-2025-01-10

1、指令1
    1.1 开始一个条件判断块
    1.2 APPLE 是 CMake 的一个内置变量，当在 APPLE 平台上运行 CMake 时，该变量为真（TRUE）。这行代码的意思是“如果当前构建平台是 APPLE，则执行接下来的指令”。
2、指令2
    2.1 将boolean类型的变量设置为ON
3、指令3
    3.1 将boolean类型的变量设置为ON
4、指令4
    4.1 将字符串类型的变量设置为Apple
5、指令5
    5.1 开始另外分支的判断块
6、指令6
    6.1 将boolean类型的变量设置为ON
7、指令7
    7.1 将boolean类型的变量设置为ON
8、指令8
    8.1 将字符串类型的变量设置为Generic
]]


if (CMAKE_CROSSCOMPILING)
    set(GGML_NATIVE_DEFAULT OFF)
else()
    set(GGML_NATIVE_DEFAULT ON)
endif()
#[[
Note:杨小兵-2025-01-10

1、指令1
    1.1 开始一个条件判断块，检查当前是否在进行交叉编译
    1.2 这是 CMake 的一个内置变量，用于指示当前是否处于交叉编译环境。
    1.3 交叉编译（Cross-compiling）：指在一种平台（宿主平台）上编译生成另一种平台（目标平台）可执行的代码。例如，在 Windows 上为 Linux 构建可执行文件，或在 x86 架构上为 ARM 构建代码。
2、指令2
    2.1 自定义变量，名称中包含项目名称（如 GGML），通常用于控制项目中特定功能或选项的启用状态。
3、指令3
    3.1 如果当前不是在进行交叉编译，即在本地编译环境中，则执行 else 块中的代码。
4、指令4
    4.1 在非交叉编译的情况下，将变量 GGML_NATIVE_DEFAULT 设置为 ON。
5、指令5
    5.1 标志着 if 条件块的结束，确保条件判断的范围明确。
]]

# defaults
if (NOT GGML_LLAMAFILE_DEFAULT)
    set(GGML_LLAMAFILE_DEFAULT OFF)
endif()
#[[
Note:杨小兵-2025-01-10

1、指令1
    1.1 如果对应的变量没有定义则需要将代码块中的Boolean设置成OFF
]]

if (NOT GGML_CUDA_GRAPHS_DEFAULT)
    set(GGML_CUDA_GRAPHS_DEFAULT OFF)
endif()
#[[
Note:杨小兵-2025-01-10

1、指令1
    1.1 如果对应的变量没有定义则需要将代码块中的Boolean设置成OFF
]]

# general
option(GGML_STATIC "ggml: static link libraries"         OFF)
option(GGML_NATIVE "ggml: enable -march=native flag"     ${GGML_NATIVE_DEFAULT})
option(GGML_LTO    "ggml: enable link time optimization" OFF)
option(GGML_CCACHE "ggml: use ccache if available"       ON)
#[[
Note:杨小兵-2025-01-10

1、指令1
    1.1 提供一个选项来启用或禁用 ggml 的静态链接库，默认设置为关闭。
2、指令2
    2.1 提供一个选项来启用 -march=native 编译标志，以优化 ggml 的性能，默认值由 GGML_NATIVE_DEFAULT 决定。
3、指令3
    3.1 提供一个选项来启用 ggml 的链接时优化（LTO），默认设置为关闭。
4、指令4
    4.1 提供一个选项来在 ggml 项目中使用 ccache 以加速编译过程，默认设置为开启。
]]


# debug
option(GGML_ALL_WARNINGS           "ggml: enable all compiler warnings"                   ON)
option(GGML_ALL_WARNINGS_3RD_PARTY "ggml: enable all compiler warnings in 3rd party libs" OFF)
option(GGML_GPROF                  "ggml: enable gprof"                                   OFF)
#[[
Note:杨小兵-2025-01-10

1、指令1
    1.1 提供一个选项来启用编译器的所有警告，默认是开启
2、指令2
    2.1 提供一个选项来启用 ggml 项目中第三方库的所有编译器警告，默认设置为关闭。
3、指令3
    3.1 提供一个选项来启用 ggml 项目中的 gprof 性能分析工具，默认设置为关闭。
]]

# build
option(GGML_FATAL_WARNINGS    "ggml: enable -Werror flag"    OFF)
#[[
Note:杨小兵-2025-01-10

1、指令1
    1.1 提供一个选项来启用 ggml 项目的 -Werror 编译标志，将所有编译警告视为错误，默认设置为关闭。
]]

# sanitizers
option(GGML_SANITIZE_THREAD    "ggml: enable thread sanitizer"    OFF)
option(GGML_SANITIZE_ADDRESS   "ggml: enable address sanitizer"   OFF)
option(GGML_SANITIZE_UNDEFINED "ggml: enable undefined sanitizer" OFF)
#[[
Note:杨小兵-2025-01-10

1、指令1
    1.1 提供一个选项来启用 ggml 项目的线程内存错误检测（Thread Sanitizer），默认设置为关闭。
2、指令2
    2.1 提供一个选项来启用 ggml 项目的地址内存错误检测（Address Sanitizer），默认设置为关闭。
3、指令3
    3.1 提供一个选项来启用 ggml 项目的未定义行为检测（Undefined Behavior Sanitizer），默认设置为关闭。
]]

# instruction set specific
if (GGML_NATIVE OR NOT GGML_NATIVE_DEFAULT)
    set(INS_ENB OFF)
else()
    set(INS_ENB ON)
endif()
#[[
Note:杨小兵-2025-01-10

1、注意
    1.1 当启用 GGML_NATIVE 或禁用 GGML_NATIVE_DEFAULT 时，将 INS_ENB 设置为 OFF，否则将其设置为 ON。
    1.2 INS_ENB：可能用于控制是否启用某些特定的指令集优化或功能。当本地优化被启用或默认本地优化被禁用时，相关的指令集优化也会被禁用，以确保兼容性或避免潜在的问题。
]]


option(GGML_CPU_HBM          "ggml: use memkind for CPU HBM" OFF)
option(GGML_CPU_AARCH64      "ggml: use runtime weight conversion of Q4_0 to Q4_X_X" ON)
option(GGML_AVX              "ggml: enable AVX"              ${INS_ENB})
option(GGML_AVX_VNNI         "ggml: enable AVX-VNNI"         OFF)
option(GGML_AVX2             "ggml: enable AVX2"             ${INS_ENB})
option(GGML_AVX512           "ggml: enable AVX512F"          OFF)
option(GGML_AVX512_VBMI      "ggml: enable AVX512-VBMI"      OFF)
option(GGML_AVX512_VNNI      "ggml: enable AVX512-VNNI"      OFF)
option(GGML_AVX512_BF16      "ggml: enable AVX512-BF16"      OFF)
if (NOT MSVC)
    # in MSVC F16C and FMA is implied with AVX2/AVX512
    option(GGML_FMA          "ggml: enable FMA"              ${INS_ENB})
    option(GGML_F16C         "ggml: enable F16C"             ${INS_ENB})
    # MSVC does not seem to support AMX
    option(GGML_AMX_TILE     "ggml: enable AMX-TILE"         OFF)
    option(GGML_AMX_INT8     "ggml: enable AMX-INT8"         OFF)
    option(GGML_AMX_BF16     "ggml: enable AMX-BF16"         OFF)
endif()
option(GGML_LASX             "ggml: enable lasx"             ON)
option(GGML_LSX              "ggml: enable lsx"              ON)
option(GGML_RVV              "ggml: enable rvv"              ON)
option(GGML_SVE              "ggml: enable SVE"              OFF)
option(GGML_CPU_ALL_VARIANTS "ggml: build all variants of the CPU backend (requires GGML_BACKEND_DL)" OFF)
#[[
Note:杨小兵-2025-01-10

1、指令1
    1.1 控制是否在 CPU 上使用 memkind 库来管理 HBM（High Bandwidth Memory）内存。
2、指令2
    2.1 控制是否在 AArch64 架构下对权重（Q4_0 格式）进行运行时转换，以支持新的量化格式（Q4_X_X）。
3、指令3
    3.1 控制是否启用 AVX 指令集，以利用相关向量化加速特性。
4、指令4
    4.1 控制是否启用 AVX-VNNI 指令集，对特定向量化神经网络运算进行加速。
5、指令5
    5.1 控制是否启用 AVX2 指令集，以进一步提升向量计算性能。
6、指令6
    6.1 控制是否启用 AVX-512F 指令集，为更高宽度的向量化运算提供支持。
7、指令7
    7.1 控制是否启用 AVX-512 VBMI 指令集，用于字节和混合操作的更高效处理。
8、指令8
    8.1 控制是否启用 AVX-512 VNNI 指令集，以加速推理和训练中的卷积操作等神经网络核心计算。
9、指令9
    9.1 控制是否启用 AVX-512 BF16 指令集，以支持 BFloat16 格式的高性能计算。
10、指令10
    10.1 控制是否启用 FMA（Fused Multiply-Add）指令，用于在乘法和加法操作中提供更高的计算效率。
11、指令11
    11.1 控制是否启用 F16C 指令集，用于在 FP16（半精度浮点）和 FP32（单精度浮点）之间进行快速转换。
12、指令12
    12.1 控制是否启用 AMX-TILE 指令，用于 Intel AMX（Advanced Matrix Extensions）中的 Tile 配置与管理。
13、指令13
    13.1 控制是否启用 AMX-INT8 指令，用于在 AMX 硬件中进行 INT8 量化矩阵运算加速。
14、指令14
    14.1 控制是否启用 AMX-BF16 指令，以支持在 AMX 硬件中的 BFloat16 格式矩阵运算加速。
15、指令15
    15.1 控制是否启用 LASX 指令集（Loongson Advanced SIMD eXtensions），在龙芯架构上进行更高效的向量化运算。
16、指令16
    16.1 控制是否启用 LSX 指令集（Loongson SIMD eXtensions），在龙芯架构上启用基础的向量化支持。
17、指令17
    17.1 控制是否启用 RVV（RISC-V Vector）指令集，为 RISC-V 架构提供可变长度向量扩展支持。
18、指令18
    18.1 控制是否启用 SVE（Scalable Vector Extension），为 ARMv8-A 架构下的可扩展向量化提供支持。
19、指令19
    19.1 控制是否在启用 GGML_BACKEND_DL（动态加载后端）的前提下，构建 CPU 后端的所有指令集变体。
]]


if (WIN32)
    set(GGML_WIN_VER "0x602" CACHE STRING   "ggml: Windows version")
endif()
#[[
Note:杨小兵-2025-01-10

1、指令1
    1.1 检查当前构建环境是否为 Windows 平台，如果是则执行后续指令，否则跳过。
    1.2 这里的WIN32是当cmake构建项目的时候会自动设置的一个内部变量，用来标识当前平台是否为windows平台包含32位和64位
2、指令2
    2.1 在 Windows 平台时，定义一个名为 GGML_WIN_VER 的 CMake 缓存变量，其值设为 "0x602"（对应 Windows 8），并为该变量添加描述信息 "ggml: Windows version"。
3、指令3
    3.1 结束 if (WIN32) 判断语句的范围。
]]

# ggml core
set(GGML_SCHED_MAX_COPIES  "4" CACHE STRING "ggml: max input copies for pipeline parallelism")
option(GGML_CPU                             "ggml: enable CPU backend"                        ON)
#[[
Note:杨小兵-2025-01-10

1、指令1
    1.1 将名为 GGML_SCHED_MAX_COPIES 的 CMake 缓存变量设置为 "4"，用于指定在流水线并行（pipeline parallelism）中允许的最大输入数据副本数量。
    1.2 这里的WIN32是当cmake构建项目的时候会自动设置的一个内部变量，用来标识当前平台是否为windows平台包含32位和64位
2、指令2
    2.1 定义一个布尔型选项 GGML_CPU 并默认启用，用于决定是否开启 CPU 后端的支持。
]]

# 3rd party libs / backends
option(GGML_ACCELERATE                      "ggml: enable Accelerate framework"               ON)
option(GGML_BLAS                            "ggml: use BLAS"                                  ${GGML_BLAS_DEFAULT})
set(GGML_BLAS_VENDOR ${GGML_BLAS_VENDOR_DEFAULT} CACHE STRING
                                            "ggml: BLAS library vendor")
option(GGML_LLAMAFILE                       "ggml: use LLAMAFILE"                             ${GGML_LLAMAFILE_DEFAULT})

option(GGML_CUDA                            "ggml: use CUDA"                                  OFF)
option(GGML_MUSA                            "ggml: use MUSA"                                  OFF)
option(GGML_CUDA_FORCE_MMQ                  "ggml: use mmq kernels instead of cuBLAS"         OFF)
option(GGML_CUDA_FORCE_CUBLAS               "ggml: always use cuBLAS instead of mmq kernels"  OFF)
option(GGML_CUDA_F16                        "ggml: use 16 bit floats for some calculations"   OFF)
set   (GGML_CUDA_PEER_MAX_BATCH_SIZE "128" CACHE STRING
                                            "ggml: max. batch size for using peer access")
option(GGML_CUDA_NO_PEER_COPY               "ggml: do not use peer to peer copies"            OFF)
option(GGML_CUDA_NO_VMM                     "ggml: do not try to use CUDA VMM"                OFF)
option(GGML_CUDA_FA_ALL_QUANTS              "ggml: compile all quants for FlashAttention"     OFF)
option(GGML_CUDA_GRAPHS                     "ggml: use CUDA graphs (llama.cpp only)"          ${GGML_CUDA_GRAPHS_DEFAULT})

option(GGML_HIP                             "ggml: use HIP"                                   OFF)
option(GGML_HIP_UMA                         "ggml: use HIP unified memory architecture"       OFF)
option(GGML_VULKAN                          "ggml: use Vulkan"                                OFF)
option(GGML_VULKAN_CHECK_RESULTS            "ggml: run Vulkan op checks"                      OFF)
option(GGML_VULKAN_DEBUG                    "ggml: enable Vulkan debug output"                OFF)
option(GGML_VULKAN_MEMORY_DEBUG             "ggml: enable Vulkan memory debug output"         OFF)
option(GGML_VULKAN_SHADER_DEBUG_INFO        "ggml: enable Vulkan shader debug info"           OFF)
option(GGML_VULKAN_PERF                     "ggml: enable Vulkan perf output"                 OFF)
option(GGML_VULKAN_VALIDATE                 "ggml: enable Vulkan validation"                  OFF)
option(GGML_VULKAN_RUN_TESTS                "ggml: run Vulkan tests"                          OFF)
option(GGML_KOMPUTE                         "ggml: use Kompute"                               OFF)
option(GGML_METAL                           "ggml: use Metal"                                 ${GGML_METAL_DEFAULT})
option(GGML_METAL_USE_BF16                  "ggml: use bfloat if available"                   OFF)
option(GGML_METAL_NDEBUG                    "ggml: disable Metal debugging"                   OFF)
option(GGML_METAL_SHADER_DEBUG              "ggml: compile Metal with -fno-fast-math"         OFF)
option(GGML_METAL_EMBED_LIBRARY             "ggml: embed Metal library"                       ${GGML_METAL})
set   (GGML_METAL_MACOSX_VERSION_MIN "" CACHE STRING
                                            "ggml: metal minimum macOS version")
set   (GGML_METAL_STD "" CACHE STRING       "ggml: metal standard version (-std flag)")
option(GGML_OPENMP                          "ggml: use OpenMP"                                ON)
option(GGML_RPC                             "ggml: use RPC"                                   OFF)
option(GGML_SYCL                            "ggml: use SYCL"                                  OFF)
option(GGML_SYCL_F16                        "ggml: use 16 bit floats for sycl calculations"   OFF)
set   (GGML_SYCL_TARGET "INTEL" CACHE STRING
                                            "ggml: sycl target device")
set   (GGML_SYCL_DEVICE_ARCH "" CACHE STRING
                                            "ggml: sycl device architecture")

option(GGML_OPENCL                          "ggml: use OpenCL"                                OFF)
option(GGML_OPENCL_PROFILING                "ggml: use OpenCL profiling (increases overhead)" OFF)
option(GGML_OPENCL_EMBED_KERNELS            "ggml: embed kernels"                             ON)
option(GGML_OPENCL_USE_ADRENO_KERNELS       "ggml: use optimized kernels for Adreno"          ON)

# extra artifacts
option(GGML_BUILD_TESTS    "ggml: build tests"    ${GGML_STANDALONE})
option(GGML_BUILD_EXAMPLES "ggml: build examples" ${GGML_STANDALONE})

#
# dependencies
#

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED true)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
#[[
Note:杨小兵-2025-01-10

1、指令1
    1.1 指定 C 语言的标准版本为 C11。
2、指令2
    2.1 强制要求使用指定的 C 语言标准（C11），如果编译器不支持则会报错。
3、指令3
    3.1 指定 C++ 语言的标准版本为 C++17。
4、指令4
    4.1 强制要求使用指定的 C++ 语言标准（C++17），如果编译器不支持则会报错。
5、指令5
    5.1 在查找多线程库时优先使用 pthread 标志，以确保在支持 pthread 的系统上使用该线程库。
6、指令6
    6.1 在系统上查找并链接线程库（如 pthread），如果找不到则会导致配置失败。
]]

#
# build the library
#

add_subdirectory(src)
#[[
Note:杨小兵-2025-01-10

1、指令1
    1.1 添加子目录：将 src 目录添加到当前的 CMake 构建过程中。add_subdirectory 命令会进入指定的子目录，并处理其中的 CMakeLists.txt 文件，从而包含该子项目的构建配置。这通常用于包含和编译第三方库或模块。
]]
#
# tests and examples
#

if (GGML_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()

if (GGML_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()

#
# install
#

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
#[[
Note:杨小兵-2025-01-10

1、指令1
    1.1 在 CMake 中，include 命令用于包含和执行另一个 CMake 脚本或模块。通过 include，可以引入外部定义的函数、宏、变量和其他设置，从而在当前的 CMakeLists.txt 文件中复用这些内容
        1.1.1 引入模块和功能：CMake 提供了许多内置模块，包含常见的功能检查、编译选项设置等。通过 include，可以使用这些模块提供的功能。
        1.1.2 复用代码： 如果您有多个项目或多个部分需要共享相同的 CMake 代码，可以将这些代码放在一个独立的 CMake 脚本中，然后在需要的地方通过 include 引入。
        1.1.3 组织项目结构： 对于大型项目，可以将不同的构建配置和逻辑分割到多个 CMake 脚本中，通过 include 进行组合，提升可读性和维护性。
    1.2 这里提到的模块指的就是对应的*.cmake文件，本质上就是一个脚本文件，只不过这里的脚本文件是被cmake解析器来进行解析的。
    1.3 示例：引入自定义的 CMake 脚本（include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/custom_settings.cmake)）
    1.4 GNUInstallDirs 模块定义了一组标准的安装目录变量，遵循 GNU 系统的目录结构惯例。这些变量用于指定不同类型的文件（如可执行文件、库、头文件等）的安装位置，从而确保项目的安装路径符合一致的标准，便于用户和系统工具查找和管理安装的组件。
    1.5 CMakePackageConfigHelpers 模块提供了一组辅助函数，用于简化和标准化 CMake 包配置文件（<PackageName>Config.cmake）的创建过程。这些配置文件使得其他项目能够通过 find_package 命令轻松找到和使用该项目的库和目标。
]]

# all public headers
set(GGML_PUBLIC_HEADERS
    include/ggml.h
    include/ggml-cpu.h
    include/ggml-alloc.h
    include/ggml-backend.h
    include/ggml-blas.h
    include/ggml-cann.h
    include/ggml-cuda.h
    include/ggml-kompute.h
    include/ggml-opt.h
    include/ggml-metal.h
    include/ggml-rpc.h
    include/ggml-sycl.h
    include/ggml-vulkan.h)

set_target_properties(ggml PROPERTIES PUBLIC_HEADER "${GGML_PUBLIC_HEADERS}")
#if (GGML_METAL)
#    set_target_properties(ggml PROPERTIES RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/ggml-metal.metal")
#endif()
install(TARGETS ggml LIBRARY PUBLIC_HEADER)
install(TARGETS ggml-base LIBRARY)

# FIXME: this should be done in the backend cmake files
if (GGML_METAL)
    # FIXME: does this need to be installed with GGML_METAL_EMBED_LIBRARY?
    install(
        FILES src/ggml-metal/ggml-metal.metal
        PERMISSIONS
            OWNER_READ
            OWNER_WRITE
            GROUP_READ
            WORLD_READ
        DESTINATION ${CMAKE_INSTALL_BINDIR})

    if (NOT GGML_METAL_EMBED_LIBRARY)
        install(
            FILES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/default.metallib
            DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()
endif()

if (GGML_STANDALONE)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ggml.pc.in
        ${CMAKE_CURRENT_BINARY_DIR}/ggml.pc
        @ONLY)

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ggml.pc
        DESTINATION share/pkgconfig)
endif()
