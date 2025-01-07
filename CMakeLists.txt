cmake_minimum_required(VERSION 3.14) # for add_link_options and implicit target directories.
project("llama.cpp" C CXX)
include(CheckIncludeFileCXX)
#[[
Note:杨小兵-2025-01-04

1、指令1
    1.1 作用：指定 CMake 的最低版本要求为 3.14，对于CMake版本大于等于3.14的都是符合要求的
    1.2 注释解释：支持两个features
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

#set(CMAKE_WARN_DEPRECATED YES)
set(CMAKE_WARN_UNUSED_CLI YES)
#[[
Note:杨小兵-2025-01-04

1、指令1
    1.1 这行cmake命令被注释掉了，说明已经被弃用了
    1.2 CMAKE作为前缀的变量是cmake内置的变量，这里将变量的值设置为YES（是一个boolean类型的变量）
2、指令2
    2.1 在 CMake 中，set 命令用于定义或修改变量的值。这些变量可以在整个 CMake 配置过程中被引用，以控制构建过程的各种方面。
]]

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#[[
Note:杨小兵-2025-01-04

1、指令1
    1.1 在 CMake 中，set 命令用于定义或修改变量的值。这些变量可以在整个 CMake 配置过程中被引用，以控制构建过程的各种方面。
    1.2 在 CMake 中，CMAKE_EXPORT_COMPILE_COMMANDS 是一个用于控制是否生成 compile_commands.json 文件的变量。通过设置 CMAKE_EXPORT_COMPILE_COMMANDS 为 ON，CMake 会在构建目录中生成一个 compile_commands.json 文件。这个文件详细记录了项目中每个源文件的编译命令和编译选项。
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
Note:杨小兵-2025-01-07

1、指令1
    1.1 上述cmake指令的作用是将 ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ 路径添加到 CMake 的模块搜索路径中，以便在配置和生成过程中，CMake 能够在这个路径下找到自定义的模块文件（通常是 .cmake 文件）。
    1.2 list 命令是 CMake 中用于操作列表（List）变量的内置命令。CMake 的列表实际上是由分号（;）分隔的字符串，list 命令提供了一系列子命令，用于对这些列表进行添加、删除、修改、查询等操作。通过 list 命令，开发者可以方便地管理和操作多个值的集合，例如源文件列表、包含目录列表、库路径列表等。
    1.3 主要功能包括：
        1.3.1 添加元素：向列表中追加或插入新的元素。
        1.3.2 删除元素：从列表中移除指定的元素。
        1.3.3 修改元素：替换列表中的某个元素。
        1.3.4 查询列表：检查列表中是否包含某个元素，获取列表的长度等。
        1.3.5 排序和去重：对列表进行排序或移除重复元素。
    1.4 常用子命令示例：
        1.4.1 APPEND：向列表末尾添加一个或多个元素。
        1.4.2 INSERT：在指定位置插入一个或多个元素。
        1.4.3 REMOVE_ITEM：从列表中删除指定的元素。
        1.4.4 FIND：查找某个元素在列表中的位置。
        1.4.5 LENGTH：获取列表的长度。
        1.4.6 SORT：对列表进行排序。
        1.4.7 REMOVE_DUPLICATES：移除列表中的重复元素。
]]

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
#[[
Note:杨小兵-2025-01-07

1、指令1
    1.1 指定所有可执行文件（runtime targets）的输出目录。即，生成的可执行文件（如 .exe 文件）将被放置在此目录下。
    1.2 runtime 在 CMake 中主要指的是 可执行文件（executables）。
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

option(BUILD_SHARED_LIBS "build shared libraries" ${BUILD_SHARED_LIBS_DEFAULT})
#[[
Note:杨小兵-2025-01-07

1、指令1
    1.1 设置一个boolean类型的选项从而用户可以进行选择，相当于提供了一个开关
    1.2 第一个参数是变量的名称；第二个参数是对变量的说明；第三个参数是变量的值
]]

