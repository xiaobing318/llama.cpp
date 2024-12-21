# 杨小兵-2024-12-21：设置TARGET的名称为llama-simple，这个命令将会生成相应名称的项目
set(TARGET llama-simple)
# 杨小兵-2024-12-21：llama-simple这个项目将会由{simple.cpp}构成
add_executable(${TARGET} simple.cpp)
#[[
杨小兵-2024-12-21：将llama-simple项目添加到安装目标中
    1. install(TARGETS ${TARGET} RUNTIME)是一个CMake的命令，用于将TARGET添加到安装目标中
    2. ${TARGET}是一个变量，表示当前的项目名称
    3. RUNTIME表示TARGET是一个可执行文件
    4. 该命令将会在安装目标中添加一个可执行文件
    5. 安装的位置是由CMAKE_INSTALL_PREFIX指定的，如果没有指定，则默认为/usr/local
]]
install(TARGETS ${TARGET} RUNTIME)
#[[
杨小兵-2024-12-21：链接llama库和线程库到llama-simple项目中
    1. target_link_libraries(${TARGET} PRIVATE llama ${CMAKE_THREAD_LIBS_INIT})是一个CMake的命令，用于将llama库和线程库链接到TARGET中
    2. ${TARGET}是一个变量，表示当前的项目名称
    3. PRIVATE表示将llama库和线程库链接到TARGET中，并且不会将llama库和线程库链接到TARGET的依赖中
    4. llama表示llama库
    5. ${CMAKE_THREAD_LIBS_INIT}表示线程库，CMAKE_THREAD_LIBS_INIT是一个CMake的变量，表示线程库
    6. 该命令将会将llama库和线程库链接到TARGET中
]]
target_link_libraries(${TARGET} PRIVATE llama ${CMAKE_THREAD_LIBS_INIT})
#[[
杨小兵-2024-12-21：设置TARGET的编译特性为C++17
    1. target_compile_features(${TARGET} PRIVATE cxx_std_17)是一个CMake的命令，用于设置TARGET的编译特性为C++17
    2. ${TARGET}是一个变量，表示当前的项目名称
    3. PRIVATE表示将C++17编译特性设置到TARGET中，并且不会将C++17编译特性设置到TARGET的依赖中
    4. cxx_std_17表示C++17编译特性
    5. 该命令将会将C++17编译特性设置到TARGET中
]]
target_compile_features(${TARGET} PRIVATE cxx_std_17)
