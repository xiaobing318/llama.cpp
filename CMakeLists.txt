#[[
1. `cmake_minimum_required(VERSION 3.14)`
   - 这一行指定了当前CMake项目所需的最低CMake版本。在这个例子中，它要求至少使用CMake版本3.14。这是因为一些特定的功能或者选项可能在旧版本的CMake中
   不可用（CMake版本至少为3.14才能提供当前项目所需要的特定功能）。在这里提到的3.14版本支持`add_link_options`和隐式目标目录等功能。

2. `project("llama.cpp" C CXX)`
   - 这行命令定义了项目的名称以及项目支持的编程语言。项目名称被设置为"llama.cpp"，同时指定了这个项目使用C和C++语言。`C`代表C语言，`CXX`代表C++语言。
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
project("llama.cpp" C CXX)
include(CheckIncludeFileCXX)















#[[
### 1、CMake 命令解释

1. `#set(CMAKE_WARN_DEPRECATED YES)`
   - 这行命令使用了 `#` 符号进行注释，因此不会执行。如果去掉 `#`，该命令将设置 `CMAKE_WARN_DEPRECATED` 变量为 `YES`，这样做会让 CMake 在使用
   已废弃的功能时发出警告。这有助于维护代码的现代性，确保不使用过时的技术。

2. `set(CMAKE_WARN_UNUSED_CLI YES)`
   - 这行命令设置了 `CMAKE_WARN_UNUSED_CLI` 变量为 `YES`。当设置此变量时，CMake 会在命令行提供了未在 CMakeLists.txt 中使用的变量时发出警告。
   这有助于识别可能的打字错误或未使用的配置选项。

3. `set(CMAKE_EXPORT_COMPILE_COMMANDS ON)`
   - 这行命令设置 `CMAKE_EXPORT_COMPILE_COMMANDS` 变量为 `ON`。开启此选项后，CMake 会生成一个 `compile_commands.json` 文件，这个文件包含了
   所有编译单元的具体编译命令。这对于使用某些工具进行代码分析、自动完成或重构非常有用，如 Clang 工具。

### 是否为 CMake 内置变量

   上述提到的变量都是 CMake 的内置变量，它们是 CMake 配置过程中可以使用来控制特定行为的预定义变量。这些变量允许用户定制 CMake 的行为，比如调整
警告输出或控制特定文件的生成，以适应不同的开发需求和环境。
]]
#set(CMAKE_WARN_DEPRECATED YES)
set(CMAKE_WARN_UNUSED_CLI YES)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
















#[[
### CMake 命令解释

1. `if (NOT XCODE AND NOT MSVC AND NOT CMAKE_BUILD_TYPE)`
   - 这是一个条件判断语句。`if` 语句检查是否不是在 Xcode 或 MS Visual Studio 环境中编译，并且没有设置 `CMAKE_BUILD_TYPE` 变量。如果所有这些条件
   都满足（即不在 Xcode 或 MSVC 中，且 `CMAKE_BUILD_TYPE` 没有预先定义），则执行后续的 `set` 命令。

2. `set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)`
   - 这行命令在上述条件为真时执行。它将 `CMAKE_BUILD_TYPE` 变量设置为 "Release"。此外，这个设置使用 `CACHE` 选项，意味着变量值被缓存起来，可以在
   不同的 CMake 运行之间保持。`STRING` 表明这是一个字符串变量，`"Build type"` 是这个缓存条目的描述。`FORCE` 表示即使变量已经在缓存中存在，也要强制
   更新它的值。

3. `set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")`
   - 这行命令为 `CMAKE_BUILD_TYPE` 缓存变量设置了属性。具体地，它定义了可能的字符串值：`"Debug"`, `"Release"`, `"MinSizeRel"` 和 
   `"RelWithDebInfo"`。这有助于在基于图形的配置工具中为用户提供可选的构建类型选项。

### 是否为 CMake 内置变量

- `CMAKE_BUILD_TYPE`: 是 CMake 的内置变量。它用于指定要使用的构建类型，通常影响编译器的优化级别和是否包含调试信息。
- `XCODE` 和 `MSVC`: 这些也是 CMake 内置变量，用于检测当前的生成环境是否为 Xcode 或 Microsoft Visual Studio。

这些内置变量使得 CMake 脚本能够根据不同的编译环境做出调整，确保在多种平台和编译器上的兼容性和最优配置。
]]
if (NOT XCODE AND NOT MSVC AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()






# 杨小兵-2024-08-10：Set MSVC specific flags
if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8")
endif()









#[[
# Add path to modules
1、将当前CMakeLists.txt文件所在的目录中的cmake添加到CMAKE_MODULE_PATH中，这样CMake可以根据CMAKE_MODULE_PATH来找到想要的.cmake文件
2、CMAKE_MODULE_PATH可以认为是字符串列表
]]
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")