if (WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()
#[[
Note:杨小兵-2025-01-04

1、指令1
    1.1 条件判断，WIN32是cmake内置的一个boolean变量，这个变量的作用就是为了标志目标平台是否为windows，这个变量的设置是在cmake配置阶段由cmake自定进行设置的，同编译器没有关系。
    1.2 需要注意的是，WIN32 表示的是 Windows 平台（包括 32 位和 64 位），而不是专门的 32 位系统。
2、指令2
    2.1 指令的作用是向编译器提供一个额外的参数。使用 add_compile_definitions(_CRT_SECURE_NO_WARNINGS) 等价于在编译命令中添加 -D_CRT_SECURE_NO_WARNINGS（对于 GCC/Clang）或 /D_CRT_SECURE_NO_WARNINGS（对于 MSVC）
    2.2 这个命令会在项目的所有目标文件中添加对应的预处理器宏定义。如果你只想对某些目标生效，可以使用更细粒度的 target_compile_definitions。
3、指令3
    3.1 条件判断的结束标识
]]

if (MSVC)
    add_compile_options("$<$<COMPILE_LANGUAGE:C>:/utf-8>")
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/utf-8>")
endif()
#[[
Note:杨小兵-2025-01-07

1、指令1
    1.1 `MSVC` 是 CMake 的内置布尔变量，当 CMake 配置项目时，如果目标生成器是 Microsoft 的 Visual C++ 编译器（MSVC），CMake 会自动定义 `MSVC` 为 `TRUE`
    1.2 条件判断
    1.3 WIN32 和 MSVC 的区别
        1.3.1 概念层面：
            1.3.1.1 WIN32 是一个 平台检测 变量，指示目标平台是否为 Windows 系统。
            1.3.1.2 MSVC 是一个 编译器检测 变量，指示当前使用的编译器是否为 MSVC。
        1.3.2 互斥关系：
            1.3.2.1 虽然在 Windows 平台上常常使用 MSVC 编译器，但 WIN32 和 MSVC 并不是互斥的。即一个项目可以在 Windows 平台上使用不同的编译器（如 MinGW、Clang），因此在 Windows 上 WIN32 为真，而 MSVC 可能为真或假，具体取决于所使用的编译器。
        1.3.3 使用场景：
            1.3.3.1 WIN32：当您需要针对 Windows 平台进行特定配置时使用。
            1.3.3.2 MSVC：当您需要针对 MSVC 编译器进行特定配置时使用。
2、指令2
    2.1 该命令是cmake中的一个内置命令
    2.2 该命令的作用是向全局的编译器命令行添加指定的选项，适用于当前项目中所有目标的编译。相当于为整个项目的所有源文件添加编译器选项。
    2.3 这是 CMake 的生成器表达式（Generator Expressions），是一种在 CMake 配置阶段动态生成编译选项的机制。
    2.4 `/utf-8` 是 MSVC 编译器的一个选项，用于指定源代码文件的字符编码为 UTF-8。这在需要处理包含非 ASCII 字符的源文件时尤为重要。
    2.5 `$<$<COMPILE_LANGUAGE:C>:/utf-8>` 是一个 CMake 生成器表达式，表示仅当当前编译的文件是 C 文件时，才为其添加 `/utf-8` 编译选项。这种机制使得 CMake 配置更加灵活，能够根据条件动态调整编译器选项。
3、指令3
    3.1 条件判断的结束标识
4、注意
    4.1 add_compile_definitions 和 add_compile_options 命令主要用于向编译器传递定义和选项，因此它们的操作对象可以视为编译器及其编译过程。
    4.2 add_compile_definitions（添加预处理器宏定义）
        4.2.1 主要用于添加预处理器宏定义（如 -D 选项）。
        4.2.2 适用于需要在代码中进行条件编译或启用特定功能的场景。
        4.2.3 当需要为代码添加预处理器宏定义时使用。例如，定义调试模式、平台特定宏等。
    4.3 add_compile_options（添加编译器选项）
        4.3.1 主要用于添加编译器选项（如优化级别、警告控制、语言标准等）。
        4.3.2 适用于需要调整编译器行为或配置特定编译选项的场景。
        4.3.3 当需要调整编译器的行为或配置特定编译器选项时使用。例如，设置优化级别、警告级别、语言标准等。
]]


