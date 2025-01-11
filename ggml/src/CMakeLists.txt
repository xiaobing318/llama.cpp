include(CheckCXXCompilerFlag)
#[[
Note:杨小兵-2025-01-10

1、指令1
    1.1 在 CMake 中，include 命令用于包含和执行另一个 CMake 脚本或模块。通过 include，您可以引入外部定义的函数、宏、变量和其他设置，从而在当前的 CMakeLists.txt 文件中复用这些内容
        1.1.1 引入模块和功能：CMake 提供了许多内置模块，包含常见的功能检查、编译选项设置等。通过 include，您可以使用这些模块提供的功能。
        1.1.2 复用代码： 如果您有多个项目或多个部分需要共享相同的 CMake 代码，可以将这些代码放在一个独立的 CMake 脚本中，然后在需要的地方通过 include 引入。
        1.1.3 组织项目结构： 对于大型项目，可以将不同的构建配置和逻辑分割到多个 CMake 脚本中，通过 include 进行组合，提升可读性和维护性。
    1.2 在 CMake 项目中，include(CheckCXXCompilerFlag) 的这条指令用于加载一个名为 CheckCXXCompilerFlag.cmake 的官方 CMake 模块。该模块为我们提供了一个名为 CHECK_CXX_COMPILER_FLAG 的宏，用来检测当前所用的 C++ 编译器（如 gcc、clang、MSVC 等）是否支持某一特定的编译器标志（compiler flag）。
    1.4 当你在 CMakeLists.txt 里使用include(CheckCXXCompilerFlag)之后，你就可以调用如下示例宏来检测一个标志（举例说明）：CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
]]

add_compile_definitions(GGML_SCHED_MAX_COPIES=${GGML_SCHED_MAX_COPIES})
#[[
Note:杨小兵-2025-01-10

1、指令1
    1.1 指令的作用是向编译器提供一个额外的参数。使用 add_compile_definitions(GGML_SCHED_MAX_COPIES) 等价于在编译命令中添加 -DGGML_SCHED_MAX_COPIES（对于 GCC/Clang）或 /DGGML_SCHED_MAX_COPIES（对于 MSVC）
    1.2 这个命令会在项目的所有目标文件中添加对应的预处理器宏定义。如果你只想对某些目标生效，可以使用更细粒度的 target_compile_definitions。
]]

# enable libstdc++ assertions for debug builds
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_compile_definitions($<$<CONFIG:Debug>:_GLIBCXX_ASSERTIONS>)
endif()
#[[
Note:杨小兵-2025-01-10

注释：
    1、说明以下命令用于在调试模式下启用 libstdc++ 的断言功能。
1、指令1
    1.1 判断当前系统名称是否匹配 "Linux"。
2、指令2
    2.1 在 Debug 配置下添加宏定义 _GLIBCXX_ASSERTIONS，以启用断言功能。
3、指令3
    3.1 条件判断的结束标识
]]

if (NOT MSVC)
    if (GGML_SANITIZE_THREAD)
        add_compile_options(-fsanitize=thread)
        link_libraries     (-fsanitize=thread)
    endif()

    if (GGML_SANITIZE_ADDRESS)
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        link_libraries     (-fsanitize=address)
    endif()

    if (GGML_SANITIZE_UNDEFINED)
        add_compile_options(-fsanitize=undefined)
        link_libraries     (-fsanitize=undefined)
    endif()
endif()
#[[
Note:杨小兵-2025-01-10

1、总体解释
    1.1 主要是通过 add_compile_options 和 link_libraries 来为非 MSVC 的编译器（如 gcc 或 clang）添加对应的编译和链接参数，用以启用 ThreadSanitizer、AddressSanitizer、UndefinedBehaviorSanitizer 等功能。
]]

function(ggml_get_flags CCID CCVER)
    set(C_FLAGS "")
    set(CXX_FLAGS "")

    if (CCID MATCHES "Clang")
        set(C_FLAGS   -Wunreachable-code-break -Wunreachable-code-return)
        set(CXX_FLAGS -Wunreachable-code-break -Wunreachable-code-return -Wmissing-prototypes -Wextra-semi)

        if (
            (CCID STREQUAL "Clang"      AND CCVER VERSION_GREATER_EQUAL 3.8.0) OR
            (CCID STREQUAL "AppleClang" AND CCVER VERSION_GREATER_EQUAL 7.3.0)
        )
            list(APPEND C_FLAGS -Wdouble-promotion)
        endif()
    elseif (CCID STREQUAL "GNU")
        set(C_FLAGS   -Wdouble-promotion)
        set(CXX_FLAGS -Wno-array-bounds)

        if (CCVER VERSION_GREATER_EQUAL 8.1.0)
            list(APPEND CXX_FLAGS -Wextra-semi)
        endif()
    endif()

    set(GF_C_FLAGS   ${C_FLAGS}   PARENT_SCOPE)
    set(GF_CXX_FLAGS ${CXX_FLAGS} PARENT_SCOPE)
