#[[
  每行命令都用于设置和配置两个不同的可执行文件的构建。这些命令确保了每个目标（executable）能正确编译并链接必要的库：

### 针对 `llama-vdot` 的设置

1. **设置目标名称**
   ```cmake
   set(TARGET llama-vdot)
   ```
   - `set` 命令用于定义一个变量。这里定义了变量 `TARGET`，并将其值设置为 `llama-vdot`。这个变量用于后续命令，避免重复书写字符串，提高代码的可维护性。

2. **添加可执行文件**
   ```cmake
   add_executable(${TARGET} vdot.cpp)
   ```
   - `add_executable` 用于创建一个可执行文件。`${TARGET}` 是前面设置的变量，代表可执行文件的名称，`vdot.cpp` 是这个可执行文件的源文件。

3. **链接库**
   ```cmake
   target_link_libraries(${TARGET} PRIVATE common llama ${CMAKE_THREAD_LIBS_INIT})
   ```
   - `target_link_libraries` 命令用于指定目标（这里是`${TARGET}`，即`llama-vdot`）需要链接的库。
   - `PRIVATE` 指定了链接类型，表示这些库在此目标中被私有链接，不会影响其他链接此目标的库或可执行文件。
   - `common` 和 `llama` 是目标需要链接的自定义库。
   - `${CMAKE_THREAD_LIBS_INIT}` 是一个由 `find_package(Threads REQUIRED)` 设置的变量，包含了必要的线程库。

4. **设置编译特性**
   ```cmake
   target_compile_features(${TARGET} PRIVATE cxx_std_11)
   ```
   - `target_compile_features` 命令用于为目标指定必须的编译器特性，这里是C++11标准。
   - `PRIVATE` 表示这些编译器特性仅适用于指定目标。

### 针对 `llama-q8dot` 的设置
1. **设置目标名称**
   ```cmake
   set(TARGET llama-q8dot)
   ```
   - 类似于上面，但是这次将目标名称设置为 `llama-q8dot`。

2. **添加可执行文件**
   ```cmake
   add_executable(${TARGET} q8dot.cpp)
   ```
   - 创建名为 `llama-q8dot` 的可执行文件，源文件是 `q8dot.cpp`。

3. **链接库**
   ```cmake
   target_link_libraries(${TARGET} PRIVATE common llama ${CMAKE_THREAD_LIBS_INIT})
   ```
   - 与 `llama-vdot` 相同，指定 `llama-q8dot` 需要链接的库。

4. **设置编译特性**
   ```cmake
   target_compile_features(${TARGET} PRIVATE cxx_std_11)
   ```
   - 为 `llama-q8dot` 设置编译器必须支持C++11标准。

### 总结

  这段CMake脚本展示了如何为两个不同的可执行文件配置基本的构建参数，包括如何定义目标名、添加源文件、链接必要的库，以及设置所需的C++标准。
使用这样的设置可以确保软件在不同平台上的可构建性和可移植性。
]]
set(TARGET llama-vdot)
add_executable(${TARGET} vdot.cpp)
target_link_libraries(${TARGET} PRIVATE common llama ${CMAKE_THREAD_LIBS_INIT})
target_compile_features(${TARGET} PRIVATE cxx_std_11)

set(TARGET llama-q8dot)
add_executable(${TARGET} q8dot.cpp)
target_link_libraries(${TARGET} PRIVATE common llama ${CMAKE_THREAD_LIBS_INIT})
target_compile_features(${TARGET} PRIVATE cxx_std_11)
