# common

find_package(Threads REQUIRED)
#[[
Note:杨小兵-2025-01-08

1、指令1
    1.1 在 CMake 中，find_package() 是一个非常常用的命令，用来自动查找并加载外部库或第三方软件包，或者查找指定库的“配置文件（Config file）”或“模块文件（FindXXX.cmake）”等，从而让后续的 CMake 配置阶段能够正确关联、链接、使用这些依赖。
    1.2 主要功能
        1.2.1 会在系统的指定路径、默认搜索路径或自定义搜索路径中，寻找对应的库 / 组件 / 配置文件，以便后续在 CMake 脚本中引用这些库、使用其头文件、链接其静态/动态库等。
    1.3 两种主要模式：
        1.3.1 Module 模式：CMake 会去寻找一个叫做 Find<PackageName>.cmake 的模块文件（例如 FindThreads.cmake），里面通常会定义变量、目标，或者自定义查找逻辑来找到库文件和头文件路径等。
        1.3.2 Config 模式：CMake 会去寻找一个叫做 <PackageName>Config.cmake 或 <lowercase-packagename>Config.cmake 的配置文件（例如 ThreadsConfig.cmake），里面通常由对应的第三方项目或安装包提供（常见于现代 CMake 项目），里面包含如何链接和使用该库的指令。
]]

llama_add_compile_flags()
#[[
Note:杨小兵-2025-01-08

1、指令1
    1.1 根据项目配置选项（LLAMA_FATAL_WARNINGS 和 LLAMA_ALL_WARNINGS）以及当前编译器类型（GNU/Clang/MSVC），自动为 C/C++ 编译添加相应的警告和错误级别选项：如果启用了“致命警告”，就把所有警告作为错误处理（如 -Werror 或 /WX）；如果启用了“全部警告”，则进一步添加一系列严苛的警告标志（如 -Wall -Wextra -Wpedantic 等）。在 MSVC 下暂留了部分 TODO 待实现。通过这样集中设置编译选项，可以确保代码在不同编译器下尽可能保持相同的警告级别和错误处理策略。
]]

# Build info header
#

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../.git")
    set(GIT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.git")

    # Is git submodule
    if(NOT IS_DIRECTORY "${GIT_DIR}")
        file(READ ${GIT_DIR} REAL_GIT_DIR_LINK)
        string(REGEX REPLACE "gitdir: (.*)\n$" "\\1" REAL_GIT_DIR ${REAL_GIT_DIR_LINK})
        string(FIND "${REAL_GIT_DIR}" "/" SLASH_POS)
        if (SLASH_POS EQUAL 0)
            set(GIT_DIR "${REAL_GIT_DIR}")
        else()
            set(GIT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../${REAL_GIT_DIR}")
        endif()
    endif()

    if(EXISTS "${GIT_DIR}/index")
        set(GIT_INDEX "${GIT_DIR}/index")
    else()
        message(WARNING "Git index not found in git repository.")
        set(GIT_INDEX "")
    endif()
else()
    message(WARNING "Git repository not found; to enable automatic generation of build info, make sure Git is installed and the project is a Git repository.")
    set(GIT_INDEX "")
endif()
#[[
Note:杨小兵-2025-01-08

1、注意
    1.1 这段脚本主要用于判断当前项目是否处于 Git 仓库（或子模块）之中，并定位 Git 的目录索引（`.git/index`）文件所在位置。它先检查顶层 `.git` 是否存在，若发现是子模块结构，则通过读取 `.git` 文件中的指针来确定真正的 Git 路径。随后再检测 `index` 文件是否存在，并在找不到时给出警告。若根本没有发现 `.git`，则提示当前项目并非 Git 仓库、无法自动生成相关构建信息。
    1.2 在 CMake 中，`if(EXISTS <path>)` 用来判断给定的 `<path>` 是否在文件系统中存在（可以是文件或目录）。如果 `<path>` 存在，则条件为真，否则为假。
]]

# Add a custom command to rebuild build-info.cpp when .git/index changes
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/build-info.cpp"
    COMMENT "Generating build details from Git"
    COMMAND ${CMAKE_COMMAND} -DMSVC=${MSVC} -DCMAKE_C_COMPILER_VERSION=${CMAKE_C_COMPILER_VERSION}
            -DCMAKE_C_COMPILER_ID=${CMAKE_C_COMPILER_ID} -DCMAKE_VS_PLATFORM_NAME=${CMAKE_VS_PLATFORM_NAME}
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/build-info-gen-cpp.cmake"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.."
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/build-info.cpp.in" ${GIT_INDEX}
    VERBATIM
)
#[[
Note:杨小兵-2025-01-09

