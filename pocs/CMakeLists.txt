# dependencies
#[[
```cmake
find_package(Threads REQUIRED)
```
- **作用**：
  - `find_package`：这是CMake中用于查找并加载外部项目或库的标准模块。它尝试在系统中找到指定的库或工具包，并提供接口以便在项目中使用这些资源。
  - `Threads`：指定要查找的包是线程库，这通常指的是系统原生的线程API，如POSIX线程（pthread）。
  - `REQUIRED`：这表明该包是必需的，如果CMake未能找到它，将会停止配置过程并报错。
]]
find_package(Threads REQUIRED)









# third-party
#[[
```cmake
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
```
- **作用**：
  - `include_directories`：这个命令用于向项目添加头文件搜索路径，类似于编译器的 `-I` 或 `/I` 选项。
  - `${CMAKE_CURRENT_SOURCE_DIR}`：这是一个CMake变量，代表当前处理的CMakeLists.txt文件所在的目录。
这意味着CMake将包含当前源代码目录作为编译时的包含目录，方便编译器找到项目内的头文件。

]]
include_directories(${CMAKE_CURRENT_SOURCE_DIR})



#[[
    `EMSCRIPTEN`：这是一个可能在CMake中设置的变量，用于判断是否使用Emscripten编译环境。Emscripten是一个可以将C/C++代码编译成JavaScript的工具，
常用于将C/C++项目移植到Web平台。表示如果`EMSCRIPTEN`变量不是真（即不在Emscripten环境下编译），则执行`else()`内部的命令。
    `add_subdirectory`：这个命令用于向当前项目添加指定的子目录。CMake会在这个子目录中查找并处理CMakeLists.txt文件，这是用来组织大型项目，将其分
解为更小、更管理的部分的常用方法。
]]
if (EMSCRIPTEN)
else()
    add_subdirectory(vdot)
endif()