#
# option list
#

# debug
option(LLAMA_ALL_WARNINGS           "llama: enable all compiler warnings"                   ON)
option(LLAMA_ALL_WARNINGS_3RD_PARTY "llama: enable all compiler warnings in 3rd party libs" OFF)
#[[
Note:杨小兵-2025-01-04

1、指令1
    1.1 提供一个Boolean类型的选项从而用户可以进行选择，相当于提供了一个开关
2、指令2
    2.1 创建一个boolean类型的变量从而给用户提供选择，相当于提供了一个开关
]]

# build
option(LLAMA_FATAL_WARNINGS "llama: enable -Werror flag" OFF)
#[[
Note:杨小兵-2025-01-04

1、指令1
    1.1 创建一个boolean类型的变量从而给用户提供选择，相当于提供了一个开关
]]

# sanitizers
option(LLAMA_SANITIZE_THREAD    "llama: enable thread sanitizer"    OFF)
option(LLAMA_SANITIZE_ADDRESS   "llama: enable address sanitizer"   OFF)
option(LLAMA_SANITIZE_UNDEFINED "llama: enable undefined sanitizer" OFF)
#[[
Note:杨小兵-2025-01-04

1、指令1
    1.1 创建一个boolean类型的变量从而给用户提供选择，相当于提供了一个开关
2、指令2
    2.1 创建一个boolean类型的变量从而给用户提供选择，相当于提供了一个开关
3、指令3
    3.1 创建一个boolean类型的变量从而给用户提供选择，相当于提供了一个开关
]]

# utils
option(LLAMA_BUILD_COMMON "llama: build common utils library" ${LLAMA_STANDALONE})
#[[
Note:杨小兵-2025-01-05

1、指令1
    1.1 创建一个boolean类型的变量从而给用户提供选择，相当于提供了一个开关
]]

# extra artifacts
option(LLAMA_BUILD_TESTS    "llama: build tests"          ${LLAMA_STANDALONE})
option(LLAMA_BUILD_EXAMPLES "llama: build examples"       ${LLAMA_STANDALONE})
option(LLAMA_BUILD_SERVER   "llama: build server example" ${LLAMA_STANDALONE})
#[[
Note:杨小兵-2025-01-05

1、指令1
    1.1 创建一个boolean类型的变量从而给用户提供选择，相当于提供了一个开关
2、指令2
    2.1 创建一个boolean类型的变量从而给用户提供选择，相当于提供了一个开关
3、指令3
    3.1 创建一个boolean类型的变量从而给用户提供选择，相当于提供了一个开关
4、名词解释
    4.1 extra artifacts:额外的构建产物
]]

# 3rd party libs
option(LLAMA_CURL "llama: use libcurl to download model from an URL" OFF)
#[[
Note:杨小兵-2025-01-05

1、指令1
    1.1 创建一个boolean类型的变量从而给用户提供选择，相当于提供了一个开关
]]

# Required for relocatable CMake package
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/build-info.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/common.cmake)
#[[
Note:杨小兵-2025-01-05

1、指令1
    1.1 在 CMake 中，include 命令用于包含和执行另一个 CMake 脚本或模块。通过 include，可以引入外部定义的函数、宏、变量和其他设置，从而在当前的 CMakeLists.txt 文件中复用这些内容
        1.1.1 引入模块和功能：CMake 提供了许多内置模块，包含常见的功能检查、编译选项设置等。通过 include，可以使用这些模块提供的功能。
        1.1.2 复用代码： 如果有多个项目或多个部分需要共享相同的 CMake 代码，可以将这些代码放在一个独立的 CMake 脚本中，然后在需要的地方通过 include 引入。
        1.1.3 组织项目结构： 对于大型项目，可以将不同的构建配置和逻辑分割到多个 CMake 脚本中，通过 include 进行组合，提升可读性和维护性。
    1.2 这里提到的模块指的就是对应的*.cmake文件，本质上就是一个脚本文件，只不过这里的脚本文件是被cmake解析器来进行解析的。
    1.3 示例：引入自定义的 CMake 脚本（include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/custom_settings.cmake)）
]]