endfunction()
#[[
Note:杨小兵-2025-01-10

1、总体解释
    1.1 这个 CMake 函数根据给定编译器及其版本（`CCID`、`CCVER`）生成一组编译器特定的警告或检查选项，然后通过 `PARENT_SCOPE` 传递回调用者，从而可以灵活地为 Clang、AppleClang 或 GCC 添加合适的编译与警告标志。
]]

if (GGML_FATAL_WARNINGS)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        list(APPEND C_FLAGS   -Werror)
        list(APPEND CXX_FLAGS -Werror)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        add_compile_options(/WX)
    endif()
endif()
#[[
Note:杨小兵-2025-01-10

1、总体解释
    1.1 如果GGML_FATAL_WARNINGS已经存在的话，那么说明条件判断是成立的，判断cmake内置的对编译器的标识号是否位GNU或者clang来做出不同的设置，针对不同的情况分别添加不同的编译器参数。
]]

if (GGML_ALL_WARNINGS)
    if (NOT MSVC)
        list(APPEND WARNING_FLAGS -Wall -Wextra -Wpedantic -Wcast-qual -Wno-unused-function)
        list(APPEND C_FLAGS       -Wshadow -Wstrict-prototypes -Wpointer-arith -Wmissing-prototypes
                                  -Werror=implicit-int -Werror=implicit-function-declaration)
        list(APPEND CXX_FLAGS     -Wmissing-declarations -Wmissing-noreturn)

        list(APPEND C_FLAGS   ${WARNING_FLAGS})
        list(APPEND CXX_FLAGS ${WARNING_FLAGS})

        ggml_get_flags(${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION})

        add_compile_options("$<$<COMPILE_LANGUAGE:C>:${C_FLAGS};${GF_C_FLAGS}>"
                            "$<$<COMPILE_LANGUAGE:CXX>:${CXX_FLAGS};${GF_CXX_FLAGS}>")
    else()
        # todo : msvc
        set(C_FLAGS   "")
        set(CXX_FLAGS "")
    endif()
endif()
#[[
Note:杨小兵-2025-01-11

1、指令1
    1.1 通过判断变量 GGML_ALL_WARNINGS 是否为真，决定是否应用一系列警告相关的编译选项。
2、指令2
    2.1 如果当前使用的编译器不是微软的Visual C++ (MSVC)，则执行代码块中的内容。
3、总体解释
    3.1 这段CMake代码的主要目的是在启用 GGML_ALL_WARNINGS 时，为非MSVC编译器配置一系列严格的警告选项，以提高代码质量和编译时的错误检测能力。同时，为不同的编译器版本提供了扩展配置的接口（通过 ggml_get_flags），并预留了对MSVC编译器的支持（待后续实现）。
]]

if (GGML_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT result OUTPUT output)
    if (result)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(WARNING "IPO is not supported: ${output}")
    endif()
endif()
#[[
Note:杨小兵-2025-01-11

1、总体解释
    1.1 如果GGML_LTO存在且为真，则执行代码块中的cmake命令，为了设置运行时链接相关内容。
]]

if (GGML_CCACHE)
    find_program(GGML_CCACHE_FOUND ccache)

    if (GGML_CCACHE_FOUND)
        # TODO: should not be set globally
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
        set(ENV{CCACHE_SLOPPINESS} time_macros)
        message(STATUS "ccache found, compilation results will be cached. Disable with GGML_CCACHE=OFF.")
    else()
        message(STATUS "Warning: ccache not found - consider installing it for faster compilation or disable this warning with GGML_CCACHE=OFF")
    endif ()
endif()
#[[
Note:杨小兵-2025-01-11

1、总体解释
    1.1 如果GGML_CCACHE_FOUND存在则执行代码块中的cmake命令，想要设置ccache环境从而来加快编译。
]]

# this version of Apple ld64 is buggy
execute_process(
    COMMAND ${CMAKE_C_COMPILER} ${CMAKE_EXE_LINKER_FLAGS} -Wl,-v
    ERROR_VARIABLE output
    OUTPUT_QUIET
)
#[[
Note:杨小兵-2025-01-11

1、注释1
    1.1 用于说明当前使用的Apple ld64 链接器版本存在问题（buggy）。注释不会被CMake处理，仅供开发者参考。