#[[
### CMake 命令解释

- `set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)`
   - 这行命令设置了变量 `CMAKE_RUNTIME_OUTPUT_DIRECTORY` 的值为 `${CMAKE_BINARY_DIR}/bin`。这意味着所有可执行文件（runtime targets）将被
   放置在构建目录下的 `bin` 子目录中。此设置有助于组织和管理生成的二进制文件，使它们与源代码和其他构建产物（如库文件）分离。

### 是否为 CMake 内置变量

- `CMAKE_RUNTIME_OUTPUT_DIRECTORY`: 是 CMake 的内置变量，用于指定生成的运行时目标（通常是可执行文件）的存放目录。
- `CMAKE_BINARY_DIR`: 也是 CMake 的内置变量，代表当前正在进行的构建操作的顶级目录，通常是项目的 `build` 目录。

    这两个变量的结合使用非常常见，目的是将生成的文件（如可执行程序）放置在一个统一的、结构化的位置，这样做有助于后续的运行和管理。
]]
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)








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

### 是否为 CMake 内置变量

- `CMAKE_SOURCE_DIR` 和 `CMAKE_CURRENT_SOURCE_DIR` 都是 CMake 的内置变量。这些变量由CMake自动设置，用于指示源代码的相关路径，非常有用
于控制项目的结构和编译行为。
- `LLAMA_STANDALONE` 不是一个内置变量，而是这个特定CMake项目中定义的一个自定义变量，用于标识项目的独立性。
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

    option(LLAMA_WASM_SINGLE_FILE "llama: embed WASM inside the generated llama.js" ON)
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
option(BUILD_SHARED_LIBS "build shared libraries" ${BUILD_SHARED_LIBS_DEFAULT})







#[[
1、如果WIN32变量是TRUE，那么添加编译定义
2、将会在编译项目的时候给MSVC compiler传入_CRT_SECURE_NO_WARNINGS参数
]]
if (WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
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

# extra artifacts
option(LLAMA_BUILD_TESTS    "llama: build tests"          ${LLAMA_STANDALONE})
option(LLAMA_BUILD_EXAMPLES "llama: build examples"       ${LLAMA_STANDALONE})
option(LLAMA_BUILD_SERVER   "llama: build server example" ${LLAMA_STANDALONE})

# 3rd party libs
option(LLAMA_CURL "llama: use libcurl to download model from an URL" OFF)

# Required for relocatable CMake package
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/build-info.cmake)

# override ggml options
set(GGML_SANITIZE_THREAD    ${LLAMA_SANITIZE_THREAD})
set(GGML_SANITIZE_ADDRESS   ${LLAMA_SANITIZE_ADDRESS})
set(GGML_SANITIZE_UNDEFINED ${LLAMA_SANITIZE_UNDEFINED})
set(GGML_ALL_WARNINGS       ${LLAMA_ALL_WARNINGS})
set(GGML_FATAL_WARNINGS     ${LLAMA_FATAL_WARNINGS})










#[[
# change the default for these ggml options
    这段CMake脚本主要用于设置特定配置选项的默认值，具体用于确保在配置时如果某些变量未被明确设置，则为它们提供一个默认值。
以下是对各个部分的详细解释：

### 命令解释

1. `if (NOT DEFINED GGML_LLAMAFILE)`
   - 这行命令检查变量 `GGML_LLAMAFILE` 是否未被定义。`NOT DEFINED` 是一个条件检查，用于确定一个变量是否没有在CMake过程中设置。

2. `set(GGML_LLAMAFILE ON)`
   - 如果 `GGML_LLAMAFILE` 未定义，这条命令将其设置为 `ON`。这个设置可能影响项目的编译或配置过程，具体取决于 `GGML_LLAMAFILE` 变量在项目中的作用。
   例如，这个变量可能控制某些特定的编译选项或功能是否启用。

3. `endif()`
   - 结束第一个 `if` 块，这是CMake中条件语句的标准结构。

4. `if (NOT DEFINED GGML_CUDA_USE_GRAPHS)`
   - 类似于第一条 `if` 语句，这行检查另一个变量 `GGML_CUDA_USE_GRAPHS` 是否未定义。

5. `set(GGML_CUDA_USE_GRAPHS ON)`
   - 如果 `GGML_CUDA_USE_GRAPHS` 未定义，这条命令将其设置为 `ON`。考虑到CUDA的语境，`GGML_CUDA_USE_GRAPHS` 可能控制CUDA图形功能的使用，这是
   CUDA最新技术之一，可以优化GPU上任务的执行和管理。

