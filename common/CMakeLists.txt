# common
#[[
    `find_package` 是 CMake 中的一个命令，用于在配置编译环境时查找并加载外部项目或库。具体到 `find_package(Threads REQUIRED)` 这一命令，其作用
和细节如下：

1. **查找 Threads 库**：这个命令告诉 CMake 查找系统上可用的线程库。线程库是进行多线程编程时必须的，它提供了创建和管理线程的功能。

2. **REQUIRED 关键字**：这个关键字指示 CMake 如果找不到线程库，则必须终止配置过程。这是因为如果该库对项目来说是必需的，没有找到相应的库将导致项目
无法正确编译或运行。

3. **使用结果**：如果成功找到线程库，CMake 会设置一些变量，包括库的路径和其他可能的标志。这些变量随后可以在 CMakeLists.txt 文件中用来链接这些库到
你的项目中。

4. **跨平台兼容性**：`find_package(Threads)` 命令的优点之一是它提供了跨平台的兼容性。例如，在 Unix-like 系统中，它可能找到 pthread 库，而在
Windows 上则可能是 Windows 线程库。

### 1. 模块和配置模式
    `find_package` 命令有两种主要的工作模式：模块模式（Module mode）和配置模式（Config mode）。对于线程库，CMake 通常使用模块模式。

### 2. CMake Modules
    在模块模式下，`find_package(Threads)` 会查找名为 `FindThreads.cmake` 的模块。CMake 自带了许多这样的模块，它们位于 CMake 的安装目录下的 
`Modules` 文件夹中。

### 3. FindThreads.cmake
    `FindThreads.cmake` 是一个特殊的脚本，包含了查找和设置线程库所需要的指令和逻辑。这个脚本会根据不同操作系统的特点来确定如何查找线程库。例如，它
会检查 POSIX 线程（pthread）是否可用，并据此设置必要的编译器和链接器标志。

### 4. 系统特定的检查
    这个脚本可能会使用一些特殊的命令和检查来确定线程库的位置和可用性。例如，它可能会检查操作系统提供的标准位置，或者使用特定的编译器标志来测试编译器
是否支持 `-pthread` 选项。

### 5. 缓存结果
    一旦找到合适的线程库，`FindThreads.cmake` 会将找到的结果和配置存储在 CMake 缓存中，以便后续的配置过程可以重新使用这些结果，而不需要再次进行检测。

]]
find_package(Threads REQUIRED)






















# Build info header
#
#[[
    这段 CMake 脚本的作用是用来确定当前项目是否位于一个 Git 仓库中，并尝试找到 Git 索引文件的位置。这通常用于在构建过程中集成版本控制信息。下面是
对每一部分的详细解释：

### 1. 检查 `.git` 目录的存在
```cmake
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../.git")
```
这行代码检查当前项目目录的上一级目录中是否存在 `.git` 目录。这通常是一个 Git 仓库的标志。

### 2. 设置 `GIT_DIR` 变量
```cmake
set(GIT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.git")
```
如果 `.git` 存在，这行代码将 `GIT_DIR` 变量设置为 `.git` 目录的路径。

### 3. 检查是否为 Git 子模块
```cmake
if(NOT IS_DIRECTORY "${GIT_DIR}")
```
如果 `GIT_DIR` 不是一个目录，那么它可能是一个包含相对路径的文件，这是 Git 子模块的常见配置。

```cmake
file(READ ${GIT_DIR} REAL_GIT_DIR_LINK)
string(REGEX REPLACE "gitdir: (.*)\n$" "\\1" REAL_GIT_DIR ${REAL_GIT_DIR_LINK})
```
这段代码读取 `GIT_DIR` 文件的内容，通常包含形如 `gitdir: <path-to-real-git-dir>` 的文本，并提取实际的 Git 目录路径。

### 4. 解析和设置真实的 Git 目录路径
```cmake
string(FIND "${REAL_GIT_DIR}" "/" SLASH_POS)
if (SLASH_POS EQUAL 0)
    set(GIT_DIR "${REAL_GIT_DIR}")
else()
    set(GIT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../${REAL_GIT_DIR}")
endif()
```
这部分代码检查提取的路径是否是绝对路径（以 `/` 开始）。如果不是，它会将路径视为相对于当前项目目录的上一级目录的相对路径。

### 5. 检查 Git 索引文件
```cmake
if(EXISTS "${GIT_DIR}/index")
    set(GIT_INDEX "${GIT_DIR}/index")
else()
    message(WARNING "Git index not found in git repository.")
    set(GIT_INDEX "")
endif()
```
    这段代码检查在确定的 Git 目录中是否存在 `index` 文件，这是 Git 仓库索引的标志。如果文件存在，它将 `GIT_INDEX` 设置为该文件的路径；如果不存在，
发出警告。

