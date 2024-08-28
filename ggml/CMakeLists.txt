#[[
1. `cmake_minimum_required(VERSION 3.14)`
   - 这一行指定了当前CMake项目所需的最低CMake版本。在这个例子中，它要求至少使用CMake版本3.14。这是因为一些特定的功能或者选项可能在旧版本的CMake中
   不可用（CMake版本至少为3.14才能提供当前项目所需要的特定功能）。在这里提到的3.14版本支持`add_link_options`和隐式目标目录等功能。

2. `project("ggml" C CXX)`
   - 这行命令定义了项目的名称以及项目支持的编程语言。项目名称被设置为"ggml"，同时指定了这个项目使用C和C++语言。`C`代表C语言，`CXX`代表C++语言。
   项目名称通常用于生成相关文档、导出目标等多种用途。

3. `include(CheckIncludeFileCXX)`
   - `include()`命令用于载入并运行CMake代码。这里，它包含了`CheckIncludeFileCXX`模块，该模块提供了检查C++头文件是否存在的功能。这对于确认编译环境
是否具备某些功能或者库的支持非常有用。

4. "for add_link_options and implicit target directories."内容解释
   在CMake中，`cmake_minimum_required(VERSION 3.14)`这行代码不仅指定了项目需要的最低CMake版本，而且它还确保了使用该版本所支持的所有功能。其中提到
的“# for add_link_options and implicit target directories”是注释，用来解释为什么需要至少3.14版本的CMake。

   1. **add_link_options**: 这是一个在CMake 3.13版本中引入的命令，它允许开发者在全局范围内为链接器添加选项。这意味着，通过这个命令，你可以为项目中的
   所有目标设置链接选项，而不需要对每一个目标分别设置。这大大简化了链接选项的管理，尤其是在大型项目中。

   2. **隐式目标目录**：从CMake 3.14开始，增加了更智能的目标二进制文件目录的处理。在这之前，你可能需要显式地设置每个目标的存放目录。从3.14版本开始，
   CMake能更智能地推断这些目录，使得目录结构的管理更为方便和直观。

通过要求至少使用CMake 3.14，这个CMake脚本确保了所有这些较新和改进的功能都可用，从而使构建过程更加高效和易于管理。
]]
cmake_minimum_required(VERSION 3.14) # for add_link_options and implicit target directories.
project("ggml" C CXX)
include(CheckIncludeFileCXX)







#[[
### 命令解释
1. `set(CMAKE_EXPORT_COMPILE_COMMANDS ON)`
   - 这行命令设置 `CMAKE_EXPORT_COMPILE_COMMANDS` 变量为 `ON`。开启此选项后，CMake 会生成一个 `compile_commands.json` 文件，这个文件包含了
   所有编译单元的具体编译命令。这对于使用某些工具进行代码分析、自动完成或重构非常有用，如 Clang 工具。

2. `if (NOT XCODE AND NOT MSVC AND NOT CMAKE_BUILD_TYPE)`
   - 这是一个条件判断语句。`if` 语句检查是否不是在 Xcode 或 MS Visual Studio 环境中编译，并且没有设置 `CMAKE_BUILD_TYPE` 变量。如果所有这些条件
   都满足（即不在 Xcode 或 MSVC 中，且 `CMAKE_BUILD_TYPE` 没有预先定义），则执行后续的 `set` 命令。

3. `set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)`
   - 这行命令在上述条件为真时执行。它将 `CMAKE_BUILD_TYPE` 变量设置为 "Release"。此外，这个设置使用 `CACHE` 选项，意味着变量值被缓存起来，可以在
   不同的 CMake 运行之间保持。`STRING` 表明这是一个字符串变量，`"Build type"` 是这个缓存条目的描述。`FORCE` 表示即使变量已经在缓存中存在，也要强制
   更新它的值。

4. `set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")`
   - 这行命令为 `CMAKE_BUILD_TYPE` 缓存变量设置了属性。具体地，它定义了可能的字符串值：`"Debug"`, `"Release"`, `"MinSizeRel"` 和 
   `"RelWithDebInfo"`。这有助于在基于图形的配置工具中为用户提供可选的构建类型选项。

### 是否为 CMake 内置变量

- `CMAKE_BUILD_TYPE`: 是 CMake 的内置变量。它用于指定要使用的构建类型，通常影响编译器的优化级别和是否包含调试信息。
- `XCODE` 和 `MSVC`: 这些也是 CMake 内置变量，用于检测当前的生成环境是否为 Xcode 或 Microsoft Visual Studio。

