# dependencies
#[[
### 1. 总结 `find_package` 这个 CMake 命令的作用是什么？

    `find_package` 命令在 CMake 脚本中用于查找并加载外部项目或库的设置。当使用此命令时，CMake 会查找并使用特定库的配置文件，这些配置文件包含了必要的
编译指令、链接选项、头文件路径等信息。这使得开发者可以在自己的项目中方便地使用第三方库或软件包，而不需要手动指定这些库的详细路径或其他复杂设置。

### 2. 详细解释 `find_package` 这个命令的工作原理是什么

`find_package` 命令在 CMake 中的工作流程比较复杂，具体如下：

1. **模块和配置模式**：
   - `find_package` 可以在两种模式下工作：模块模式（Module mode）和配置模式（Config mode）。默认情况下，CMake 首先尝试模块模式，如果失败，再尝试
   配置模式。
   - **模块模式**：在这种模式下，CMake 使用预定义的模块（通常是 CMake 自带的或用户提供的 `Find<Package>.cmake` 模块文件）来查找库。这些模块文件包
   含了如何在系统中查找特定库的指令，并设置相应的变量。
   - **配置模式**：在配置模式下，CMake 直接使用库提供的配置文件（例如 `<Package>Config.cmake` 或 `<package>-config.cmake`），这些文件由库的开发
   者提供，并且包含了库的详细使用信息。这种方式更加健壮，因为库的开发者最了解如何正确配置他们的库。

2. **查找过程**：
   - CMake 在一系列预定义路径中搜索相应的模块或配置文件。这些路径包括系统默认路径、由用户通过 `CMAKE_MODULE_PATH` 变量指定的路径，以及库自身可能
   提供的路径。
   - 对于模块模式，CMake 将加载 `Find<Package>.cmake` 文件，并执行其中的脚本来确定库的位置和设置相关变量。
   - 对于配置模式，CMake 将寻找 `<Package>Config.cmake` 或 `<package>-config.cmake` 文件，并直接加载这些文件来设置库的使用详情。

3. **必需和可选**：
   - `REQUIRED` 选项表示找到指定的包是必须的，如果未找到，CMake 将停止处理并报错。这在确保构建环境正确设置时非常有用。
   - 如果没有 `REQUIRED`，则即使未找到包，CMake 也会继续执行，但可能会设置一些状态变量，例如 `Package_FOUND`，以标记查找是否成功。

在实际使用中，`find_package` 通过简化外部库的查找和使用过程，极大地提升了项目的可移植性和易维护性。
]]
find_package(Threads REQUIRED)

# third-party

# ...

# examples
#[[
    将当前CMakeLists.txt文件所在的路径也添加到到头文件的搜索路径中。
]]
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

#[[
1、如果EMSCRIPTEN变量为TRUE即在webassembly的编译环境下，那么不做任何处理，反之需要将一些子目录添加从而进行递归构建。
]]
if (EMSCRIPTEN)
else()
    add_subdirectory(cvector-generator)
    add_subdirectory(baby-llama)
    add_subdirectory(batched-bench)
    add_subdirectory(batched)
    add_subdirectory(benchmark)
    add_subdirectory(convert-llama2c-to-ggml)
    add_subdirectory(embedding)
    add_subdirectory(eval-callback)
    add_subdirectory(export-lora)
    add_subdirectory(gbnf-validator)
    add_subdirectory(gguf-hash)
    add_subdirectory(gguf-split)
    add_subdirectory(gguf)
    add_subdirectory(gritlm)
    add_subdirectory(imatrix)
    add_subdirectory(infill)
    add_subdirectory(llama-bench)
    add_subdirectory(llava)
    add_subdirectory(lookahead)
    add_subdirectory(lookup)
    add_subdirectory(main)
    add_subdirectory(parallel)
    add_subdirectory(passkey)
    add_subdirectory(perplexity)
    add_subdirectory(quantize-stats)
    add_subdirectory(quantize)
    add_subdirectory(retrieval)
    if (GGML_RPC)
        add_subdirectory(rpc)
    endif()
    if (LLAMA_BUILD_SERVER)
    add_subdirectory(server)
    endif()
    if (GGML_SYCL)
        add_subdirectory(sycl)
    endif()
    add_subdirectory(save-load-state)
    add_subdirectory(simple)
    add_subdirectory(speculative)
    add_subdirectory(tokenize)
endif()
