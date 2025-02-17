set(TARGET llama-server)
#[[
Notes:杨小兵-2025-02-14

1、通过cmake中的内置set命令设置目标对象的名称为llama-server。
2、这个目标对象的名称将会在后续的构建流程中进行引用。
]]

option(LLAMA_SERVER_SSL "Build SSL support for the server" OFF)
#[[
Notes:杨小兵-2025-02-14

1、通过cmake中的内置option命令创建一个可选项，开发者在构建llama-server的时候可以根据该选项来决定是否构建SSL支持。
2、对于camke中的内置option命令需要设置默认值，在cmake-gui中可以直接看到该选项的设置项。
]]

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
#[[
Notes:杨小兵-2025-02-14

1、通过cmake中的内置include_directories命令设置头文件的搜索路径。
2、C/C++中基本上都会使用到头文件，在包含头文件的时候，编译器需要知道头文件的具体位于哪里，所以需要通过这种方式来设置头文件的搜索路径。
]]

if (MINGW)
    # fix: https://github.com/ggerganov/llama.cpp/actions/runs/9651004652/job/26617901362?pr=8006
    add_compile_definitions(_WIN32_WINNT=${GGML_WIN_VER})
endif()
#[[
Notes:杨小兵-2025-02-14

1、MINGW是cmake中内置的一个变量用来检测当前的构建环境是否使用MinGW（即GCC编译器在windows平台上的实现），从而判断是否处于GCC for windows的构建环境。
    1.1 或许可以通过这部分内容来了解QGIS build的更多内容。
2、GCC for windows的需求
    2.1 兼容性与平台一致性：一些项目希望在所有平台上尽可能使用统一的构建流程，而 MinGW 作为 GCC 在 Windows 上的实现，可以与 Linux 上的 GCC 保持更一致的编译行为。
    2.2 开源与跨平台：使用 GCC（MinGW）可以保持工具链的开源性和跨平台一致性。有些开发者或 CI 系统可能偏好使用 GCC 而非 Visual Studio 的 MSVC。
3、特定问题修复
    3.1 从注释来看，这个宏定义 `_WIN32_WINNT=${GGML_WIN_VER}` 的添加是为了解决一个具体问题（见对应的 GitHub Actions 任务链接），可能是为了确保 MinGW 编译器正确识别或启用某些 Windows API 的特性，而 MSVC 默认可能已经处理得较好，因此只需要在 MinGW 下补充这个定义。
4、MinGW编译器生成的对象
    MinGW 编译器生成的动态库（DLL）和可执行文件（EXE）都是针对 Windows 平台的，只要构建过程正确，并满足 Windows 的运行时依赖，它们就可以在 Windows 上正常运行。
5、add_compile_definitions解释
    5.1 该命令是 CMake 内置的命令，用于在编译时添加预处理器宏定义。它可以全局或在特定范围内设置宏，类似于在编译器命令行中添加 -D 参数，从而使这些宏在源代码中可用。
]]

set(TARGET_SRCS
    server.cpp
    utils.hpp
    httplib.h
)
#[[
Notes:杨小兵-2025-02-14

1、使用cmake中的内置命令set来创建一个自定义变量，该变量用来保存当前文件所在目录中的指定文件，同C/C++相关的源文件。
]]

set(PUBLIC_ASSETS
    index.html.gz
    loading.html
)
#[[
Notes:杨小兵-2025-02-14

1、使用cmake中的内置命令set来创建一个自定义变量，该变量用来保存当前文件所在目录中的指定文件，同web ui相关的源文件。
]]

foreach(asset ${PUBLIC_ASSETS})
    set(input "${CMAKE_CURRENT_SOURCE_DIR}/public/${asset}")
    set(output "${CMAKE_CURRENT_BINARY_DIR}/${asset}.hpp")
    list(APPEND TARGET_SRCS ${output})
    add_custom_command(
        DEPENDS "${input}"
        OUTPUT "${output}"
        COMMAND "${CMAKE_COMMAND}" "-DINPUT=${input}" "-DOUTPUT=${output}" -P "${PROJECT_SOURCE_DIR}/scripts/xxd.cmake"
    )
    set_source_files_properties(${output} PROPERTIES GENERATED TRUE)
endforeach()
#[[
Notes:杨小兵-2025-02-14

1、使用cmake中的内置循环命令foreach分别来处理每个web ui相关的源文件。
2、通过两条cmake内置set命令来设置输入、输出变量。
3、通过cmake中的内置list命令将生成的文件添加到之前创建的变量。
4、xxd.cmake是cmake中将会使用的一个脚本文件，该脚本文件的主要作用就是将对应的文件转化成C/C++可以使用的头文件。
    4.1 这里的需要对转化前、转化后进行对比分析，目前还不能知道前后对应关系。
]]