# override ggml options
set(GGML_SANITIZE_THREAD    ${LLAMA_SANITIZE_THREAD})
set(GGML_SANITIZE_ADDRESS   ${LLAMA_SANITIZE_ADDRESS})
set(GGML_SANITIZE_UNDEFINED ${LLAMA_SANITIZE_UNDEFINED})
set(GGML_ALL_WARNINGS       ${LLAMA_ALL_WARNINGS})
set(GGML_FATAL_WARNINGS     ${LLAMA_FATAL_WARNINGS})
#[[
Note:杨小兵-2025-01-05

1、指令1
    1.1 创建一个Boolean变量用来设置GGML选项
2、指令2
    2.1 创建一个Boolean变量用来设置GGML选项
3、指令3
    3.1 创建一个Boolean变量用来设置GGML选项
4、指令4
    4.1 创建一个Boolean变量用来设置GGML选项
5、指令5
    5.1 创建一个Boolean变量用来设置GGML选项
]]

# change the default for these ggml options
if (NOT DEFINED GGML_LLAMAFILE)
    set(GGML_LLAMAFILE_DEFAULT ON)
endif()
#[[
Note:杨小兵-2025-01-05

1、指令1
    1.1 条件判断，判断是否定义了对应的boolean变量
2、指令2
    2.1 创建一个Boolean变量用来设置GGML选项
3、指令3
    3.1 条件判断的结束标识
]]

if (NOT DEFINED GGML_CUDA_GRAPHS)
    set(GGML_CUDA_GRAPHS_DEFAULT ON)
endif()
#[[
Note:杨小兵-2025-01-05

1、指令1
    1.1 条件判断，判断是否定义了对应的boolean变量
2、指令2
    2.1 创建一个Boolean变量用来设置GGML选项
3、指令3
    3.1 条件判断的结束标识
]]

# transition helpers
function (llama_option_depr TYPE OLD NEW)
    if (${OLD})
        message(${TYPE} "${OLD} is deprecated and will be removed in the future.\nUse ${NEW} instead\n")
        set(${NEW} ON PARENT_SCOPE)
    endif()
endfunction()
#[[
Note:杨小兵-2025-01-05

1、指令1
    1.1 注释标识接下来的代码是“过渡辅助工具”
    1.2 定义函数：声明一个名为 llama_option_depr 的函数，接收三个参数：TYPE、OLD 和 NEW。
2、指令2
    2.1 条件判断：检查变量 OLD 是否被定义且其值为真（即是否启用了旧选项）
3、指令3
    3.1 输出消息：使用 message 命令输出一条消息。TYPE 指定了消息的类型（如 WARNING、STATUS 等）。消息内容指出 OLD 选项已被弃用，未来将被移除，并建议使用 NEW 选项代替。
4、指令4
    4.1 设置新变量：将 NEW 变量设置为 ON，并将其作用域提升到父作用域（即调用该函数的作用域）。这意味着即使在函数内部设置，外部也能访问到 NEW 被设置为 ON 的状态。
5、指令5
    5.1 条件判断结束标识
6、指令6
    6.1 函数定义结束标识
]]

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
#[[
Note:杨小兵-2025-01-05

1、指令1
    1.1 调用前面定义的函数
2、指令2
    2.1 调用前面定义的函数
3、指令3
    3.1 调用前面定义的函数
4、指令4
    4.1 调用前面定义的函数
5、指令5
    5.1 调用前面定义的函数
6、指令6
    6.1 调用前面定义的函数
7、指令7
    7.1 调用前面定义的函数
8、指令8
    8.1 调用前面定义的函数
9、指令9
    9.1 调用前面定义的函数
10、指令10
    10.1 调用前面定义的函数
]]

#
# build the library
#

if (NOT TARGET ggml)
    add_subdirectory(ggml)
    # ... otherwise assume ggml is added by a parent CMakeLists.txt