1、指令1
    1.1 execute_process 是CMake中的一个命令，用于在配置阶段执行外部命令或脚本。它允许CMake在生成构建系统之前执行特定的操作，例如检测工具链、检查库等。
    1.2 执行C编译器，传递链接器标志并请求链接器输出其版本信息。这可能用于检测ld64的版本或验证其行为是否正常。
    1.3 如果执行的命令产生错误信息，这些信息将被存储在变量output中，供后续CMake脚本使用或记录。
    1.4 抑制命令的正常输出，避免在CMake配置过程中产生大量无关的信息，只关注错误输出（通过ERROR_VARIABLE捕获）
]]

if (output MATCHES "dyld-1015\.7")
    add_compile_definitions(HAVE_BUGGY_APPLE_LINKER)
endif()

# architecture specific
# TODO: probably these flags need to be tweaked on some architectures
#       feel free to update the Makefile for your architecture and send a pull request or issue
message(STATUS "CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
if (MSVC)
    string(TOLOWER "${CMAKE_GENERATOR_PLATFORM}" CMAKE_GENERATOR_PLATFORM_LWR)
    message(STATUS "CMAKE_GENERATOR_PLATFORM: ${CMAKE_GENERATOR_PLATFORM}")
else ()
    set(CMAKE_GENERATOR_PLATFORM_LWR "")
endif ()

if (NOT MSVC)
    if (GGML_STATIC)
        add_link_options(-static)
        if (MINGW)
            add_link_options(-static-libgcc -static-libstdc++)
        endif()
    endif()
    if (GGML_GPROF)
        add_compile_options(-pg)
    endif()
endif()
#[[
Note:杨小兵-2025-01-11

1、总体作用
    1.1 如果是非MSVC编译器，则根据不同的选项设置对应的链接参数
]]

if (MINGW)
    # Target Windows 8 for PrefetchVirtualMemory
    add_compile_definitions(_WIN32_WINNT=${GGML_WIN_VER})
endif()
#[[
Note:杨小兵-2025-01-11

1、总体作用
    1.1 如果是MINGW编译环境，则上述指定的编译参数
]]

#
# POSIX conformance
#

# clock_gettime came in POSIX.1b (1993)
# CLOCK_MONOTONIC came in POSIX.1-2001 / SUSv3 as optional
# posix_memalign came in POSIX.1-2001 / SUSv3
# M_PI is an XSI extension since POSIX.1-2001 / SUSv3, came in XPG1 (1985)