这些内置变量使得 CMake 脚本能够根据不同的编译环境做出调整，确保在多种平台和编译器上的兼容性和最优配置。

]]
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if (NOT XCODE AND NOT MSVC AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()





#[[
### CMake 命令解释

1. `if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)`
   - 这行命令检查 `CMAKE_SOURCE_DIR` 和 `CMAKE_CURRENT_SOURCE_DIR` 是否相等。`CMAKE_SOURCE_DIR` 表示项目的顶级源代码目录，而 
   `CMAKE_CURRENT_SOURCE_DIR` 指的是当前处理的 CMakeLists.txt 文件所在的目录。如果这两者相同，说明当前CMakeLists.txt是在顶级目录，
   可能表示这个项目是作为独立项目被配置的，而不是作为某个其他项目的一部分。

2. `set(LLAMA_STANDALONE ON)`
   - 在条件为真的情况下执行，即当这个项目作为独立项目构建时。它设置一个自定义变量 `LLAMA_STANDALONE` 为 `ON`，可能用于在其他地方控制项目的配置
   或编译选项。

3. `include(git-vars)`
   - 这行命令包含了一个名为 `git-vars` 的CMake脚本。这个脚本可能定义了与git相关的变量或设置，比如从git仓库中获取版本信息等。

4. `# configure project version`
   - 这是一行注释，提示还有待完成的任务：配置项目版本。这可能意味着将来要添加代码来根据一些标准设置项目的版本号。

5. `else()`
   - 这部分是 `if` 语句的一部分，它指定了当 `if` 的条件不满足时执行的代码块。

6. `set(LLAMA_STANDALONE OFF)`
   - 在 `if` 条件不满足时执行，即项目不是作为独立项目构建的情况下，设置 `LLAMA_STANDALONE` 为 `OFF`。

7. `endif()`
   - 结束 `if` 语句块。

### 是否为 CMake 内置变量?

- `CMAKE_SOURCE_DIR` 和 `CMAKE_CURRENT_SOURCE_DIR` 都是 CMake 的内置变量。这些变量由CMake自动设置，用于指示源代码的相关路径，非常有用
于控制项目的结构和编译行为。
- `LLAMA_STANDALONE` 不是一个内置变量，而是这个特定CMake项目中定义的一个自定义变量，用于标识项目的独立性。
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
### 解释各行命令

1. `if (EMSCRIPTEN)`
   - 这行命令检查是否在使用Emscripten编译环境。Emscripten是一个将C/C++编译为WebAssembly的工具链，常用于将本地应用程序和游戏移植到网页上。

2. `set(BUILD_SHARED_LIBS_DEFAULT OFF)`
   - 在Emscripten环境下执行，将`BUILD_SHARED_LIBS_DEFAULT`变量设置为`OFF`。这通常意味着在这个环境下默认不生成共享库（动态库），而是生成静态库，
   因为WebAssembly环境通常不支持动态链接。

3. `option(LLAMA_WASM_SINGLE_FILE "llama: embed WASM inside the generated llama.js" ON)`
   - 这个命令在Emscripten环境中定义了一个选项`LLAMA_WASM_SINGLE_FILE`，默认值为`ON`。这个选项用于决定是否将生成的WebAssembly代码嵌入到一个单一
   的JavaScript文件中，方便在网页中加载和执行。

4. `else()`
   - 如果不是在Emscripten环境下，执行后续的条件分支。

5. `if (MINGW)`
   - 这行检查是否在使用MinGW编译环境。MinGW是一个允许在Windows操作系统上使用GNU工具链的环境，常用于生成与Windows平台兼容的应用程序。

6. `set(BUILD_SHARED_LIBS_DEFAULT OFF)`
   - 在MinGW环境下执行，设置`BUILD_SHARED_LIBS_DEFAULT`为`OFF`，指示在这个环境下默认也不生成共享库。

7. `else()`
   - 如果既不是Emscripten环境也不是MinGW环境，则执行这个分支。

8. `set(BUILD_SHARED_LIBS_DEFAULT ON)`
   - 在其他环境下，默认设置为生成共享库（动态库），这是大多数环境支持和常见的做法。

9. `endif()`
   - 结束内层的`if`（关于MINGW的检查）。

10. `endif()`
    - 结束最外层的`if`（关于EMSCRIPTEN的检查）。

### 总结

    这段CMake代码根据编译环境的不同设置库的构建类型。在特定的编译环境（如Emscripten和MinGW）中，默认禁用共享库的构建，而在其他环境中则默认启用。
    这有助于优化各环境下的编译和运行行为，确保兼容性和性能。
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
这行CMake命令使用了`option`命令来定义一个可配置的布尔选项，具体来说：

### 命令解释

- `option(BUILD_SHARED_LIBS "build shared libraries" ${BUILD_SHARED_LIBS_DEFAULT})`
  - `option`命令用于定义一个用户可通过CMake界面（如cmake-gui或ccmake）设置的布尔值选项。
  - `BUILD_SHARED_LIBS` 是选项的变量名，该变量控制项目是否构建共享库（动态库）。
  - `"build shared libraries"` 是该选项的描述文字，通常在图形界面中显示给用户看。
  - `${BUILD_SHARED_LIBS_DEFAULT}` 是这个选项的默认值。这个值是从另一个变量 `BUILD_SHARED_LIBS_DEFAULT` 中获取的，该变量的值根据前文的条件
  设置（如是否在使用Emscripten或MinGW）会有所不同。

### 用途和影响

使用`option`命令允许用户在配置项目时选择是否希望构建共享库，这对于项目管理和最终用户来说提供了灵活性。例如：

- **开发共享库**：当项目需要提供API或功能给其他项目使用时，生成共享库是常见的选择。
- **静态链接**：在某些环境下，如嵌入式系统或特定的发布需求，可能更倾向于使用静态库以避免运行时依赖。

]]
option(BUILD_SHARED_LIBS "ggml: build shared libraries" ${BUILD_SHARED_LIBS_DEFAULT})
















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

