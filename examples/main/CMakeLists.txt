#[[
1、设置一个名为TARGET的变量，这个变量的值为llama-cli
2、创建一个可执行文件（源文件集合：main.cpp）
3、设置可执行文件的安装位置
4、将可执行文件TARGET同common\llama\pthread进行linking
5、对可执行文件在编译的时候添加编译器选项：cxx_std_11


1. **`set(TARGET llama-cli)`**
   - 这行命令设置了一个变量 `TARGET`，其值为 `llama-cli`。这是一个常见的做法，用于在多个地方引用同一个目标名时保持代码的一致性和可维护性。

2. **`add_executable(${TARGET} main.cpp)`**
   - 此命令创建了一个可执行文件，名称为由 `TARGET` 变量指定的 `llama-cli`，并指定 `main.cpp` 作为源文件。这是编译过程的起点，告诉 CMake 从 
   `main.cpp` 开始构建指定的可执行文件。

3. **`install(TARGETS ${TARGET} RUNTIME)`**
   - 这行命令指定安装步骤，将 `TARGET` （即 `llama-cli`）作为运行时组件安装。它指示 CMake 在安装阶段将生成的可执行文件放在适当的位置（通常是系
   统的二进制目录中）。

4. **`target_link_libraries(${TARGET} PRIVATE common llama ${CMAKE_THREAD_LIBS_INIT})`**
   - 这行命令为目标 `llama-cli` 添加链接库。这里使用 `PRIVATE` 指定链接范围，表明这些库仅在编译此目标时使用，不会影响其他链接到此目标的库。
   `common`、`llama` 以及 CMake 自动确定的线程库 `${CMAKE_THREAD_LIBS_INIT}` 被用作链接依赖。

5. **`target_compile_features(${TARGET} PRIVATE cxx_std_11)`**
   - 此命令为目标 `llama-cli` 指定了编译特性，确保使用 C++11 标准进行编译。通过设置 `PRIVATE`，这个编译特性仅应用于指定目标，不影响其他目标。

  每条命令都是构建、配置和安装 C++ 项目的关键部分，确保了编译流程的明确性和可管理性。这些设置帮助维持项目的模块化和可扩展性，同时确保编译器和链接
  器的配置可以满足项目需求。
]]
set(TARGET llama-cli)
add_executable(${TARGET} main.cpp)
install(TARGETS ${TARGET} RUNTIME)
target_link_libraries(${TARGET} PRIVATE common llama ${CMAKE_THREAD_LIBS_INIT})
target_compile_features(${TARGET} PRIVATE cxx_std_11)