6. `endif()`
   - 结束第二个 `if` 块。

### 总结

    这段代码的主要目的是为特定的配置变量提供默认值，确保在项目配置过程中，这些关键功能默认是启用的，除非用户明确地禁用它们。这样做可以简化配置过程，
也确保了项目在缺乏明确设置时的行为一致性。这种做法在管理复杂项目时尤其有用，可以减少因配置遗漏而导致的潜在问题。
]]
if (NOT DEFINED GGML_LLAMAFILE)
    set(GGML_LLAMAFILE ON)
endif()

if (NOT DEFINED GGML_CUDA_USE_GRAPHS)
    set(GGML_CUDA_USE_GRAPHS ON)
endif()








#[[

这段CMake脚本定义了一个名为 `llama_option_depr` 的函数，旨在处理项目中选项的过时（deprecated）状态。下面是对这个函数及其组件的详细解释：

### 函数解释

1. `function (llama_option_depr TYPE OLD NEW)`
   - `llama_option_depr` 是函数的名称，它接受三个参数：`TYPE`，`OLD`，和 `NEW`。
     - `TYPE` 用于指定消息的类型，比如 `WARNING` 或 `DEPRECATION`，这会影响消息的显示方式。
     - `OLD` 是被弃用的选项的名称。
     - `NEW` 是推荐使用的新选项的名称。

2. `if (${OLD})`
   - 这个 `if` 语句检查旧变量 `OLD` 是否被设置为 `TRUE`。如果为 `TRUE`，则执行内部的命令。这里 `${OLD}` 通过解引用获取变量 `OLD` 的值。

3. `message(${TYPE} "${OLD} is deprecated and will be removed in the future.\nUse ${NEW} instead\n")`
   - 这条 `message` 命令用于输出一条指定类型的消息，通知用户一个旧选项已被废弃并将来会被移除，同时推荐改用一个新选项。消息的内容和类型由函数的
   参数动态确定。

4. `set(${NEW} ON PARENT_SCOPE)`
   - 这个 `set` 命令将新选项 `${NEW}` 设置为 `ON`，并且使用 `PARENT_SCOPE` 参数，这意味着变量设置会影响到调用这个函数的作用域之外。这是确保
   在整个项目中，新选项被激活的一种方式。

5. `endif()`
   - 结束 `if` 语句。

### 总结

    这个函数为CMake项目提供了一种管理和迁移过时配置选项的方法。通过这种方式，项目维护者可以更容易地引导用户从旧的配置方法过渡到新的配置方法，同时
保持代码的整洁和最新状态。这对于保持大型项目的健康和可维护性至关重要。
]]
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
#[[

这段CMake脚本用于条件性地包含一个子目录作为构建过程的一部分。以下是对这段代码的详细解释：

### 命令解释
1. `if (NOT TARGET ggml)`
   - 这个 `if` 语句检查名为 `ggml` 的目标是否尚未定义。在CMake中，一个“目标”通常代表一个构建目标，如可执行文件、库或自定义目标。`NOT TARGET` 
   检查的是是否没有名为 `ggml` 的目标存在（名为ggml的目标是否存在）。

2. `add_subdirectory(ggml)`
   - 如果没有名为 `ggml` 的目标，这条命令将 `ggml` 目录添加到构建过程中。`add_subdirectory` 命令指示 CMake 读取指定子目录中的 CMakeLists.txt 
   文件，并将其包含到主项目的构建配置中。这通常用于添加项目依赖的模块或库。

3. `# ... otherwise assume ggml is added by a parent CMakeLists.txt`
   - 这是一条注释，说明如果已经存在 `ggml` 目标，将假定这个目标已经通过上级（parent）CMakeLists.txt文件添加到了项目中。这种情况下，不需要重复添加
   同一个子目录，避免了构建冲突或重复处理。

4. `endif()`
   - 结束 `if` 语句块。

### 总结

    这段代码用于管理项目中的依赖和模块，确保 `ggml` 目录只被添加一次，无论是直接在当前的 CMakeLists.txt 中添加，还是在更高级别的 CMakeLists.txt 
中已经添加。这种管理方式有助于避免重复包含相同的构建目标，保持项目构建的整洁和有效性。
]]
if (NOT TARGET ggml)
    add_subdirectory(ggml)
    # ... otherwise assume ggml is added by a parent CMakeLists.txt
endif()
#添加子目录src，在src这个子目录中也有对应的CMakeLists.txt，递归执行
add_subdirectory(src)

















#
# install
#

#[[
### 命令解释

