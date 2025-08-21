# 设置目标可执行文件的名称为 llama-server
set(TARGET llama-server)

# 定义一个选项，用来控制是否为服务器构建SSL支持，默认关闭
option(LLAMA_SERVER_SSL "Build SSL support for the server" OFF)

# 添加头文件搜索路径，包含当前源码目录和当前构建目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

# 如果使用MINGW编译器（Windows下的GCC）
if (MINGW)
    # 修复特定的Windows版本兼容性问题
    # fix: https://github.com/ggml-org/llama.cpp/actions/runs/9651004652/job/26617901362?pr=8006
    # 定义Windows NT版本宏，确保兼容性
    add_compile_definitions(_WIN32_WINNT=${GGML_WIN_VER})
endif()

# 定义目标源文件列表，包含主要的C++源码文件
set(TARGET_SRCS
    server.cpp    # 服务器主程序文件
    utils.hpp     # 工具函数头文件
)
# 定义公共资源文件列表，这些是网页界面相关的静态资源
set(PUBLIC_ASSETS
    index.html.gz  # 压缩过的主页HTML文件
    loading.html   # 加载页面HTML文件
)

# 循环处理每个资源文件，将HTML等资源文件转换为C++头文件
foreach(asset ${PUBLIC_ASSETS})
    # 设置输入文件路径（在public目录下的资源文件）
    set(input "${CMAKE_CURRENT_SOURCE_DIR}/public/${asset}")
    # 设置输出文件路径（转换后的.hpp头文件）
    set(output "${CMAKE_CURRENT_BINARY_DIR}/${asset}.hpp")
    # 将生成的头文件添加到源文件列表中
    list(APPEND TARGET_SRCS ${output})
    # 添加自定义命令：使用xxd.cmake脚本将资源文件转换为C++头文件
    add_custom_command(
        DEPENDS "${input}"     # 依赖原始资源文件
        OUTPUT "${output}"     # 输出生成的头文件
        # 调用CMake脚本进行转换，类似于xxd工具的功能
        COMMAND "${CMAKE_COMMAND}" "-DINPUT=${input}" "-DOUTPUT=${output}" -P "${PROJECT_SOURCE_DIR}/scripts/xxd.cmake"
    )
    # 标记输出文件为生成的文件（不是源码仓库中的文件）
    set_source_files_properties(${output} PROPERTIES GENERATED TRUE)
endforeach()

# 创建可执行文件目标，使用上面定义的所有源文件
add_executable(${TARGET} ${TARGET_SRCS})
# 安装规则：将可执行文件安装到系统的运行时目录
install(TARGETS ${TARGET} RUNTIME)

#[[
1、添加私有头文件搜索路径：mtmd目录（用于多模态功能）
2、原始包含的头文件路径是 ../llava 问题是为什么即使路径错误，编译仍然能通过？
    2.1 mtmd库的公开头文件：mtmd库将其自身目录（.）设置为PUBLIC包含目录，这意味着任何链接到mtmd库的目标都会自动获得mtmd目录的头文件搜索路径，即通过 target_include_directories(mtmd PUBLIC  .) 
    命令实现的。
    2.2 链接关系：server目标通过以下命令链接了mtmd库：target_link_libraries(${TARGET} PRIVATE common mtmd ${CMAKE_THREAD_LIBS_INIT})
    2.3 传递性包含：由于mtmd库使用了PUBLIC关键字设置包含目录，这个包含路径会传递给所有链接mtmd的目标，包括server。
    2.4 错误路径被忽略：虽然target_include_directories(${TARGET} PRIVATE ../llava)指向了不存在的目录，但因为mtmd库已经通过传递性依赖提供了正确的头文件路径，编译器能找到所需的头文件。
3、所以答案是：编译能通过是因为mtmd库通过PUBLIC包含目录传递性地提供了头文件路径，使得server即使包含了错误的../llava路径也能找到mtmd的头文件。因此这里包含 mtmd 目录所谓头文件搜索路径或者
不包含都是可以的。
4、TODO-PR：提交一个 PR ，该 PR 已提交成功。
]]
target_include_directories(${TARGET} PRIVATE ../mtmd)
# 添加私有头文件搜索路径：项目根目录
target_include_directories(${TARGET} PRIVATE ${CMAKE_SOURCE_DIR})
# 链接必要的库：common库、mtmd库和线程库
target_link_libraries(${TARGET} PRIVATE common mtmd ${CMAKE_THREAD_LIBS_INIT})

# 如果启用了SSL支持
if (LLAMA_SERVER_SSL)
    # 查找并要求OpenSSL库
    find_package(OpenSSL REQUIRED)
    # 链接OpenSSL的SSL和Crypto库
    target_link_libraries(${TARGET} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    # 定义宏，启用HTTP库的OpenSSL支持
    target_compile_definitions(${TARGET} PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
endif()

# 如果是Windows平台
if (WIN32)
    # 链接Windows套接字库（用于网络通信）
    TARGET_LINK_LIBRARIES(${TARGET} PRIVATE ws2_32)
endif()

# 设置目标使用C++17标准进行编译
target_compile_features(${TARGET} PRIVATE cxx_std_17)