if (CMAKE_CROSSCOMPILING)
    set(GGML_NATIVE_DEFAULT OFF)
else()
    set(GGML_NATIVE_DEFAULT ON)
endif()

# general
option(GGML_STATIC "ggml: static link libraries"         OFF)
option(GGML_NATIVE "ggml: enable -march=native flag"     ${GGML_NATIVE_DEFAULT})
option(GGML_LTO    "ggml: enable link time optimization" OFF)
option(GGML_CCACHE "ggml: use ccache if available"       ON)

# debug
option(GGML_ALL_WARNINGS           "ggml: enable all compiler warnings"                   ON)
option(GGML_ALL_WARNINGS_3RD_PARTY "ggml: enable all compiler warnings in 3rd party libs" OFF)
option(GGML_GPROF                  "ggml: enable gprof"                                   OFF)

# build
option(GGML_FATAL_WARNINGS    "ggml: enable -Werror flag"    OFF)

# sanitizers
option(GGML_SANITIZE_THREAD    "ggml: enable thread sanitizer"    OFF)
option(GGML_SANITIZE_ADDRESS   "ggml: enable address sanitizer"   OFF)
option(GGML_SANITIZE_UNDEFINED "ggml: enable undefined sanitizer" OFF)

# instruction set specific
if (GGML_NATIVE OR NOT GGML_NATIVE_DEFAULT)
    set(INS_ENB OFF)
else()
    set(INS_ENB ON)
endif()

option(GGML_CPU_HBM     "ggml: use memkind for CPU HBM" OFF)

option(GGML_AVX         "ggml: enable AVX"              ${INS_ENB})
option(GGML_AVX2        "ggml: enable AVX2"             ${INS_ENB})
option(GGML_AVX512      "ggml: enable AVX512"           OFF)
option(GGML_AVX512_VBMI "ggml: enable AVX512-VBMI"      OFF)
option(GGML_AVX512_VNNI "ggml: enable AVX512-VNNI"      OFF)
option(GGML_AVX512_BF16 "ggml: enable AVX512-BF16"      OFF)
option(GGML_FMA         "ggml: enable FMA"              ${INS_ENB})
if (NOT MSVC)
    option(GGML_F16C    "ggml: enable F16C"             ${INS_ENB}) # in MSVC F16C is implied with AVX2/AVX512
endif()
option(GGML_LASX        "ggml: enable lasx"             ON)
option(GGML_LSX         "ggml: enable lsx"              ON)
option(GGML_SVE         "ggml: enable SVE"              OFF)

if (WIN32)
    set(GGML_WIN_VER "0x602" CACHE STRING "ggml: Windows Version")
endif()

# ggml core
set(GGML_SCHED_MAX_COPIES  "4" CACHE STRING "ggml: max input copies for pipeline parallelism")

