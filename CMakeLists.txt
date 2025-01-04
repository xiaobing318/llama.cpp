cmake_minimum_required(VERSION 3.14) # for add_link_options and implicit target directories.
project("llama.cpp" C CXX)
include(CheckIncludeFileCXX)
#[[
Note:杨小兵-2025-01-04

1、指令1
    1.1 指定 CMake 的最低版本要求为 3.14，这是为了确保支持 `add_link_options` 和隐式目标目录等功能。
    1.2 在当前项目的构建过程中使用到了cmake高版本中的功能，但是在cmake低版本中没有对应的功能，因此需要指定cmake的版本
2、指令2
    2.1 定义项目名称为 "llama.cpp"，并指定支持的编程语言为 C 和 C++。
    2.2 llama.cpp是项目的名称，C、CXX则是指定该项目中使用的programming languages为C、C++
3、指令3
    3.1 在 CMake 中，include 命令用于包含和执行另一个 CMake 脚本或模块。通过 include，您可以引入外部定义的函数、宏、变量和其他设置，从而在当前的 CMakeLists.txt 文件中复用这些内容
        3.1.1 引入模块和功能：CMake 提供了许多内置模块，包含常见的功能检查、编译选项设置等。通过 include，您可以使用这些模块提供的功能。
        3.1.2 复用代码： 如果您有多个项目或多个部分需要共享相同的 CMake 代码，可以将这些代码放在一个独立的 CMake 脚本中，然后在需要的地方通过 include 引入。
        3.1.3 组织项目结构： 对于大型项目，可以将不同的构建配置和逻辑分割到多个 CMake 脚本中，通过 include 进行组合，提升可读性和维护性。
    3.2 这里提到的模块指的就是对应的*.cmake文件，本质上就是一个脚本文件，只不过这里的脚本文件是被cmake解析器来进行解析的。
    3.3 示例：引入自定义的 CMake 脚本（include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/custom_settings.cmake)）
    3.4 CheckIncludeFileCXX.cmake该模块用于检查系统中是否存在特定的 C++ 头文件。它通过尝试编译包含该头文件的简单测试程序来确定头文件的可用性。
]]

#set(CMAKE_WARN_DEPRECATED YES)
set(CMAKE_WARN_UNUSED_CLI YES)
#[[
Note:杨小兵-2025-01-04

1、指令1
    1.1 这行cmake命令被注释掉了，说明已经被弃用了
    1.2 创建一个新的cmake变量将其设置为CMAKE_WARN_DEPRECATED
2、指令2
    2.1 在 CMake 中，set 命令用于定义或修改变量的值。这些变量可以在整个 CMake 配置过程中被引用，以控制构建过程的各种方面。
]]

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#[[
Note:杨小兵-2025-01-04

1、指令1
    1.1 在 CMake 中，set 命令用于定义或修改变量的值。这些变量可以在整个 CMake 配置过程中被引用，以控制构建过程的各种方面。
    1.2 在在 CMake 中，CMAKE_EXPORT_COMPILE_COMMANDS 是一个用于控制是否生成 compile_commands.json 文件的变量。通过设置 CMAKE_EXPORT_COMPILE_COMMANDS 为 ON，CMake 会在构建目录中生成一个 compile_commands.json 文件。这个文件详细记录了项目中每个源文件的编译命令和编译选项。
    1.3 set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 的主要作用是让 CMake 生成一个包含所有源文件编译命令的 compile_commands.json 文件。这对于静态分析工具、代码编辑器的智能功能以及其他依赖编译信息的工具非常有用。通过启用这一选项，可以显著提升开发过程中的代码质量检查和开发效率。
    1.4 Visual Studio 生成器不支持 compile_commands.json：CMake 的 CMAKE_EXPORT_COMPILE_COMMANDS 选项在大多数 Unix-like 生成器（如 Ninja 和 Unix Makefiles）中是支持的，但对于 Visual Studio 生成器，它并不生成 compile_commands.json 文件。
]]

if (NOT XCODE AND NOT MSVC AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()
#[[
Note:杨小兵-2025-01-04

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


# Add path to modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
#[[
Note:杨小兵-2025-01-04

1、指令1
    1.1 上述cmake指令的作用是将 ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ 路径添加到 CMake 的模块搜索路径中，以便在配置和生成过程中，CMake 能够在这个路径下找到自定义的模块文件（通常是 .cmake 文件）。
]]

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
#[[
Note:杨小兵-2025-01-04

1、指令1
    将项目构建的结果设置成对应目录下的bin文件夹中
]]


if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(LLAMA_STANDALONE ON)

    include(git-vars)

    # configure project version
    # TODO