1、注意
    1.1 这段自定义命令利用 `add_custom_command()` 指定了一个生成 `build-info.cpp` 的任务：当 `.git/index` 或模板文件 `build-info.cpp.in` 发生变化时（即触发了 `DEPENDS`），将会执行一个 CMake 脚本（`build-info-gen-cpp.cmake`），在其中根据当前编译器和平台信息生成最新的 `build-info.cpp` 文件，从而实现基于 Git 状态的自动构建信息更新。
]]
set(TARGET build_info)
add_library(${TARGET} OBJECT build-info.cpp)
if (BUILD_SHARED_LIBS)
    set_target_properties(${TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
#[[
Note:杨小兵-2025-01-09

1、指令1
    1.1 set(<variable> <value>) 是 CMake 用于定义或修改变量的语句。
    1.2 将 <variable>（即 TARGET）设置为 <value>（即 build_info）。
    1.3 该变量在当前以及子级 CMakeLists 作用域中都可使用（不使用 PARENT_SCOPE 或 CACHE 的情况下，默认不会传递到父级作用域或 CMake 缓存）。
    1.4 需要注意，这样定义出来的 TARGET 与 CMake 中的“target”（例如用 add_executable()、add_library() 定义的目标）完全是两回事；它仅仅是一个普通的字符串变量，后续脚本可以拿这个值做其他用途。
2、指令2
    2.1 使用 TARGET 变量的值（即 "build_info"） 来创建一个 对象库（OBJECT library），并指定源文件 build-info.cpp。
    2.2 对象库并不会生成最终的 .a 或 .so/.dll 等库文件，而是生成一组可供后续链接的目标文件（.o 或 .obj）。
3、指令3
    3.1 条件判断，其中被判断的值是父级cmake脚本文件定义的
4、指令4
    4.1 如果要构建共享库（BUILD_SHARED_LIBS=ON），则为这个对象库开启位置无关编码（PIC），以确保后续能正确地链接到共享库或以共享方式发布。
    4.2 POSITION_INDEPENDENT_CODE（简称 PIC）用来确保编译出的目标可以被安全地链接成共享库，对于多平台的共享库构建十分关键。若需要生成共享库，就必须启用 PIC（在大多数类 Unix 系统上）。
]]

set(TARGET common)
add_library(${TARGET} STATIC
    arg.cpp
    arg.h
    base64.hpp
    common.cpp
    common.h
    console.cpp
    console.h
    json-schema-to-grammar.cpp
    json.hpp
    log.cpp
    log.h
    ngram-cache.cpp
    ngram-cache.h
    sampling.cpp
    sampling.h
    speculative.cpp
    speculative.h
    )

if (BUILD_SHARED_LIBS)
    set_target_properties(${TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
#[[
Note:杨小兵-2025-01-09

1、注意
    1.1 整体的作用就是创建一个名为common的静态库
    1.2 定义并创建一个名为 “common” 的静态库，包含所列出的多个源码和头文件；同时，如果启用了特定的选项，还会将其编译为位置无关代码（PIC），从而使它在需要动态链接时也具备相应的灵活性。
]]

set(LLAMA_COMMON_EXTRA_LIBS build_info)

# Use curl to download model url
if (LLAMA_CURL)
    find_package(CURL REQUIRED)
    target_compile_definitions(${TARGET} PUBLIC LLAMA_USE_CURL)
    include_directories(${CURL_INCLUDE_DIRS})
    find_library(CURL_LIBRARY curl REQUIRED)
    set(LLAMA_COMMON_EXTRA_LIBS ${LLAMA_COMMON_EXTRA_LIBS} ${CURL_LIBRARY})
endif ()

target_include_directories(${TARGET} PUBLIC .)
target_compile_features   (${TARGET} PUBLIC cxx_std_17)
target_link_libraries     (${TARGET} PRIVATE ${LLAMA_COMMON_EXTRA_LIBS} PUBLIC llama Threads::Threads)
#[[
Note:杨小兵-2025-01-09

1、指令1
    1.1 定义一个自定义的cmake变量，这个变量的类型是字符串类型
2、注释1
    2.1 提示后续代码与使用 curl 下载模型 URL 有关。
3、指令2
    3.1 判断cmake变量是否存在并且为真（通常在命令行或其他 CMake 逻辑中定义/传入）。只有在变量处于开启状态时，下面的语句块才会被执行。
4、指令3
    4.1 查找并加载系统中安装的 CURL（头文件和库）
    4.2 REQUIRED 表示如果找不到 curl，就会报错并停止配置
5、指令4
    5.1 为当前目标添加一个编译器定义（宏）LLAMA_USE_CURL。
    5.2 PUBLIC 意味着所有依赖这个目标的其他目标也能看到这个宏定义
6、指令5
    6.1 将 CURL_INCLUDE_DIRS 中的目录加入到当前编译上下文的头文件搜索路径中
    6.2 这是一个较旧的写法，现在更常用 target_include_directories()，但功能相同：让编译器知道从哪里能找到 <curl/curl.h> 等头文件
7、指令6
    7.1 在系统中搜索名为 curl 的库文件，并将其路径赋给变量 CURL_LIBRARY。找不到则报错
8、指令7
    8.1 将已找到的 CURL_LIBRARY 追加到 LLAMA_COMMON_EXTRA_LIBS 变量中，使后续链接时会包含 curl 库。
9、指令8
    9.1 条件判断结束标识
10、指令9
    10.1 为当前目标设置头文件包含目录 .（当前目录）
    10.2 PUBLIC 表示除了该目标自身外，依赖它的其他目标也会继承这个包含路径
11、指令10
    11.1 指定目标 ${TARGET} 需要使用 C++17 标准特性进行编译
    11.2 PUBLIC 同样表示依赖此目标的其他目标也会继承这项编译特性
12、指令11
    12.1 为当前目标链接一系列依赖库
    12.2 PRIVATE ${LLAMA_COMMON_EXTRA_LIBS}：以私有方式链接先前累积的 LLAMA_COMMON_EXTRA_LIBS（如 build_info、curl 等）。私有依赖不会向更上层传递。
    12.3 PUBLIC llama Threads::Threads：以公有方式链接 llama 和线程库（Threads::Threads）。任何链接到 ${TARGET} 的其他目标也会隐式地链接到这些库。
]]