add_executable(${TARGET} ${TARGET_SRCS})
#[[
Notes:杨小兵-2025-02-14

1、使用cmake中内置的命令创建由TRAGET指定的目标对象，该目标对象将会用到由TARGET_SRCS指定的“资料”。
]]

install(TARGETS ${TARGET} RUNTIME)
#[[
Notes:杨小兵-2025-02-14

1、CMake 中的 install 命令用于定义安装规则，将编译成功后的目标（如可执行文件、库文件）或其他文件复制到指定的目录中，便于后续的部署或打包。
2、该命令是在构建成功后将生成的文件拷贝到目标安装目录中，从而形成最终的可分发版本。
3、install 命令的参数解析
    3.1 TARGETS：这一参数指定了要安装的目标，通常是通过 add_executable() 或 add_library() 定义的编译目标。
    3.2 ${TARGET}：这是一个变量，代表一个或多个具体的目标名称，你需要预先定义好这些目标。
    3.3 RUNTIME：这个参数指定了安装类型为运行时文件（例如可执行文件）。在不同平台上，CMake 会将不同类型的目标安装到不同的目录：
        3.3.1 RUNTIME：通常用于可执行文件，在 Windows 下这些文件会被放在 bin 文件夹中；
        3.3.2 LIBRARY：用于共享库；
        3.3.3 ARCHIVE：用于静态库。
    3.4 除了这些参数外，install 命令还可以配合 DESTINATION 参数指定目标安装目录（例如 bin、lib、include 等），以及 FILES 或 DIRECTORY 参数用于安装非目标文件。通过这些参数，install 命令帮助你构建一个完整的安装系统，确保所有必要的文件都被放在正确的位置，方便后续的发布和部署。
]]

target_include_directories(${TARGET} PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(${TARGET} PRIVATE common ${CMAKE_THREAD_LIBS_INIT})
#[[
Notes:杨小兵-2025-02-14

1、通过cmake中的内置target_include_directories命令设置目标对象头文件的搜索路径。
    1.1 C/C++中基本上都会使用到头文件，在包含头文件的时候，编译器需要知道头文件的具体位于哪里，所以需要通过这种方式来设置头文件的搜索路径。
2、通过cmake中的内置target_link_directories命令设置目标链接对象的搜索路径。
    2.1 C/C++中单个项目如果用到其他的模块，在包含链接对象的时候，编译器需要知道链接对象的具体位于哪里，所以需要通过这种方式来设置链接对象的搜索路径。
3、对比
    3.1 include_directories和target_include_directories
        3.1.1 include_directories：该命令会为当前目录及其子目录中的所有目标设置头文件搜索路径，即全局性的设置。
        3.1.2 target_include_directories：该命令则只针对指定的目标设置头文件搜索路径，并且还可以通过指定 PRIVATE、PUBLIC 或 INTERFACE 来控制传播范围和使用场景，从而实现更细粒度的控制。
    3.2 link_libraries 与 target_link_libraries
        3.2.1 link_libraries：该命令是全局性的，会影响当前目录及其子目录中的所有目标的链接设置。
        3.2.2 target_link_libraries：则仅针对指定的目标进行链接库的设置，并能明确指定链接属性（例如 PRIVATE、PUBLIC、INTERFACE），从而精确控制依赖关系和链接传播。
]]

if (LLAMA_SERVER_SSL)
    find_package(OpenSSL REQUIRED)
    target_link_libraries(${TARGET} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(${TARGET} PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
endif()
#[[
Notes:杨小兵-2025-02-14

1、使用cmake中内置的条件判断LLAMA_SERVER_SSL变量是否有效，如果有效的话意味着llama-server需要支持SSL，那么需要对SSL相关内容进行设置。
]]

if (WIN32)
    TARGET_LINK_LIBRARIES(${TARGET} PRIVATE ws2_32)
endif()
#[[
Notes:杨小兵-2025-02-14

1、使用cmake中内置的条件判断当前编译环境是否为windows环境（这里不区分64 bits/32 bits，统一指的是windows环境）
2、如果在windows构建环境下的话需要设置指定目标对象的链接对象，即ws2_32。
]]

target_compile_features(${TARGET} PRIVATE cxx_std_17)
#[[
Notes:杨小兵-2025-02-16

1、使用cmake中内置的命令设置指定目标对象的C++标准为17。
2、用于声明目标所需的语言特性，主要是针对编译器阶段。
3、该命令的目的在于确保编译器支持指定的语言标准或特性（例如 C++17），并自动添加相应的编译选项（如 -std=c++17 或 /std:c++17）。因此，target_compile_features 仅仅作用于编译器阶段，不用于直接控制预处理器、汇编器或链接器的行为。
]]