# Somehow in OpenBSD whenever POSIX conformance is specified
# some string functions rely on locale_t availability,
# which was introduced in POSIX.1-2008, forcing us to go higher
if (CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
    add_compile_definitions(_XOPEN_SOURCE=700)
else()
    add_compile_definitions(_XOPEN_SOURCE=600)
endif()

# Data types, macros and functions related to controlling CPU affinity and
# some memory allocation are available on Linux through GNU extensions in libc
if (CMAKE_SYSTEM_NAME MATCHES "Linux" OR CMAKE_SYSTEM_NAME MATCHES "Android")
    add_compile_definitions(_GNU_SOURCE)
endif()

# RLIMIT_MEMLOCK came in BSD, is not specified in POSIX.1,
# and on macOS its availability depends on enabling Darwin extensions
# similarly on DragonFly, enabling BSD extensions is necessary
if (
    CMAKE_SYSTEM_NAME MATCHES "Darwin" OR
    CMAKE_SYSTEM_NAME MATCHES "iOS"    OR
    CMAKE_SYSTEM_NAME MATCHES "tvOS"   OR
    CMAKE_SYSTEM_NAME MATCHES "DragonFly"
)
    add_compile_definitions(_DARWIN_C_SOURCE)
endif()

# alloca is a non-standard interface that is not visible on BSDs when
# POSIX conformance is specified, but not all of them provide a clean way
# to enable it in such cases
if (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    add_compile_definitions(__BSD_VISIBLE)
endif()
if (CMAKE_SYSTEM_NAME MATCHES "NetBSD")
    add_compile_definitions(_NETBSD_SOURCE)
endif()
if (CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
    add_compile_definitions(_BSD_SOURCE)
endif()

if (WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

# ggml

if (GGML_BACKEND_DL AND NOT BUILD_SHARED_LIBS)
    message(FATAL_ERROR "GGML_BACKEND_DL requires BUILD_SHARED_LIBS")
endif()

add_library(ggml-base
            ../include/ggml.h
            ../include/ggml-alloc.h
            ../include/ggml-backend.h
            ../include/ggml-cpp.h
            ../include/ggml-opt.h
            ggml.c
            ggml-alloc.c
            ggml-backend.cpp
            ggml-opt.cpp
            ggml-threading.cpp
            ggml-threading.h
            ggml-quants.c
            ggml-quants.h)

target_include_directories(ggml-base PRIVATE .)

add_library(ggml
            ggml-backend-reg.cpp)

target_link_libraries(ggml PUBLIC ggml-base)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_link_libraries(ggml PRIVATE dl)
endif()

function(ggml_add_backend_library backend)
    if (GGML_BACKEND_DL)
        add_library(${backend} MODULE ${ARGN})
        # write the shared library to the output directory
        set_target_properties(${backend} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
        target_compile_definitions(${backend} PRIVATE GGML_BACKEND_DL)
    else()
        add_library(${backend} ${ARGN})
        target_link_libraries(ggml PUBLIC ${backend})
        install(TARGETS ${backend} LIBRARY)
    endif()

    target_link_libraries(${backend} PRIVATE ggml-base)
    target_include_directories(${backend} PRIVATE ..)

    if (${BUILD_SHARED_LIBS})
        target_compile_definitions(${backend} PRIVATE GGML_BACKEND_BUILD)
        target_compile_definitions(${backend} PUBLIC  GGML_BACKEND_SHARED)
    endif()
endfunction()

function(ggml_add_backend backend)
    string(TOUPPER "GGML_${backend}" backend_id)
    if (${backend_id})
        string(TOLOWER "ggml-${backend}" backend_target)
        add_subdirectory(${backend_target})
        message(STATUS "Including ${backend} backend")
        if (NOT GGML_BACKEND_DL)
            string(TOUPPER "GGML_USE_${backend}" backend_use)
            target_compile_definitions(ggml PUBLIC ${backend_use})
        endif()
    endif()
endfunction()

function(ggml_add_cpu_backend_variant tag_name)
    set(GGML_CPU_TAG_NAME ${tag_name})
    # other: OPENMP LLAMAFILE CPU_HBM
    foreach (feat NATIVE
                  AVX AVX2 AVX_VNNI FMA F16C
                  AVX512 AVX512_VBMI AVX512_VNNI AVX512_BF16
                  AMX_TILE AMX_INT8 AMX_BF16)
        set(GGML_${feat} OFF)
    endforeach()

    foreach (feat ${ARGN})
        set(GGML_${feat} ON)
    endforeach()

    ggml_add_cpu_backend_variant_impl(${tag_name})
endfunction()

ggml_add_backend(CPU)

if (GGML_CPU_ALL_VARIANTS)
    if (NOT GGML_BACKEND_DL)
        message(FATAL_ERROR "GGML_CPU_ALL_VARIANTS requires GGML_BACKEND_DL")
    endif()
    ggml_add_cpu_backend_variant(sandybridge    AVX)
    ggml_add_cpu_backend_variant(haswell        AVX F16C AVX2 FMA)
    ggml_add_cpu_backend_variant(skylakex       AVX F16C AVX2 FMA AVX512)
    ggml_add_cpu_backend_variant(icelake        AVX F16C AVX2 FMA AVX512 AVX512_VBMI AVX512_VNNI)
    if (NOT MSVC)
        # MSVC doesn't support AVX-VNNI or AMX
        ggml_add_cpu_backend_variant(alderlake      AVX F16C AVX2 FMA AVX_VNNI)
        ggml_add_cpu_backend_variant(sapphirerapids AVX F16C AVX2 FMA AVX512 AVX512_VBMI AVX512_VNNI AVX512_BF16 AMX_TILE AMX_INT8)
    endif()
else ()
    ggml_add_cpu_backend_variant_impl("")
endif()

ggml_add_backend(BLAS)
ggml_add_backend(CANN)
ggml_add_backend(CUDA)
ggml_add_backend(HIP)
ggml_add_backend(Kompute)
ggml_add_backend(METAL)
ggml_add_backend(MUSA)
ggml_add_backend(RPC)
ggml_add_backend(SYCL)
ggml_add_backend(Vulkan)
ggml_add_backend(OpenCL)

foreach (target ggml-base ggml)
    target_include_directories(${target} PUBLIC    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include> $<INSTALL_INTERFACE:include>)
    target_compile_features   (${target} PRIVATE c_std_11 cxx_std_17) # don't bump
endforeach()

target_link_libraries(ggml-base PRIVATE Threads::Threads)

find_library(MATH_LIBRARY m)
if (MATH_LIBRARY)
    if (NOT WIN32 OR NOT DEFINED ENV{ONEAPI_ROOT})
        target_link_libraries(ggml-base PRIVATE m)
    endif()
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Android")
    target_link_libraries(ggml-base PRIVATE dl)
endif()

if (BUILD_SHARED_LIBS)
    foreach (target ggml-base ggml)
        set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE ON)
        target_compile_definitions(${target} PRIVATE GGML_BUILD)
        target_compile_definitions(${target} PUBLIC  GGML_SHARED)
    endforeach()
endif()
