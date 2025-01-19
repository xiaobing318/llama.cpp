#pragma region "1 包含头文件（减少重复）"
#include "ggml-cpu.h"

#ifdef GGML_USE_CUDA
#include "ggml-cuda.h"
#endif

#ifdef GGML_USE_METAL
#include "ggml-metal.h"
#endif

#ifdef GGML_USE_VULKAN
#include "ggml-vulkan.h"
#endif

#ifdef GGML_USE_SYCL
#include "ggml-sycl.h"
#endif

#include "ggml-rpc.h"

#ifdef _WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#endif

#include <string>
#include <stdio.h>
#pragma endregion

#pragma region "2 自定义结构体"
/*
Note:杨小兵-2025-01-17

1、从 C++11 开始，您可以在 struct（以及 class）中直接对成员进行初始化。这种特性被称为内联成员初始化（in-class member initializers）。
它允许您在定义结构体或类时为成员变量提供默认值，而无需在构造函数中进行初始化。
*/
struct rpc_server_params {
    std::string host        = "127.0.0.1";
    int         port        = 50052;
    size_t      backend_mem = 0;
};
#pragma endregion

#pragma region "3 相关函数"
/*
Note:杨小兵-2025-01-17

1、以static关键词修饰的函数意味着这个被修饰的函数只能在当前编译单元中是可见的，类似于类中的private的作用一样。
2、在不使用参数名称的情况下对参数进行说明，或者在参数名称旁边添加额外的信息以提高代码的可读性，参数 argc 未被使用，同时通过注释保留了参数的名称以便于理解。
3、在函数参数中添加注释一般都是为了解释说明
4、函数说明
    4.1 printf 函数将格式化的输出发送到标准输出（stdout）
    4.2 fprintf 函数允许指定输出流，因此可以将输出发送到任意文件流，例如标准错误（stderr）
*/
static void print_usage(int /*argc*/, char ** argv, rpc_server_params params) {
    fprintf(stderr, "Usage: %s [options]\n\n", argv[0]);
    fprintf(stderr, "options:\n");
    fprintf(stderr, "  -h, --help            show this help message and exit\n");
    fprintf(stderr, "  -H HOST, --host HOST  host to bind to (default: %s)\n", params.host.c_str());
    fprintf(stderr, "  -p PORT, --port PORT  port to bind to (default: %d)\n", params.port);
    fprintf(stderr, "  -m MEM, --mem MEM     backend memory size (in MB)\n");
    fprintf(stderr, "\n");
}

/*
Note:杨小兵-2025-01-17

1、以static关键词修饰的函数意味着这个被修饰的函数只能在当前编译单元中是可见的，类似于类中的private的作用一样。
2、在不使用参数名称的情况下对参数进行说明，或者在参数名称旁边添加额外的信息以提高代码的可读性，参数 argc 未被使用，同时通过注释保留了参数的名称以便于理解。
3、在函数参数中添加注释一般都是为了解释说明
4、自增运算符说明
    4.1 i++:先使用i然后再增加i
    4.2 ++i:先增加i然后再使用i
*/
static bool rpc_server_params_parse(int argc, char ** argv, rpc_server_params & params) {
    std::string arg;
    for (int i = 1; i < argc; i++) {
        arg = argv[i];
        if (arg == "-H" || arg == "--host") {
            if (++i >= argc) {
                return false;
            }
            params.host = argv[i];
        } else if (arg == "-p" || arg == "--port") {
            if (++i >= argc) {
                return false;
            }
            params.port = std::stoi(argv[i]);
            if (params.port <= 0 || params.port > 65535) {
                return false;
            }
        } else if (arg == "-m" || arg == "--mem") {
            if (++i >= argc) {
                return false;
            }
            params.backend_mem = std::stoul(argv[i]) * 1024 * 1024;
        } else if (arg == "-h" || arg == "--help") {
            print_usage(argc, argv, params);
            exit(0);
        } else {
            fprintf(stderr, "error: unknown argument: %s\n", arg.c_str());
            print_usage(argc, argv, params);
            exit(0);
        }
    }
    return true;
}