### 6. 没有找到 Git 仓库的情况
```cmake
else()
    message(WARNING "Git repository not found; to enable automatic generation of build info, make sure Git is installed and the project is a Git repository.")
    set(GIT_INDEX "")
endif()
```
如果最初的 `.git` 目录检查失败，将发出警告信息，说明未找到 Git 仓库，并且 `GIT_INDEX` 被设置为空字符串。

    总的来说，这段脚本对构建系统提供了检测和适应 Git 版本控制的能力，特别是在处理子模块或非标准 Git 仓库布局时。这可以用于在构建过程中自动包含版本
控制信息，例如生成包含 Git 提交哈希的版本号。
]]
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
# Add a custom command to rebuild build-info.cpp when .git/index changes
    这段 CMake 脚本定义了一个自定义命令，用于在 `.git/index` 文件发生变化时重新生成 `build-info.cpp` 文件。这是版本控制信息自动集成到构建过程中的
一个实用方式，特别是在开发阶段。下面是对每个部分的详细解释：

### 1. 输出文件指定
```cmake
OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/build-info.cpp"
```
这指定了自定义命令的输出文件，即 `build-info.cpp`。CMake 将监控这个文件，如果它不存在或需要更新，则执行此命令。

### 2. 注释
```cmake
COMMENT "Generating build details from Git"
```
这为自定义命令提供了一个注释，当命令执行时，这个注释会显示在构建输出中，帮助开发者理解正在发生的操作。

### 3. 执行的命令
```cmake
COMMAND ${CMAKE_COMMAND} -DMSVC=${MSVC} -DCMAKE_C_COMPILER_VERSION=${CMAKE_C_COMPILER_VERSION}
        -DCMAKE_C_COMPILER_ID=${CMAKE_C_COMPILER_ID} -DCMAKE_VS_PLATFORM_NAME=${CMAKE_VS_PLATFORM_NAME}
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/build-info-gen-cpp.cmake"
```
    这一行指定了实际执行的命令。它调用 CMake 自身（`${CMAKE_COMMAND}`）以处理模式运行一个脚本 (`-P`)。这个脚本位于 
`cmake/build-info-gen-cpp.cmake`，用于生成 `build-info.cpp` 文件。传递给脚本的参数包括各种编译器和平台的信息，这些信息可能会被脚本使用来定制生成
的代码。

### 4. 工作目录
```cmake
WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.."
```
这指定了命令运行时的工作目录。在这种情况下，它被设置为当前源目录的父目录。

### 5. 依赖项
```cmake
DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/build-info.cpp.in" ${GIT_INDEX}
```
    这定义了命令的依赖项。如果 `build-info.cpp.in`（模板文件）或 `${GIT_INDEX}`（Git 索引文件，指示版本控制状态的变更）发生变化，CMake 将重新运行
此命令以更新 `build-info.cpp` 文件。

### 6. VERBATIM
```cmake
VERBATIM
```
这个关键字确保命令在所有平台上以字面意义解释，避免了对特殊字符的解释问题，如在某些系统上可能会对命令行参数进行错误的解释。

### 总结
    这个自定义命令确保了每当 Git 版本控制的状态发生变化时（如进行了新的提交），`build-info.cpp` 能够基于最新的代码库状态自动重新生成。这对于在软件
构建中包含确切的版本控制信息非常有用，例如记录构建版本时使用的确切提交哈希。这在跟踪问题和管理不同版本的发布中非常重要。
]]
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
这段 CMake 脚本定义了一个对象库的构建配置，并根据是否要构建共享库设置了一些属性。具体的解释如下：

### 1. 设置目标名称
```cmake
set(TARGET build_info)
```
    这行命令设置了一个变量 `TARGET`，其值为 `build_info`。这个变量后续用来指代构建目标的名称，这样可以在脚本中多处使用而无需重复写出具体的名字。

### 2. 添加对象库
```cmake
add_library(${TARGET} OBJECT build-info.cpp)
```
    这行命令使用 `add_library` 函数创建一个对象库（Object Library）。对象库是一个编译目标，它不会被直接打包成最终的二进制文件，而是生成中间的
对象文件（.o 或 .obj 文件），这些对象文件后续可以链接到其他的静态或动态库中。在这个例子中，`build-info.cpp` 被编译为对象文件，并与变量 `${TARGET}`（即 `build_info`）关联。

### 3. 设置位置无关代码
```cmake
if (BUILD_SHARED_LIBS)
    set_target_properties(${TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
```
    这部分代码检查一个变量 `BUILD_SHARED_LIBS` 是否被设置为 `true`。这个变量通常在项目的顶级 CMake 文件中定义，用来指示是否应构建共享库（动态库）。

- **POSITION_INDEPENDENT_CODE ON**：如果 `BUILD_SHARED_LIBS` 是 `true`，则为目标设置 `POSITION_INDEPENDENT_CODE` 属性为 `ON`。位置无关代码
（PIC）是在创建共享库时必须的，因为它允许生成的代码在内存中可被多个程序共享，不依赖于具体的地址。设置为 `ON` 确保对象文件是为了支持动态链接而生成的。