# 3rd party libs / backends
option(GGML_ACCELERATE                      "ggml: enable Accelerate framework"               ON)
option(GGML_BLAS                            "ggml: use BLAS"                                  ${GGML_BLAS_DEFAULT})
set(GGML_BLAS_VENDOR ${GGML_BLAS_VENDOR_DEFAULT} CACHE STRING
                                            "ggml: BLAS library vendor")
option(GGML_LLAMAFILE                       "ggml: use LLAMAFILE"                             OFF)

option(GGML_CUDA                            "ggml: use CUDA"                                  OFF)
option(GGML_MUSA                            "ggml: use MUSA"                                  OFF)
option(GGML_CUDA_FORCE_DMMV                 "ggml: use dmmv instead of mmvq CUDA kernels"     OFF)
option(GGML_CUDA_FORCE_MMQ                  "ggml: use mmq kernels instead of cuBLAS"         OFF)
option(GGML_CUDA_FORCE_CUBLAS               "ggml: always use cuBLAS instead of mmq kernels"  OFF)
set   (GGML_CUDA_DMMV_X   "32" CACHE STRING "ggml: x stride for dmmv CUDA kernels")
set   (GGML_CUDA_MMV_Y     "1" CACHE STRING "ggml: y block size for mmv CUDA kernels")
option(GGML_CUDA_F16                        "ggml: use 16 bit floats for some calculations"   OFF)
set   (GGML_CUDA_KQUANTS_ITER "2" CACHE STRING
                                            "ggml: iters./thread per block for Q2_K/Q6_K")
set   (GGML_CUDA_PEER_MAX_BATCH_SIZE "128" CACHE STRING
                                            "ggml: max. batch size for using peer access")
option(GGML_CUDA_NO_PEER_COPY               "ggml: do not use peer to peer copies"            OFF)
option(GGML_CUDA_NO_VMM                     "ggml: do not try to use CUDA VMM"                OFF)
option(GGML_CUDA_FA_ALL_QUANTS              "ggml: compile all quants for FlashAttention"     OFF)
option(GGML_CUDA_USE_GRAPHS                 "ggml: use CUDA graphs (llama.cpp only)"          OFF)

option(GGML_HIPBLAS                         "ggml: use hipBLAS"                               OFF)
option(GGML_HIP_UMA                         "ggml: use HIP unified memory architecture"       OFF)
option(GGML_VULKAN                          "ggml: use Vulkan"                                OFF)
option(GGML_VULKAN_CHECK_RESULTS            "ggml: run Vulkan op checks"                      OFF)
option(GGML_VULKAN_DEBUG                    "ggml: enable Vulkan debug output"                OFF)
option(GGML_VULKAN_MEMORY_DEBUG             "ggml: enable Vulkan memory debug output"         OFF)
option(GGML_VULKAN_PERF                     "ggml: enable Vulkan perf output"                 OFF)
option(GGML_VULKAN_VALIDATE                 "ggml: enable Vulkan validation"                  OFF)
option(GGML_VULKAN_RUN_TESTS                "ggml: run Vulkan tests"                          OFF)
option(GGML_KOMPUTE                         "ggml: use Kompute"                               OFF)
option(GGML_METAL                           "ggml: use Metal"                                 ${GGML_METAL_DEFAULT})
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

# extra artifacts
option(GGML_BUILD_TESTS    "ggml: build tests"    ${GGML_STANDALONE})
option(GGML_BUILD_EXAMPLES "ggml: build examples" ${GGML_STANDALONE})

#
# dependencies
#

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED true)

if (GGML_SYCL)
    set(CMAKE_CXX_STANDARD 17)
else()
    set(CMAKE_CXX_STANDARD 11)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED true)

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)

















#
# build the library
#

add_subdirectory(src)

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

# all public headers
set(GGML_PUBLIC_HEADERS
    include/ggml.h
    include/ggml-alloc.h
    include/ggml-backend.h
    include/ggml-blas.h
    include/ggml-cann.h
    include/ggml-cuda.h
    include/ggml.h
    include/ggml-kompute.h
    include/ggml-metal.h
    include/ggml-rpc.h
    include/ggml-sycl.h
    include/ggml-vulkan.h)

set_target_properties(ggml PROPERTIES PUBLIC_HEADER "${GGML_PUBLIC_HEADERS}")
#if (GGML_METAL)
#    set_target_properties(ggml PROPERTIES RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/ggml-metal.metal")
#endif()
install(TARGETS ggml PUBLIC_HEADER)

if (BUILD_SHARED_LIBS)
    install(TARGETS ggml LIBRARY)
endif()

if (GGML_METAL)
    install(
        FILES src/ggml-metal.metal
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
