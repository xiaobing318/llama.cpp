#[[
1、set(TARGET llama-server)
    设置一个变量TARGET，这个变量的值为llama-server
2、option(LLAMA_SERVER_VERBOSE "Build verbose logging option for Server" ON)
    设置一个名为LLAMA_SERVER_VERBOSE的bool类型的值，注释内容是为server构建详细的log信息，默认是开启状态
3、option(LLAMA_SERVER_SSL     "Build SSL support for the server"        OFF)
    设置一个名为LLAMA_SERVER_SSL的bool类型的值，注释内容是为server构建SSL支持，默认是开启状态
]]
set(TARGET llama-server)
option(LLAMA_SERVER_VERBOSE "Build verbose logging option for Server" ON)
option(LLAMA_SERVER_SSL     "Build SSL support for the server"        OFF)



#[[
1、include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
    将${CMAKE_CURRENT_SOURCE_DIR}和${CMAKE_CURRENT_BINARY_DIR}两个路径添加到当前target的头文件搜索路径中，以便在build的时候能够找到所需的头文件
]]
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})




#[[
1、if (MINGW)
    MINGW这个变量是在CMake中内置的一个变量，如果cmake检测到当前的build环境是mingw，那么MINGW这个变量将会被cmake设置为TRUE
2、# fix: https://github.com/ggerganov/llama.cpp/actions/runs/9651004652/job/26617901362?pr=8006
    通过一个PR修复了存在的问题
3、add_compile_definitions(_WIN32_WINNT=${GGML_WIN_VER})
    对当前的target添加编译定义：_WIN32_WINNT（这个变量的值设置为GGML_WIN_VER的值）
4、endif()
    cmake中条件编译的结束
]]
if (MINGW)
    # fix: https://github.com/ggerganov/llama.cpp/actions/runs/9651004652/job/26617901362?pr=8006
    add_compile_definitions(_WIN32_WINNT=${GGML_WIN_VER})
endif()



#[[
1、set(TARGET_SRCS server.cpp utils.hpp httplib.h)
    设置一个名为TARGET_SRCS的变量，将server.cpp utils.hpp httplib.h添加到TARGET_SRCS变量中，TARGET_SRCS变量相当于是一个字符串列表
]]
set(TARGET_SRCS
    server.cpp
    utils.hpp
    httplib.h
)




#[[
1、set(PUBLIC_ASSETS ...)
    设置一个名为PUBLIC_ASSETS的变量，将一些*.css、*.html、*.js、*.mjs添加到PUBLIC_ASSETS变量中，PUBLIC_ASSETS变量是一个字符串列表
]]
set(PUBLIC_ASSETS
    colorthemes.css
    style.css
    theme-beeninorder.css
    theme-ketivah.css
    theme-mangotango.css
    theme-playground.css
    theme-polarnight.css
    theme-snowstorm.css
    index.html
    index-new.html
    index.js
    completion.js
    system-prompts.js
    prompt-formats.js
    json-schema-to-grammar.mjs
)



#[[
    这部分cmake指令主要用于处理一组公共资源文件，并将每个文件转换成 C++ 头文件。

1. `foreach(asset ${PUBLIC_ASSETS})` 至 `endforeach()`：
   - 这是一个循环结构，它会遍历 `${PUBLIC_ASSETS}` 变量中列出的每个资产（资源文件名）。每个循环迭代都将对当前的 `asset` 执行内部的 CMake 命令。

2. `set(input "${CMAKE_CURRENT_SOURCE_DIR}/public/${asset}")`：
   - 这条指令设置变量 `input`，其值为当前源代码目录下的 `public` 子目录中的 `asset` 文件的完整路径。这表示每个资源文件的源位置。

3. `set(output "${CMAKE_CURRENT_BINARY_DIR}/${asset}.hpp")`：
   - 这条指令设置变量 `output`，其值为构建目录中的 `asset` 文件名，后缀改为 `.hpp`。这是资源文件将被转换成的 C++ 头文件的目标路径。

4. `list(APPEND TARGET_SRCS ${output})`：
   - 这条指令将每次循环生成的 `output` 文件路径添加到 `TARGET_SRCS` 列表中。这个列表可能用于其他 CMake 指令，以跟踪所有生成的头文件。

5. `add_custom_command(...)`：
   - 这是一个添加自定义命令的指令，用于在构建时执行一个特定的命令。它依赖于 `input` 文件，并产生 `output` 文件作为输出。具体命令调用了 CMake 
