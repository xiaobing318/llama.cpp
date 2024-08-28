#[[
    这段 CMake 脚本的作用是在构建 Windows 平台上的共享库（动态链接库，DLL）时，自动导出所有符号。

1. **条件判断 `if (WIN32)`**: 这一行判断当前是否在 Windows 系统上编译。`WIN32` 是 CMake 在检测到 Windows 系统时自动设置的一个变量。

2. **嵌套条件 `if (BUILD_SHARED_LIBS)`**: 这一行进一步检查是否定义了 `BUILD_SHARED_LIBS` 变量且其值为真。`BUILD_SHARED_LIBS` 是一个常用的 
CMake 变量，用于指示是否构建共享库（动态库）而不是静态库。

3. **设置 `CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS`**: 当上述两个条件都满足时，即在 Windows 平台上构建共享库时，这行代码设置 
`CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS` 变量为 `ON`。此设置是 CMake 特有的，它告诉 CMake 自动生成导出所有符号的代码，这是在 
Windows 平台上创建 DLL 所必需的，因为 Windows 的 DLL 需要明确指定哪些符号（函数、变量等）是可被其他应用或库访问的。

    这段代码通常用于简化 DLL 的创建过程，避免手动在代码中使用 `__declspec(dllexport)` 宏来标记导出的符号。通过这种方式，开发者可以更容易地维护和
构建跨平台的库。
]]
# TODO: should not use this
if (WIN32)
    if (BUILD_SHARED_LIBS)
        set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    endif()
endif()












#[[
这段 CMake 脚本的作用主要是定义和配置一个名为 `llama` 的库项目。

1. **`add_library(llama ...)`**: 这一行声明了一个新的库 `llama`，并指定了构成这个库的源文件。这些文件包括头文件 `../include/llama.h` 和
多个 `.cpp` 文件，这些 `.cpp` 文件实现了库的各种功能。

2. **`target_include_directories(llama PUBLIC . ../include)`**: 这行代码为 `llama` 库设置了公共的包含目录，即当前目录（`.`）和上级目录下的
 `include` 文件夹（`../include`）。`PUBLIC` 关键字指明这些目录在库被其他目标链接时也会被使用。

3. **`target_compile_features(llama PUBLIC cxx_std_11)`**: 设置 `llama` 库需要的 C++ 标准为 C++11，并且这一要求也适用于链接了 `llama` 的目标。
`PUBLIC` 表示这一编译特性被传递给任何依赖此库的其他目标。

4. **`target_link_libraries(llama PUBLIC ggml)`**: 指示 CMake 将 `ggml` 库链接到 `llama` 库。使用 `PUBLIC` 关键字表示 `ggml` 库也将自动链接
到任何链接了 `llama` 的目标。

5. **`if (BUILD_SHARED_LIBS)`**: 这个条件检查是否定义了 `BUILD_SHARED_LIBS` 变量且其值为真，通常这意味着构建设置是生成动态库而非静态库。
   - **`set_target_properties(llama PROPERTIES POSITION_INDEPENDENT_CODE ON)`**: 当构建共享库时，这行代码设置 `llama` 库编译成位置无关代码
   （Position Independent Code, PIC），这是在许多平台上生成动态库所必需的。
   - **`target_compile_definitions(llama PRIVATE LLAMA_SHARED LLAMA_BUILD)`**: 为 `llama` 库添加编译时定义，这些定义仅在 `llama` 库内部可
   见（`PRIVATE`）。`LLAMA_SHARED` 和 `LLAMA_BUILD` 可能用于条件编译，以区分库是作为共享库还是静态库进行构建，以及是否处于构建过程中。

    总结来说，这段 CMake 脚本为 `llama` 库的构建提供了详尽的配置，包括文件列表、依赖库、编译特性、包含目录和特定于构建类型的设置。这样的配置确保了 
`llama` 库能够在不同的平台和条件下正确地编译和链接。
]]
#
# libraries
#

# llama

add_library(llama
            ../include/llama.h
            llama.cpp
            llama-vocab.cpp
            llama-grammar.cpp
            llama-sampling.cpp
            unicode.h
            unicode.cpp
            unicode-data.cpp
            )

target_include_directories(llama PUBLIC . ../include)
target_compile_features   (llama PUBLIC cxx_std_11) # don't bump

target_link_libraries(llama PUBLIC ggml)

if (BUILD_SHARED_LIBS)
    set_target_properties(llama PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_compile_definitions(llama PRIVATE LLAMA_SHARED LLAMA_BUILD)
endif()
