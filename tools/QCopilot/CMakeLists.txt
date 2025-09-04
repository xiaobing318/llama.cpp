#[[
1、设置当前目标的名称为 qcopilot 。
2、这个目标名称将会显示在 CMake 的构建输出中，并且可以在其他地方引用。
]]
set(TARGET qcopilot)

#[[
1、定义目标的源文件列表和公共资源文件列表。
2、PUBLIC_ASSETS 包含需要嵌入到可执行文件中的前端资源文件。
]]
set(TARGET_SRCS
    qcopilot.cpp
    qcopilot_utils.cpp
    qcopilot_executor.cpp
    qcopilot_builtin_tools.cpp
)
set(PUBLIC_ASSETS
    index.html.gz
    # 目前没有用到这个 HTML 文件，但是在 llama-server 中使用到了即在 llama-server 还在启动后端模型的时候前端需要加载提示信息。
    loading.html
)

#[[
1、为每个公共资源文件生成对应的 C++ 头文件。
2、使用 xxd.cmake 脚本将二进制文件转换为 C++ 数组，这样可以将前端资源嵌入到可执行文件中。
3、生成的头文件会被自动添加到编译单元列表中。
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
1、该目标包含了以下编译单元：
   - qcopilot.cpp: 主程序入口和HTTP服务器
   - qcopilot_utils.cpp: 通用工具函数库
   - qcopilot_executor.cpp: 工具执行器核心逻辑
   - qcopilot_builtin_tools.cpp: 内置工具实现（文件操作、数学计算、系统命令等）
   - 生成的前端资源头文件
2、编译单元指的是源代码中的一个独立的文件，它可以被单独编译并生成一个目标文件，可以包含头文件、源文件和其他资源。
]]
add_executable(${TARGET} ${TARGET_SRCS})

#[[
1、包含当前源目录是为了让编译器能够找到当前目录下的头文件，因为该目标中的编译单元需要该目录下的头文件。
2、包含当前二进制目录是为了让编译器能够找到当前二进制目录下的头文件，因为该目标中的编译单元需要该二进制目录下的头文件，即前端代码头文件。
3、包含通用头文件目录是为了让编译器能够找到项目中共享的头文件，这些头文件可能在多个目标中使用，目前使用的是 nlohmann/json 库。
4、在指定包含头文件目录的时候需要明确说明针对哪一个目标进行的参数。
]]
target_include_directories(${TARGET} PRIVATE
    # 向编译系统传递该头文件查找路径是为了能够包含 qcopilot_utils.h/qcopilot_executor.h 头文件
    ${CMAKE_CURRENT_SOURCE_DIR}
    # 向编译系统传递该头文件查找路径是为了能够包含 index.html.gz.hpp/loading.html.hpp 头文件
    ${CMAKE_CURRENT_BINARY_DIR}
    # 向编译系统传递该头文件查找路径是为了能够包含 json.hpp/json_fwd.hpp 头文件
    ${CMAKE_SOURCE_DIR}/vendor/nlohmann
    # 向编译系统传递该头文件查找路径是为了能够包含 httplib.h 头文件
    ${CMAKE_SOURCE_DIR}/vendor/cpp-httplib
    )

#[[
1、该目标使用到了 common、llama 和 ${CMAKE_THREAD_LIBS_INIT} 这几个库，因此需要通过链接的方式将其包含进来。
2、通过链接这些库，可以使得该目标能够使用这些库中定义的函数和数据结构，从而实现更复杂的功能，这里采用私有的链接
方式使得这些库的实现细节对其他目标是不可见的。
]]
target_link_libraries(${TARGET} PRIVATE
    common
    llama
    ${CMAKE_THREAD_LIBS_INIT}
)

#[[
1、该目标使用了 C++17 标准，因此需要设置编译特性。
]]
target_compile_features(${TARGET} PRIVATE cxx_std_17)

#[[
1、针对特定平台的设置，即不同的平台有着不同的设置。
2、在windows平台上，需要链接 ws2_32 和 iphlpapi 库，这些库提供了网络编程和 IP 地址解析的功能。
]]
if (WIN32)
    target_link_libraries(${TARGET} PRIVATE ws2_32 iphlpapi)
    if (BUILD_SHARED_LIBS)
        set_target_properties(${TARGET} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
    endif()
endif()

#[[
1、如果 LLAMA_SERVER_SSL 被定义为真，则表示需要使用 SSL/TLS 加密。
2、在这种情况下，需要找到并链接 OpenSSL 库，以便在代码中使用 SSL/TLS 功能。
]]
if (LLAMA_SERVER_SSL)
    find_package(OpenSSL REQUIRED)
    target_link_libraries(${TARGET} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(${TARGET} PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
endif()

#[[
1、设置安装目录。
]]
install(TARGETS ${TARGET} RUNTIME)
