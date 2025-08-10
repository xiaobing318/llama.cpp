set(TARGET llama-agent)

# 包含项目生成目录是为了包含前端代码转化后的头文件
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/common
    )

add_executable(${TARGET} agent.cpp agent_utils.cpp tool_executor.cpp)

target_link_libraries(${TARGET} PRIVATE
    common
    llama
    ${CMAKE_THREAD_LIBS_INIT}
)

target_compile_features(${TARGET} PRIVATE cxx_std_17)

# Platform specific settings
if (WIN32)
    target_link_libraries(${TARGET} PRIVATE ws2_32 iphlpapi)
    if (BUILD_SHARED_LIBS)
        set_target_properties(${TARGET} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
    endif()
endif()

# Find OpenSSL if LLAMA_SERVER_SSL is available
if (LLAMA_SERVER_SSL)
    find_package(OpenSSL REQUIRED)
    target_link_libraries(${TARGET} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(${TARGET} PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
endif()

# Set installation directory
install(TARGETS ${TARGET} RUNTIME)