### 总结
    这段 CMake 脚本的主要目的是编译一个中间的对象文件，这些文件在构建过程中不立即构成最终的可执行文件或库，但可用于创建多个版本的库（静态或动态）。
通过条件性地设置位置无关代码，它还支持基于配置决定是否适用于动态库的构建，增加了构建系统的灵活性。
]]
set(TARGET build_info)
add_library(${TARGET} OBJECT build-info.cpp)
if (BUILD_SHARED_LIBS)
    set_target_properties(${TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()






# 这行命令使用新的值覆盖变量 `TARGET`，更新后的值为 `common`
set(TARGET common)
#[[ 
    这行命令使用 add_library 函数来定义一个静态库（STATIC），名字由 ${TARGET}（即 common）指定。随后列出的是构成这个库的所有源文件和头文件。
静态库编译后的结果是一个 .a 或 .lib 文件，它可以被其他项目或执行文件链接。
]]
add_library(${TARGET} STATIC
    base64.hpp
    common.h
    common.cpp
    sampling.h
    sampling.cpp
    console.h
    console.cpp
    grammar-parser.h
    grammar-parser.cpp
    json.hpp
    json-schema-to-grammar.cpp
    train.h
    train.cpp
    ngram-cache.h
    ngram-cache.cpp
    )

if (BUILD_SHARED_LIBS)
    set_target_properties(${TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()




#[[
    这段 CMake 脚本用于集成 cURL 库到一个 C++ 项目中，以便下载和处理网络资源。脚本首先检查是否定义了 `LLAMA_CURL` 变量（通常在 CMake 脚本的配置
阶段设定），如果定义了，则执行以下操作：

### 1. **查找 cURL 包**
```cmake
find_package(CURL REQUIRED)
```
    这行命令使用 CMake 的 `find_package` 函数来查找 cURL 库。`REQUIRED` 关键字指定了 cURL 是必需的；如果找不到 cURL，CMake 配置过程会报错并停止。

### 2. **添加预处理器定义**
```cmake
add_definitions(-DLLAMA_USE_CURL)
```
    此命令添加一个预处理器定义 `LLAMA_USE_CURL`，这通常用于源代码中以条件编译的方式包含使用 cURL 的代码段。当代码被编译时，任何依赖于这个宏的部分都
将被激活。

### 3. **包含 cURL 的头文件目录**
```cmake
include_directories(${CURL_INCLUDE_DIRS})
```
    `CURL_INCLUDE_DIRS` 是 `find_package` 查找 cURL 时设置的一个变量，它包含了 cURL 头文件的目录。这行命令将这些目录添加到编译器的头文件搜索路径
中，使得项目中的其他源文件可以方便地包含 cURL 的头文件。

### 4. **查找 cURL 库文件**
```cmake
find_library(CURL_LIBRARY curl REQUIRED)
```
这行命令查找实际的 cURL 库文件。如果设置为 `REQUIRED`，则该库文件必须存在，否则 CMake 将报错。这里查找名为 "curl" 的库。

### 5. **添加 cURL 库到链接库列表**
```cmake
set(LLAMA_COMMON_EXTRA_LIBS ${LLAMA_COMMON_EXTRA_LIBS} ${CURL_LIBRARY})
```
    最后，这行命令将 cURL 库添加到一个现有的库列表变量 `LLAMA_COMMON_EXTRA_LIBS` 中。这个变量后续可能用于链接这些库到某个目标（例如可执行文件或其
他库）。

### 总结
    通过这些步骤，CMake 脚本配置了项目，使其能够在源代码中使用 cURL 库进行网络通信。这包括查找必要的库和头文件，并确保它们在编译时可用，并且在最终的
链接阶段，确保 cURL 库被正确地链接到目标中。这种配置方法确保了构建系统的灵活性和可移植性，使得项目可以在不同的开发环境中轻松配置和使用 cURL。
]]
set(LLAMA_COMMON_EXTRA_LIBS build_info)
# Use curl to download model url
if (LLAMA_CURL)
    find_package(CURL REQUIRED)
    add_definitions(-DLLAMA_USE_CURL)
    include_directories(${CURL_INCLUDE_DIRS})
    find_library(CURL_LIBRARY curl REQUIRED)
    set(LLAMA_COMMON_EXTRA_LIBS ${LLAMA_COMMON_EXTRA_LIBS} ${CURL_LIBRARY})
endif ()

target_include_directories(${TARGET} PUBLIC .)
target_compile_features   (${TARGET} PUBLIC cxx_std_11)
target_link_libraries     (${TARGET} PRIVATE ${LLAMA_COMMON_EXTRA_LIBS} PUBLIC llama Threads::Threads)