1. `include(GNUInstallDirs)`
   - 这个命令包含了`GNUInstallDirs`模块，该模块定义了一系列变量来指定安装目录的约定（如库文件、头文件和可执行文件的默认安装位置）。这有助于确保项目
   遵循GNU的安装目录标准，使得项目在不同系统上的安装更加一致。

2. `include(CMakePackageConfigHelpers)`
   - 包含`CMakePackageConfigHelpers`模块，提供了一些宏和函数来帮助创建和安装CMake包配置文件。这些工具用于生成配置文件，使得其他项目可以通过
   `find_package()`命令找到并正确使用该项目的库。

3. `set(LLAMA_BUILD_NUMBER ${BUILD_NUMBER})`
   - 将变量`LLAMA_BUILD_NUMBER`设置为`BUILD_NUMBER`的值。这通常用于在构建过程中指定构建号，可能从外部传入（如CI系统）。

4. `set(LLAMA_BUILD_COMMIT ${BUILD_COMMIT})`
   - 类似地，这条命令将`LLAMA_BUILD_COMMIT`设置为`BUILD_COMMIT`的值，通常表示当前代码库中的提交ID，用于跟踪构建与特定版本控制状态的关联。

5. `set(LLAMA_INSTALL_VERSION 0.0.${BUILD_NUMBER})`
   - 设置项目的安装版本号，格式为`0.0.x`，其中`x`是构建号。这有助于版本控制和在发布时区分不同的构建。

6. `set(LLAMA_INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR} CACHE PATH "Location of header files")`
   - 设置`LLAMA_INCLUDE_INSTALL_DIR`变量，并将其缓存，指定头文件的安装目录为`CMAKE_INSTALL_INCLUDEDIR`（由`GNUInstallDirs`模块定义）。
   `CACHE PATH`指定这是一个路径类型的变量，"Location of header files"是变量的描述。

7. `set(LLAMA_LIB_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR} CACHE PATH "Location of library files")`
   - 设置库文件的安装目录为`CMAKE_INSTALL_LIBDIR`，与头文件的设置方法相同。这也是被`GNUInstallDirs`定义的标准库文件目录。

8. `set(LLAMA_BIN_INSTALL_DIR ${CMAKE_INSTALL_BINDIR} CACHE PATH "Location of binary files")`
   - 设置可执行文件的安装目录为`CMAKE_INSTALL_BINDIR`，保持与其他安装路径设置的一致性。这也是一个被`GNUInstallDirs`定义的标准二进制文件目录。

### 总结

    这些命令共同工作，以确保项目配置符合GNU的目录标准，并且项目的版本信息、库、头文件和可执行文件的安装位置都明确并易于管理。这有助于简化项目的包装、
分发和使用。
]]
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(LLAMA_BUILD_NUMBER        ${BUILD_NUMBER})
set(LLAMA_BUILD_COMMIT        ${BUILD_COMMIT})
set(LLAMA_INSTALL_VERSION 0.0.${BUILD_NUMBER})

set(LLAMA_INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR} CACHE PATH "Location of header  files")
set(LLAMA_LIB_INSTALL_DIR     ${CMAKE_INSTALL_LIBDIR}     CACHE PATH "Location of library files")
set(LLAMA_BIN_INSTALL_DIR     ${CMAKE_INSTALL_BINDIR}     CACHE PATH "Location of binary  files")











#[[
# At the moment some compile definitions are placed within the ggml/src
# directory but not exported on the `ggml` target. This could be improved by
# determining _precisely_ which defines are necessary for the llama-config
# package.

目前，一些编译定义位于 ggml/src 目录中，但未导出到 `ggml` 目标。可以通过确定 llama-config 包所需的定义来改进此问题。
]]
get_target_property(GGML_DIRECTORY ggml SOURCE_DIR)
get_directory_property(GGML_DIR_DEFINES DIRECTORY ${GGML_DIRECTORY} COMPILE_DEFINITIONS)
get_target_property(GGML_TARGET_DEFINES ggml COMPILE_DEFINITIONS)
set(GGML_TRANSIENT_DEFINES ${GGML_TARGET_DEFINES} ${GGML_DIR_DEFINES})
get_target_property(GGML_LINK_LIBRARIES ggml LINK_LIBRARIES)

set_target_properties(llama PROPERTIES PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/include/llama.h)
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
# programs, examples and tests
#

add_subdirectory(common)

if (LLAMA_BUILD_TESTS AND NOT CMAKE_JS_VERSION)
    include(CTest)
    add_subdirectory(tests)
endif ()

if (LLAMA_BUILD_EXAMPLES)
    add_subdirectory(examples)
    add_subdirectory(pocs)
endif()