/*
Note:杨小兵-2025-01-17

1、以static关键词修饰的函数意味着这个被修饰的函数只能在当前编译单元中是可见的，类似于类中的private的作用一样。
2、该函数整体的作用：根据不同的backend来获取backend相关的指针
*/
static ggml_backend_t create_backend() {
    ggml_backend_t backend = NULL;
#ifdef GGML_USE_CUDA
    fprintf(stderr, "%s: using CUDA backend\n", __func__);
    backend = ggml_backend_cuda_init(0); // init device 0
    if (!backend) {
        fprintf(stderr, "%s: ggml_backend_cuda_init() failed\n", __func__);
    }
#elif GGML_USE_METAL
    fprintf(stderr, "%s: using Metal backend\n", __func__);
    backend = ggml_backend_metal_init();
    if (!backend) {
        fprintf(stderr, "%s: ggml_backend_metal_init() failed\n", __func__);
    }
#elif GGML_USE_VULKAN
    fprintf(stderr, "%s: using Vulkan backend\n", __func__);
    backend = ggml_backend_vk_init(0); // init device 0
    if (!backend) {
        fprintf(stderr, "%s: ggml_backend_vulkan_init() failed\n", __func__);
    }
#elif GGML_USE_SYCL
    fprintf(stderr, "%s: using SYCL backend\n", __func__);
    backend = ggml_backend_sycl_init(0); // init device 0
    if (!backend) {
        fprintf(stderr, "%s: ggml_backend_sycl_init() failed\n", __func__);
    }
#endif

    // if there aren't GPU Backends fallback to CPU backend
    if (!backend) {
        fprintf(stderr, "%s: using CPU backend\n", __func__);
        backend = ggml_backend_cpu_init();
    }
    return backend;
}

/*
Note:杨小兵-2025-01-18

1、以static关键词修饰的函数意味着这个被修饰的函数只能在当前编译单元中是可见的，类似于类中的private的作用一样。
2、该函数整体的作用：针对不同的平台和后端，使用了不同的方法来获取系统的内存信息。
3、GlobalMemoryStatusEx 函数：检索有关当前系统内存使用情况的详细信息。
*/
static void get_backend_memory(size_t * free_mem, size_t * total_mem) {
#ifdef GGML_USE_CUDA
    ggml_backend_cuda_get_device_memory(0, free_mem, total_mem);
#elif GGML_USE_VULKAN
    ggml_backend_vk_get_device_memory(0, free_mem, total_mem);
#elif GGML_USE_SYCL
    ggml_backend_sycl_get_device_memory(0, free_mem, total_mem);
#else
    #ifdef _WIN32
        MEMORYSTATUSEX status;
        status.dwLength = sizeof(status);
        GlobalMemoryStatusEx(&status);
        *total_mem = status.ullTotalPhys;
        *free_mem = status.ullAvailPhys;
    #else
        long pages = sysconf(_SC_PHYS_PAGES);
        long page_size = sysconf(_SC_PAGE_SIZE);
        *total_mem = pages * page_size;
        *free_mem = *total_mem;
    #endif
#endif
}

int main(int argc, char * argv[]) {
    rpc_server_params params;
    if (!rpc_server_params_parse(argc, argv, params)) {
        fprintf(stderr, "Invalid parameters\n");
        return 1;
    }

    if (params.host != "127.0.0.1") {
        fprintf(stderr, "\n");
        fprintf(stderr, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        fprintf(stderr, "WARNING: Host ('%s') is != '127.0.0.1'\n", params.host.c_str());
        fprintf(stderr, "         Never expose the RPC server to an open network!\n");
        fprintf(stderr, "         This is an experimental feature and is not secure!\n");
        fprintf(stderr, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        fprintf(stderr, "\n");
    }

    ggml_backend_t backend = create_backend();
    if (!backend) {
        fprintf(stderr, "Failed to create backend\n");
        return 1;
    }
    std::string endpoint = params.host + ":" + std::to_string(params.port);
    size_t free_mem, total_mem;
    if (params.backend_mem > 0) {
        free_mem = params.backend_mem;
        total_mem = params.backend_mem;
    } else {
        get_backend_memory(&free_mem, &total_mem);
    }
    printf("Starting RPC server on %s, backend memory: %zu MB\n", endpoint.c_str(), free_mem / (1024 * 1024));
    ggml_backend_rpc_start_server(backend, endpoint.c_str(), free_mem, total_mem);
    ggml_backend_free(backend);
    return 0;
}

#pragma endregion