else()
    set(LLAMA_STANDALONE OFF)
endif()
#[[
Note:杨小兵-2025-01-04

1、指令1
    1.1 判断当前目录是否是项目的顶层目录，若是，则启用独立构建模式（LLAMA_STANDALONE），并加载 git-vars 配置；否则禁用独立模式（LLAMA_STANDALONE）
    1.2 独立构建模式：开发独立工具或库时，用于单独测试、发布或调试。
    1.3 非独立构建模式：项目被集成到更大系统中，如作为一个子模块或插件时，采用这种模式简化管理。
]]

if (EMSCRIPTEN)
    set(BUILD_SHARED_LIBS_DEFAULT OFF)

    option(LLAMA_WASM_SINGLE_FILE "llama: embed WASM inside the generated llama.js" ON)
else()
    if (MINGW)
        set(BUILD_SHARED_LIBS_DEFAULT OFF)
    else()
        set(BUILD_SHARED_LIBS_DEFAULT ON)
    endif()
endif()
#[[
Note:杨小兵-2025-01-04

1、总体解释
    这段命令主要根据不同的构建环境设置共享库（shared libraries）的默认构建选项，并为特定环境（如 EMSCRIPTEN）启用额外选项：
2、判断是否使用 EMSCRIPTEN 工具链：
    2.1 如果 EMSCRIPTEN 被定义：
        2.1.1 禁用共享库的默认构建（BUILD_SHARED_LIBS_DEFAULT OFF）。
        2.1.2 定义一个选项 LLAMA_WASM_SINGLE_FILE，用于控制是否将 WebAssembly 文件嵌入生成的 JavaScript 文件中，默认值为 ON。
    2.2 非 EMSCRIPTEN 环境：
        2.2.1 如果当前环境是 MinGW（Windows 的 GCC 工具链）：
            2.2.1.1 禁用共享库的默认构建（BUILD_SHARED_LIBS_DEFAULT OFF）。
        2.2.2 否则（其他环境，例如 Linux 或 macOS）：
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

option(BUILD_SHARED_LIBS "build shared libraries" ${BUILD_SHARED_LIBS_DEFAULT})

if (WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

if (MSVC)
    add_compile_options("$<$<COMPILE_LANGUAGE:C>:/utf-8>")
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/utf-8>")
endif()

#
# option list
#

# debug
option(LLAMA_ALL_WARNINGS           "llama: enable all compiler warnings"                   ON)
option(LLAMA_ALL_WARNINGS_3RD_PARTY "llama: enable all compiler warnings in 3rd party libs" OFF)

# build
option(LLAMA_FATAL_WARNINGS "llama: enable -Werror flag" OFF)

# sanitizers
option(LLAMA_SANITIZE_THREAD    "llama: enable thread sanitizer"    OFF)
option(LLAMA_SANITIZE_ADDRESS   "llama: enable address sanitizer"   OFF)
option(LLAMA_SANITIZE_UNDEFINED "llama: enable undefined sanitizer" OFF)

# utils
option(LLAMA_BUILD_COMMON "llama: build common utils library" ${LLAMA_STANDALONE})

# extra artifacts
option(LLAMA_BUILD_TESTS    "llama: build tests"          ${LLAMA_STANDALONE})
option(LLAMA_BUILD_EXAMPLES "llama: build examples"       ${LLAMA_STANDALONE})
option(LLAMA_BUILD_SERVER   "llama: build server example" ${LLAMA_STANDALONE})

# 3rd party libs
option(LLAMA_CURL "llama: use libcurl to download model from an URL" OFF)

# Required for relocatable CMake package
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/build-info.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/common.cmake)

# override ggml options
set(GGML_SANITIZE_THREAD    ${LLAMA_SANITIZE_THREAD})
set(GGML_SANITIZE_ADDRESS   ${LLAMA_SANITIZE_ADDRESS})
set(GGML_SANITIZE_UNDEFINED ${LLAMA_SANITIZE_UNDEFINED})
set(GGML_ALL_WARNINGS       ${LLAMA_ALL_WARNINGS})
set(GGML_FATAL_WARNINGS     ${LLAMA_FATAL_WARNINGS})

# change the default for these ggml options
if (NOT DEFINED GGML_LLAMAFILE)
    set(GGML_LLAMAFILE_DEFAULT ON)
endif()

if (NOT DEFINED GGML_CUDA_GRAPHS)
    set(GGML_CUDA_GRAPHS_DEFAULT ON)
endif()

# transition helpers
function (llama_option_depr TYPE OLD NEW)
    if (${OLD})
        message(${TYPE} "${OLD} is deprecated and will be removed in the future.\nUse ${NEW} instead\n")
        set(${NEW} ON PARENT_SCOPE)
    endif()