endif()
add_subdirectory(src)
#[[
Note:杨小兵-2025-01-07

1、指令1
    1.1 条件判断：检查是否已经存在名为 ggml 的目标（target）。TARGET 是 CMake 中用于引用已经定义的目标（如库或可执行文件）。NOT TARGET ggml 表示如果当前项目中还没有定义 ggml 这个目标，则执行接下来的代码块。
    1.2 CMake 提供了一个条件表达式 TARGET <name>，用于检查是否已经定义了名为 <name> 的目标。结合逻辑操作符 NOT，NOT TARGET <name> 的含义就是“如果没有定义名为 <name> 的目标”。
2、指令2
    2.1 添加子目录：将 ggml 目录添加到当前的 CMake 构建过程中。add_subdirectory 命令会进入指定的子目录，并处理其中的 CMakeLists.txt 文件，从而包含该子项目的构建配置。这通常用于包含和编译第三方库或模块。
3、指令3
    3.1 注释：这是一行注释，用于说明如果条件 NOT TARGET ggml 不成立（即目标 ggml 已经存在），则假设 ggml 已经由上级（父级） CMakeLists.txt 文件添加。因此，不需要在当前目录中再次添加 ggml 子目录。
4、指令4
    4.1 条件判断结束标识
5、指令5
    5.1 添加子目录：将 src 目录添加到当前的 CMake 构建过程中。add_subdirectory 命令会进入指定的子目录，并处理其中的 CMakeLists.txt 文件，从而包含该子项目的构建配置。这通常用于包含和编译第三方库或模块。
]]


#
# install
#

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
#[[
Note:杨小兵-2025-01-05

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

set(LLAMA_BUILD_NUMBER        ${BUILD_NUMBER})
set(LLAMA_BUILD_COMMIT        ${BUILD_COMMIT})
set(LLAMA_INSTALL_VERSION 0.0.${BUILD_NUMBER})
#[[
Note:杨小兵-2025-01-05

1、指令1
    1.1 创建一个变量用来设置构建选项
2、指令2
    2.1 创建一个变量用来设置构建选项
3、指令3
    3.1 创建一个变量用来设置构建选项
]]

set(LLAMA_INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR} CACHE PATH "Location of header  files")
set(LLAMA_LIB_INSTALL_DIR     ${CMAKE_INSTALL_LIBDIR}     CACHE PATH "Location of library files")
set(LLAMA_BIN_INSTALL_DIR     ${CMAKE_INSTALL_BINDIR}     CACHE PATH "Location of binary  files")
#[[
Note:杨小兵-2025-01-05

1、指令1
    1.1 创建一个变量用来设置头文件的安装位置
2、指令2
    2.1 创建一个变量用来设置库文件的安装位置
3、指令3
    3.1 创建一个变量用来设置二进制文件的安装位置
]]

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
#[[
Note:杨小兵-2025-01-05

1、指令1
    1.1 install 命令用于指定在安装过程中将文件、目标或目录复制到指定的安装位置。
    1.2 FILES：
        1.2.1 作用：指定要安装的一个或多个文件。
    1.3 DESTINATION：
        1.3.1 作用：定义文件在安装时的目标路径，相对于安装目录（通常由 CMAKE_INSTALL_PREFIX 指定）。
    1.4 在这段 CMake 脚本中，install 命令的作用是将生成的 llama.pc 文件从当前的二进制构建目录复制到最终安装目录的 lib/pkgconfig 子目录中。这样，安装完成后，llama.pc 文件将位于 <安装前缀>/lib/pkgconfig/ 目录下，供其他工具（如 pkg-config）使用。
]]

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
#[[
Note:杨小兵-2025-01-05

1、指令1
    1.1 install 命令用于指定在安装过程中将文件、目标或目录复制到指定的安装位置。
    1.2 FILES：
        1.2.1 作用：指定要安装的一个或多个文件。
    1.3 DESTINATION：
        1.3.1 作用：定义文件在安装时的目标路径，相对于安装目录（通常由 CMAKE_INSTALL_PREFIX 指定）。
    1.4 在这段 CMake 脚本中，install 命令的作用是将生成的 llama.pc 文件从当前的二进制构建目录复制到最终安装目录的 lib/pkgconfig 子目录中。这样，安装完成后，llama.pc 文件将位于 <安装前缀>/lib/pkgconfig/ 目录下，供其他工具（如 pkg-config）使用。
]]