自身 (`"${CMAKE_COMMAND}"`)，传入了 `INPUT` 和 `OUTPUT` 定义，以及一个指向脚本 `xxd.cmake` 的路径。这个脚本位于项目的 `scripts` 目录下，它
负责将输入文件转换成一个 C++ 头文件格式。

6. `-P "${PROJECT_SOURCE_DIR}/scripts/xxd.cmake"`：
   - 这个参数指定了 CMake 在处理自定义命令时应该执行的脚本。`-P` 选项告诉 CMake 处理一个给定的 CMake 脚本文件。在这种情况下，脚本用于执行实际的
文件转换逻辑。

    总结，这段 CMake 脚本通过一个循环处理一组资产文件，将每个文件转换为对应的 C++ 头文件，并将生成的头文件路径存储起来，可能用于后续的构建步骤或
其他依赖。这样的处理方式在需要将资源嵌入到 C++ 应用程序中时非常有用，例如在游戏开发或嵌入式系统开发中包含图像、着色器代码等资源。
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
endforeach()



#[[
1、add_executable(${TARGET} ${TARGET_SRCS})
    通过add_executable命令创建一个项目，这个项目名为TARGET变量指定的名称，项目所使用到的文件为TARGET_SRCS变量指定的文件，需要注意的是TARGET_SRCS
变量内是已经经过修改后的文件列表
2、install(TARGETS ${TARGET} RUNTIME)
    指定将build后的target安装在指定的位置中
3、target_compile_definitions(${TARGET} PRIVATE SERVER_VERBOSE=$<BOOL:${LLAMA_SERVER_VERBOSE}>)
    向TARGET项目添加编译器选项，通过PRIVATE指定这个编译器选项只对当前target是有效的
]]
add_executable(${TARGET} ${TARGET_SRCS})
install(TARGETS ${TARGET} RUNTIME)
target_compile_definitions(${TARGET} PRIVATE
    SERVER_VERBOSE=$<BOOL:${LLAMA_SERVER_VERBOSE}>
)



#[[
1、target_link_libraries(${TARGET} PRIVATE common ${CMAKE_THREAD_LIBS_INIT})
    向当前TARGET目标添加需要链接的对象（common,CMAKE_THREAD_LIBS_INIT指定的链接对象），并且通过PRIVATE指定链接对象只对当前target是有效的
]]
target_link_libraries(${TARGET} PRIVATE common ${CMAKE_THREAD_LIBS_INIT})



#[[
1、if (LLAMA_SERVER_SSL)
    这是cmake中的条件判断语句，LLAMA_SERVER_SSL是一个bool类型的值，如果LLAMA_SERVER_SSL是TRUE那么指定其中内部的代码，反之则不执行。
2、find_package(OpenSSL REQUIRED)
    通过cmake中的find_package命令找到OpenSSL库，这个库是必须的，如果没有找到这个库，那么cmake配置过程将会中止（通过REQUIRED来实现）
3、target_link_libraries(${TARGET} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    向当前target目标指定link对象（OpenSSL::SSL OpenSSL::Crypto），通过PRIVATE选项指定这些link对象只对当前target是有效的
4、target_compile_definitions(${TARGET} PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
    向当前target目标添加编译定义，并且通过PRIVATE指定这个编译定义只对当前target有用，不对其他的target有用
5、endif()
    cmake中条件判断的结尾
]]
if (LLAMA_SERVER_SSL)
    find_package(OpenSSL REQUIRED)
    target_link_libraries(${TARGET} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(${TARGET} PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
endif()



#[[
1、if (WIN32)
    cmake中条件判断指令，其中WIN32是cmake中的一个内置命令，如果cmake检测到当前的编译环境是windows，那么则执行内部指定，反之则不执行
2、TARGET_LINK_LIBRARIES(${TARGET} PRIVATE ws2_32)
    向当前target添加link对象ws2_32，并且通过PRIVATE指定link对象只对当前target是有效的
3、endif()
    cmake中条件判断的结束
]]
if (WIN32)
    TARGET_LINK_LIBRARIES(${TARGET} PRIVATE ws2_32)
endif()


#[[
1、target_compile_features(${TARGET} PRIVATE cxx_std_11)
    向当前target添加编译特性，并且通过PRIVATE指定当前compile features只对当前target有效
]]
target_compile_features(${TARGET} PRIVATE cxx_std_11)