endfunction()

llama_option_depr(FATAL_ERROR LLAMA_CUBLAS              GGML_CUDA)
llama_option_depr(WARNING     LLAMA_CUDA                GGML_CUDA)
llama_option_depr(WARNING     LLAMA_KOMPUTE             GGML_KOMPUTE)
llama_option_depr(WARNING     LLAMA_METAL               GGML_METAL)
llama_option_depr(WARNING     LLAMA_METAL_EMBED_LIBRARY GGML_METAL_EMBED_LIBRARY)
llama_option_depr(WARNING     LLAMA_NATIVE              GGML_NATIVE)
llama_option_depr(WARNING     LLAMA_RPC                 GGML_RPC)
llama_option_depr(WARNING     LLAMA_SYCL                GGML_SYCL)
llama_option_depr(WARNING     LLAMA_SYCL_F16            GGML_SYCL_F16)
llama_option_depr(WARNING     LLAMA_CANN                GGML_CANN)

#
# build the library
#

if (NOT TARGET ggml)
    add_subdirectory(ggml)
    # ... otherwise assume ggml is added by a parent CMakeLists.txt
endif()
add_subdirectory(src)

#
# install
#

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(LLAMA_BUILD_NUMBER        ${BUILD_NUMBER})
set(LLAMA_BUILD_COMMIT        ${BUILD_COMMIT})
set(LLAMA_INSTALL_VERSION 0.0.${BUILD_NUMBER})

set(LLAMA_INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR} CACHE PATH "Location of header  files")
set(LLAMA_LIB_INSTALL_DIR     ${CMAKE_INSTALL_LIBDIR}     CACHE PATH "Location of library files")
set(LLAMA_BIN_INSTALL_DIR     ${CMAKE_INSTALL_BINDIR}     CACHE PATH "Location of binary  files")

# At the moment some compile definitions are placed within the ggml/src
# directory but not exported on the `ggml` target. This could be improved by
# determining _precisely_ which defines are necessary for the llama-config
# package.
#
set(GGML_TRANSIENT_DEFINES)
get_target_property(GGML_DIRECTORY ggml SOURCE_DIR)
get_directory_property(GGML_DIR_DEFINES DIRECTORY ${GGML_DIRECTORY} COMPILE_DEFINITIONS)
if (GGML_DIR_DEFINES)
    list(APPEND GGML_TRANSIENT_DEFINES ${GGML_DIR_DEFINES})
endif()
get_target_property(GGML_TARGET_DEFINES ggml COMPILE_DEFINITIONS)
if (GGML_TARGET_DEFINES)
    list(APPEND GGML_TRANSIENT_DEFINES ${GGML_TARGET_DEFINES})
endif()
get_target_property(GGML_LINK_LIBRARIES ggml LINK_LIBRARIES)
# all public headers
set(LLAMA_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/llama.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/llama-cpp.h)
set_target_properties(llama PROPERTIES PUBLIC_HEADER "${LLAMA_PUBLIC_HEADERS}")
install(TARGETS llama LIBRARY PUBLIC_HEADER)

configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/llama-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/llama-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/llama
    PATH_VARS LLAMA_INCLUDE_INSTALL_DIR
              LLAMA_LIB_INSTALL_DIR
              LLAMA_BIN_INSTALL_DIR )

write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/llama-version.cmake
    VERSION ${LLAMA_INSTALL_VERSION}
    COMPATIBILITY SameMajorVersion)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/llama-config.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/llama-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/llama)

install(
    FILES convert_hf_to_gguf.py
    PERMISSIONS
        OWNER_READ
        OWNER_WRITE
        OWNER_EXECUTE
        GROUP_READ
        GROUP_EXECUTE
        WORLD_READ
        WORLD_EXECUTE
    DESTINATION ${CMAKE_INSTALL_BINDIR})

configure_file(cmake/llama.pc.in
        "${CMAKE_CURRENT_BINARY_DIR}/llama.pc"
        @ONLY)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/llama.pc"
        DESTINATION lib/pkgconfig)

#
# utils, programs, examples and tests
#

if (LLAMA_BUILD_COMMON)
    add_subdirectory(common)
endif()

if (LLAMA_BUILD_COMMON AND LLAMA_BUILD_TESTS AND NOT CMAKE_JS_VERSION)
    include(CTest)
    add_subdirectory(tests)
endif()

if (LLAMA_BUILD_COMMON AND LLAMA_BUILD_EXAMPLES)
    add_subdirectory(examples)
    add_subdirectory(pocs)
endif()