configure_file(cmake/llama.pc.in
        "${CMAKE_CURRENT_BINARY_DIR}/llama.pc"
        @ONLY)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/llama.pc"
        DESTINATION lib/pkgconfig)
#[[
Note:杨小兵-2025-01-05

1、指令1
    1.1 install 命令用于指定在安装过程中将文件、目标或目录复制到指定的安装位置。
    1.2 FILES：
        1.2.1 作用：指定要安装的一个或多个文件。
        1.2.2 在此例中："${CMAKE_CURRENT_BINARY_DIR}/llama.pc" 表示要安装的具体文件路径，即当前二进制目录下的 llama.pc 文件。
    1.3 DESTINATION：
        1.3.1 作用：定义文件在安装时的目标路径，相对于安装目录（通常由 CMAKE_INSTALL_PREFIX 指定）。
        1.3.2 在此例中：lib/pkgconfig 表示 llama.pc 文件将被安装到安装目录下的 lib/pkgconfig 目录中。
    1.4 在这段 CMake 脚本中，install 命令的作用是将生成的 llama.pc 文件从当前的二进制构建目录复制到最终安装目录的 lib/pkgconfig 子目录中。这样，安装完成后，llama.pc 文件将位于 <安装前缀>/lib/pkgconfig/ 目录下，供其他工具（如 pkg-config）使用。
]]
    


#
# utils, programs, examples and tests
#

if (LLAMA_BUILD_COMMON)
    add_subdirectory(common)
endif()
#[[
Note:杨小兵-2025-01-05

1、指令1
    1.1 条件判断指令
2、指令2
    2.1 包含common子文件
3、指令3
    3.1 条件判断结束标识
]]

if (LLAMA_BUILD_COMMON AND LLAMA_BUILD_TESTS AND NOT CMAKE_JS_VERSION)
    include(CTest)
    add_subdirectory(tests)
endif()
#[[
Note:杨小兵-2025-01-05

1、指令1
    1.1 条件判断指令
2、指令2
    2.1 在 CMake 中，include 命令用于包含和执行另一个 CMake 脚本或模块。通过 include，您可以引入外部定义的函数、宏、变量和其他设置，从而在当前的 CMakeLists.txt 文件中复用这些内容
        2.1.1 引入模块和功能：CMake 提供了许多内置模块，包含常见的功能检查、编译选项设置等。通过 include，您可以使用这些模块提供的功能。
        2.1.2 复用代码： 如果您有多个项目或多个部分需要共享相同的 CMake 代码，可以将这些代码放在一个独立的 CMake 脚本中，然后在需要的地方通过 include 引入。
        2.1.3 组织项目结构： 对于大型项目，可以将不同的构建配置和逻辑分割到多个 CMake 脚本中，通过 include 进行组合，提升可读性和维护性。
    2.2 这里提到的模块指的就是对应的*.cmake文件，本质上就是一个脚本文件，只不过这里的脚本文件是被cmake解析器来进行解析的。
    2.3 示例：引入自定义的 CMake 脚本（include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/custom_settings.cmake)）
    2.4 `CTest` 模块为 CMake 项目集成了测试功能，使开发者能够定义、管理和运行自动化测试以验证项目构建的正确性。
3、指令3
    3.1 包含tests子文件
4、指令4
    4.1 条件判断结束标识
]]

if (LLAMA_BUILD_COMMON AND LLAMA_BUILD_EXAMPLES)
    add_subdirectory(examples)
    add_subdirectory(pocs)
endif()
#[[
Note:杨小兵-2025-01-05

1、指令1
    1.1 条件判断指令
2、指令2
    2.1 包含examples子文件
3、指令3
    3.1 包含pocs子文件
]]