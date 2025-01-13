# 设置目标名称为 "llama-server"
set(TARGET llama-server)
# 定义一个选项 LLAMA_SERVER_SSL，用于决定是否为服务器构建 SSL 支持，默认值为 OFF
option(LLAMA_SERVER_SSL "Build SSL support for the server" OFF)
# 添加当前源目录和当前二进制目录到包含目录中
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
# 如果在 MinGW 环境下进行构建
if (MINGW)
    # fix: https://github.com/ggerganov/llama.cpp/actions/runs/9651004652/job/26617901362?pr=8006
    # 添加编译定义 _WIN32_WINNT，值为 GGML_WIN_VER 变量的值
    add_compile_definitions(_WIN32_WINNT=${GGML_WIN_VER})
endif()
# 设置目标源文件列表，包括 server.cpp、utils.hpp 和 httplib.h
set(TARGET_SRCS
    server.cpp
    utils.hpp
    httplib.h
)
# 定义公共资源文件列表，包括压缩的 HTML 文件
set(PUBLIC_ASSETS
    index.html.gz
    loading.html
)
# 遍历每个公共资源文件，生成相应的 C++ 头文件
foreach(asset ${PUBLIC_ASSETS})
    # 输入文件路径，位于源代码的 public 目录下
    set(input "${CMAKE_CURRENT_SOURCE_DIR}/public/${asset}")
    # 输出文件路径，位于二进制目录下，文件扩展名为 .hpp
    set(output "${CMAKE_CURRENT_BINARY_DIR}/${asset}.hpp")
    # 将生成的头文件添加到目标源文件列表中
    list(APPEND TARGET_SRCS ${output})
    # 添加自定义命令（path_to_llama.cpp_project/llama.cpp/scripts/xxd.cmake），生成头文件
    add_custom_command(
        DEPENDS "${input}"  # 依赖于输入文件
        OUTPUT "${output}"  # 输出生成的头文件
        COMMAND "${CMAKE_COMMAND}" "-DINPUT=${input}" "-DOUTPUT=${output}" -P "${PROJECT_SOURCE_DIR}/scripts/xxd.cmake" # 执行 CMake 脚本来生成头文件
    )
    # 设置生成的头文件属性为 GENERATED，表明这是由 CMake 生成的文件
    set_source_files_properties(${output} PROPERTIES GENERATED TRUE)
endforeach()
# 创建一个可执行文件目标，包含所有源文件
add_executable(${TARGET} ${TARGET_SRCS})
# 安装目标的运行时文件
install(TARGETS ${TARGET} RUNTIME)
# 将项目的源目录路径（${CMAKE_SOURCE_DIR}）添加到指定目标（${TARGET}）的私有包含路径中，确保目标在编译时可以访问该路径下的头文件。
target_include_directories(${TARGET} PRIVATE ${CMAKE_SOURCE_DIR})
# 将 "common" 库和线程库链接到目标上，使用 PRIVATE 作用域
target_link_libraries(${TARGET} PRIVATE common ${CMAKE_THREAD_LIBS_INIT})
# 如果启用了 SSL 支持
if (LLAMA_SERVER_SSL)
    # 查找 OpenSSL 包，并确保其为必需的
    find_package(OpenSSL REQUIRED)
    # 将 OpenSSL 的 SSL 和 Crypto 库链接到目标上，使用 PRIVATE 作用域
    target_link_libraries(${TARGET} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    # 为目标添加编译定义 CPPHTTPLIB_OPENSSL_SUPPORT，以启用 SSL 支持
    target_compile_definitions(${TARGET} PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
endif()
# 如果在 Windows 平台上进行构建
if (WIN32)
    # 将 Windows Sockets 2 库 (ws2_32) 链接到目标上，使用 PRIVATE 作用域
    TARGET_LINK_LIBRARIES(${TARGET} PRIVATE ws2_32)
endif()
# 设置目标所需的 C++ 标准为 C++17
target_compile_features(${TARGET} PRIVATE cxx_std_17)
