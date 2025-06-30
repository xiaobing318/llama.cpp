/*
Notes:杨小兵-2025-06-30

1、通过相对路径的方式包含指定的头文件，编译系统在编译该编译单元的时候首先将会在额外指定的目录中查找对应的头文件，然后在默认系统目录中查找
对应的头文件。也就是说编译系统将会在系统配置目录和额外指定目录中查找对应的头文件，通过这种方式可以避免在包含头文件的时候使用绝对路径，这样
做的好处就是可以使得代码的可移植性更好，避免了在不同的系统上使用不同的绝对路径来包含头文件的问题。
*/
#include "ggml-backend-impl.h"

/*
Notes:杨小兵-2025-06-30

1、下列整体是一个条件编译块，只有在满足特定条件时才会编译和包含其中的代码，整体是为了编译特定于Intel/AMD的x86/x86-64系列处理器。
    1.1 条件一：编译器是 x86_64 架构的编译器。
    1.2 条件二：编译器是 Microsoft Visual C++ 编译器，并且目标架构是 AMD64。
*/
#if defined(__x86_64__) || (defined(_MSC_VER) && defined(_M_AMD64))

/*
Notes:杨小兵-2025-06-30

1、如果编译器是 Microsoft Visual C++ 编译器，则包含指定的头文件。
2、该头文件提供了对 CPUID 指令的访问，这个指令可以用来查询处理器的特性和功能。
*/
#ifdef _MSC_VER
#include <intrin.h>
#endif

/*
Notes:杨小兵-2025-06-30

1、包含 C/C++ 标准库中的头文件。
*/
#include <cstring>
#include <vector>
#include <bitset>
#include <array>
#include <string>

/*
Notes:杨小兵-2025-06-30

1、声明一个结构体 cpuid_x86，用于查询和存储 x86/x86-64 处理器的特性和功能。
2、结构体 cpuid_x86 的声明参考了 Intel 的软件开发手册（SDM），该手册提供了关于 x86/x86-64 架构的详细信息。
*/
// ref: https://cdrdv2-public.intel.com/782156/325383-sdm-vol-2abcd.pdf
struct cpuid_x86 {
    bool SSE3(void) { return f_1_ecx[0]; }
    bool PCLMULQDQ(void) { return f_1_ecx[1]; }
    bool MONITOR(void) { return f_1_ecx[3]; }
    bool SSSE3(void) { return f_1_ecx[9]; }
    bool FMA(void) { return f_1_ecx[12]; }
    bool CMPXCHG16B(void) { return f_1_ecx[13]; }
    bool SSE41(void) { return f_1_ecx[19]; }
    bool SSE42(void) { return f_1_ecx[20]; }
    bool MOVBE(void) { return f_1_ecx[22]; }
    bool POPCNT(void) { return f_1_ecx[23]; }
    bool AES(void) { return f_1_ecx[25]; }
    bool XSAVE(void) { return f_1_ecx[26]; }
    bool OSXSAVE(void) { return f_1_ecx[27]; }
    bool AVX(void) { return f_1_ecx[28]; }
    bool F16C(void) { return f_1_ecx[29]; }
    bool RDRAND(void) { return f_1_ecx[30]; }

    bool MSR(void) { return f_1_edx[5]; }
    bool CX8(void) { return f_1_edx[8]; }
    bool SEP(void) { return f_1_edx[11]; }
    bool CMOV(void) { return f_1_edx[15]; }
    bool CLFSH(void) { return f_1_edx[19]; }
    bool MMX(void) { return f_1_edx[23]; }
    bool FXSR(void) { return f_1_edx[24]; }
    bool SSE(void) { return f_1_edx[25]; }
    bool SSE2(void) { return f_1_edx[26]; }

    bool FSGSBASE(void) { return f_7_ebx[0]; }
    bool BMI1(void) { return f_7_ebx[3]; }
    bool HLE(void) { return is_intel && f_7_ebx[4]; }
    bool AVX2(void) { return f_7_ebx[5]; }
    bool BMI2(void) { return f_7_ebx[8]; }
    bool ERMS(void) { return f_7_ebx[9]; }
    bool INVPCID(void) { return f_7_ebx[10]; }
    bool RTM(void) { return is_intel && f_7_ebx[11]; }
    bool AVX512F(void) { return f_7_ebx[16]; }
    bool AVX512DQ(void) { return f_7_ebx[17]; }
    bool RDSEED(void) { return f_7_ebx[18]; }
    bool ADX(void) { return f_7_ebx[19]; }
    bool AVX512PF(void) { return f_7_ebx[26]; }
    bool AVX512ER(void) { return f_7_ebx[27]; }
    bool AVX512CD(void) { return f_7_ebx[28]; }
    bool AVX512BW(void) { return f_7_ebx[30]; }
    bool AVX512VL(void) { return f_7_ebx[31]; }

    bool SHA(void) { return f_7_ebx[29]; }

    bool PREFETCHWT1(void) { return f_7_ecx[0]; }

    bool LAHF(void) { return f_81_ecx[0]; }
    bool LZCNT(void) { return is_intel && f_81_ecx[5]; }
    bool ABM(void) { return is_amd && f_81_ecx[5]; }
    bool SSE4a(void) { return is_amd && f_81_ecx[6]; }
    bool XOP(void) { return is_amd && f_81_ecx[11]; }
    bool TBM(void) { return is_amd && f_81_ecx[21]; }

    bool SYSCALL(void) { return is_intel && f_81_edx[11]; }
    bool MMXEXT(void) { return is_amd && f_81_edx[22]; }
    bool RDTSCP(void) { return is_intel && f_81_edx[27]; }
    bool _3DNOWEXT(void) { return is_amd && f_81_edx[30]; }
    bool _3DNOW(void) { return is_amd && f_81_edx[31]; }

    bool AVX512_VBMI(void) { return f_7_ecx[1]; }
    bool AVX512_VNNI(void) { return f_7_ecx[11]; }
    bool AVX512_FP16(void) { return f_7_edx[23]; }
    bool AVX512_BF16(void) { return f_7_1_eax[5]; }
    bool AVX_VNNI(void) { return f_7_1_eax[4]; }

    bool AMX_TILE(void) { return f_7_edx[24]; }
    bool AMX_INT8(void) { return f_7_edx[25]; }
    bool AMX_FP16(void) { return f_7_1_eax[21]; }
    bool AMX_BF16(void) { return f_7_edx[22]; }

#ifdef _MSC_VER
    static void cpuid(int cpu_info[4], int eax) {
        __cpuid(cpu_info, eax);
    }
    static void cpuidex(int cpu_info[4], int eax, int ecx) {
        __cpuidex(cpu_info, eax, ecx);
    }
#else
    static void cpuid(int cpu_info[4], int eax) {
        __asm__ __volatile__(
            "cpuid"
            : "=a"(cpu_info[0]), "=b"(cpu_info[1]), "=c"(cpu_info[2]), "=d"(cpu_info[3])
            : "a"(eax), "c"(0));
    }
    static void cpuidex(int cpu_info[4], int eax, int ecx) {
        __asm__ __volatile__(
            "cpuid"
            : "=a"(cpu_info[0]), "=b"(cpu_info[1]), "=c"(cpu_info[2]), "=d"(cpu_info[3])
            : "a"(eax), "c"(ecx));
    }
#endif

    cpuid_x86() {
        /*
        Notes:杨小兵-2025-06-30

        1、定义了一个整数数组 cpui，用于存储 CPUID 指令的结果，为什么是4个整数？因为CPUID指令返回的结果包含四个32位整数，
        分别存储在 eax、ebx、ecx 和 edx 寄存器中，我们使用4个整数来存储这些结果。
        2、定义了一个二维数组 data，用于存储 CPUID 指令的结果，每个元素是一个包含四个整数的数组，可以存储多个上述4个整数的结果，
        为什么要用向量？因为我们会多次调用 CPUID 指令，每次调用返回的结果可能会不同，并且为了将这些不同的结果都存储起来，因此
        需要使用动态数组来存储这些结果。
        */
        std::array<int, 4> cpui;
        std::vector<std::array<int, 4>> data;

        /*
        Notes:杨小兵-2025-06-30

        1、首先调用了 cpuid 函数（CPUID 指令的封装），传入 0，意思是问 CPU：“你支持的最大基本功能编号是多少？”结果会填到 cpui 里。
        2、然后 CPU 把最大功能编号（highest valid function ID）存在 cpui[0]（对应 EAX 寄存器），我们把它取出来存到 n_ids 里。
        为什么要这么做？因为接下来我们要用这个编号决定查多少次 CPUID。
        */
        // calling __cpuid with 0x0 as the function_id argument
        // gets the number of the highest valid function ID.
        cpuid(cpui.data(), 0);
        int n_ids = cpui[0];

        /*
        Notes:杨小兵-2025-06-30

        1、下列代码是一个循环，从 0 跑到 n_ids，每次加 1。为什么要循环？因为 CPUID 支持多个功能编号（0、1、2...），每个编号返回不同信息，
        需要把它们都收集起来。
        2、调用指定函数，传入功能编号 i 和子叶 0（ECX=0），结果存到 cpui 里。为什么要用 cpuidex？因为它能更精确地查询某些信息。
        3、把这次查询的结果塞进动态数组中存储起来。为什么要存起来？因为后面要用这些数据提取厂商名、功能标志等。
        */
        for (int i = 0; i <= n_ids; ++i) {
            cpuidex(cpui.data(), i, 0);
            data.push_back(cpui);
        }

        /*
        Notes:杨小兵-2025-06-30

        1、首先声明一个局部临时的字符数组 vendor，用于存储 CPU 厂商字符串，长度为 32 字节（0x20 = 32）。为什么要 32 字节？
        够大，能装下标准厂商字符串。
        2、从动态数组（功能编号 0 的结果）里取 EBX（data[0][1]），塞到 vendor 的前 4 个字节。为什么要这样？
        因为 CPUID 用 EBX、EDX、ECX 返回厂商字符串的字符。
        3、从动态数组（功能编号 0 的结果）里取 EDX（data[0][3]），塞到 vendor 的第 5 到 8 字节。
        4、从动态数组（功能编号 0 的结果）里取 ECX（data[0][2]），塞到 vendor 的第 9 到 12 字节。为什么要按这个顺序？
        因为 CPUID 返回的顺序是 EBX-EDX-ECX，拼起来就是厂商名。
        5、将拼接好的厂商名存储到成员变量中。
        */
        // capture vendor string
        char vendor[0x20] = {};
        *reinterpret_cast<int *>(vendor)     = data[0][1];
        *reinterpret_cast<int *>(vendor + 4) = data[0][3];
        *reinterpret_cast<int *>(vendor + 8) = data[0][2];
        this->vendor = vendor;
        if (this->vendor == "GenuineIntel") {
            is_intel = true;
        } else if (this->vendor == "AuthenticAMD") {
            is_amd = true;
        }

        /*
        Notes:杨小兵-2025-06-30

        1、检查最大功能编号是不是至少到 1。如果是，才继续。为什么要检查？因为不是所有 CPU 都支持功能 1。
        2、从 data[1]（功能编号 1）取 ECX 的值，存到 f_1_ecx。为什么要存？ECX 包含一堆功能标志，比如 SSE3 支持。
        3、取 EDX 的值，存到 f_1_edx。EDX 也有功能标志，比如 MMX 支持。
        */
        // load bitset with flags for function 0x00000001
        if (n_ids >= 1) {
            f_1_ecx = data[1][2];
            f_1_edx = data[1][3];
        }

        /*
        Notes:杨小兵-2025-06-30

        1、检查是否支持功能编号 7（现代 CPU 才支持）。为什么要检查？老 CPU 可能不支持。
        2、从 data[7]（功能编号 7，ECX=0）取 EBX，存到 f_7_ebx。这些是高级功能标志，比如 AVX2。相对应的还有 ECX 和 EDX 。
        3、再查一次功能 7，但这次 ECX=1，获取额外信息。为什么要再查？因为功能 7 有子叶，ECX=1 返回不同数据。
        4、把这次的 EAX 存到 f_7_1_eax，记录更多功能。
        */
        // load bitset with flags for function 0x00000007
        if (n_ids >= 7) {
            f_7_ebx = data[7][1];
            f_7_ecx = data[7][2];
            f_7_edx = data[7][3];
            cpuidex(cpui.data(), 7, 1);
            f_7_1_eax = cpui[0];
        }

        /*
        Notes:杨小兵-2025-06-30

        1、调用 CPUID，传入 0x80000000，问：“你支持的最大扩展功能编号是多少？”结果存到 cpui。
        2、把最大扩展编号存到 n_ex_ids。为什么要查扩展？因为基本功能（0 开始）和扩展功能（0x80000000 开始）是分开的。
        */
        // calling __cpuid with 0x80000000 as the function_id argument
        // gets the number of the highest valid extended ID.
        cpuid(cpui.data(), 0x80000000);
        unsigned int n_ex_ids = cpui[0];

        /*
        Notes:杨小兵-2025-06-30

        1、声明定义一个动态二维数组用来存储扩展功能的 CPUID 结果，从 0x80000000 到 n_ex_ids。
        2、循环从 0x80000000 到 n_ex_ids，每次调用 cpuidex，传入 i（功能编号）和 0，获取扩展功能信息。
        3、把每次的结果存到 ext_data 中。为什么要这么做？因为扩展功能编号也有很多，不能只查一个。
        */
        std::vector<std::array<int, 4>> ext_data;
        for (unsigned int i = 0x80000000; i <= n_ex_ids; ++i) {
            cpuidex(cpui.data(), i, 0);
            ext_data.push_back(cpui);
        }

        /*
        Notes:杨小兵-2025-06-30

        1、检查最大扩展功能编号是不是至少到 0x80000001。如果是，才继续。为什么要检查？因为不是所有 CPU 都支持功能 0x80000001。
        2、从 ext_data[1]（扩展功能编号 2）取 ECX 的值，存到 f_81_ecx。为什么要存？ECX 包含一堆功能标志，比如 64 位支持。
        3、从 ext_data[1]（扩展功能编号 3）取 EDX 的值，存到 f_81_edx。
        */
        // load bitset with flags for function 0x80000001
        if (n_ex_ids >= 0x80000001) {
            f_81_ecx = ext_data[1][2];
            f_81_edx = ext_data[1][3];
        }

        /*
        Notes:杨小兵-2025-06-30

        1、整体的目的是解析 CPU 的品牌字符串。
        2、首先声明一个字符数组 brand，长度为 64 字节（0x40 = 64）。为什么要 64 字节？因为品牌字符串可能很长，需要足够空间。
        3、检查最大扩展功能编号是不是至少到 0x80000004。如果是，才继续。为什么要检查？因为不是所有 CPU 都支持品牌字符串。
        4、从 ext_data[2]（扩展功能编号 4）取数据，复制到 brand 的前 16 字节。为什么要复制？因为品牌字符串分成了多个部分。
        5、从 ext_data[3]（扩展功能编号 5）取数据，复制到 brand 的第 17 到 32 字节。
        6、从 ext_data[4]（扩展功能编号 6）取数据，复制到 brand 的第 33 到 48 字节。为什么要分三块？因为品牌名太长，分三部分返回。
        7、最后把 brand 转换成字符串，存到成员变量 this->brand 中。为什么要转换？因为我们需要一个易读的品牌名。
        */
        // interpret CPU brand string if reported
        char brand[0x40] = {};
        if (n_ex_ids >= 0x80000004) {
            std::memcpy(brand, ext_data[2].data(), sizeof(cpui));
            std::memcpy(brand + 16, ext_data[3].data(), sizeof(cpui));
            std::memcpy(brand + 32, ext_data[4].data(), sizeof(cpui));
            this->brand = brand;
        }
    }

    bool is_intel = false;
    bool is_amd = false;
    std::string vendor;
    std::string brand;
    std::bitset<32> f_1_ecx;
    std::bitset<32> f_1_edx;
    std::bitset<32> f_7_ebx;
    std::bitset<32> f_7_ecx;
    std::bitset<32> f_7_edx;
    std::bitset<32> f_7_1_eax;
    std::bitset<32> f_81_ecx;
    std::bitset<32> f_81_edx;
};

#if 0
void test_x86_is() {
    cpuid_x86 is;
    printf("CPU Vendor: %s\n", is.vendor.c_str());
    printf("Brand: %s\n", is.brand.c_str());
    printf("is_intel: %d\n", is.is_intel);
    printf("is_amd: %d\n", is.is_amd);
    printf("sse3: %d\n", is.SSE3());
    printf("pclmulqdq: %d\n", is.PCLMULQDQ());
    printf("ssse3: %d\n", is.SSSE3());
    printf("fma: %d\n", is.FMA());
    printf("cmpxchg16b: %d\n", is.CMPXCHG16B());
    printf("sse41: %d\n", is.SSE41());
    printf("sse42: %d\n", is.SSE42());
    printf("movbe: %d\n", is.MOVBE());
    printf("popcnt: %d\n", is.POPCNT());
    printf("aes: %d\n", is.AES());
    printf("xsave: %d\n", is.XSAVE());
    printf("osxsave: %d\n", is.OSXSAVE());
    printf("avx: %d\n", is.AVX());
    printf("f16c: %d\n", is.F16C());
    printf("rdrand: %d\n", is.RDRAND());
    printf("msr: %d\n", is.MSR());
    printf("cx8: %d\n", is.CX8());
    printf("sep: %d\n", is.SEP());
    printf("cmov: %d\n", is.CMOV());
    printf("clflush: %d\n", is.CLFSH());
    printf("mmx: %d\n", is.MMX());
    printf("fxsr: %d\n", is.FXSR());
    printf("sse: %d\n", is.SSE());
    printf("sse2: %d\n", is.SSE2());
    printf("fsgsbase: %d\n", is.FSGSBASE());
    printf("bmi1: %d\n", is.BMI1());
    printf("hle: %d\n", is.HLE());
    printf("avx2: %d\n", is.AVX2());
    printf("bmi2: %d\n", is.BMI2());
    printf("erms: %d\n", is.ERMS());
    printf("invpcid: %d\n", is.INVPCID());
    printf("rtm: %d\n", is.RTM());
    printf("avx512f: %d\n", is.AVX512F());
    printf("rdseed: %d\n", is.RDSEED());
    printf("adx: %d\n", is.ADX());
    printf("avx512pf: %d\n", is.AVX512PF());
    printf("avx512er: %d\n", is.AVX512ER());
    printf("avx512cd: %d\n", is.AVX512CD());
    printf("sha: %d\n", is.SHA());
    printf("prefetchwt1: %d\n", is.PREFETCHWT1());
    printf("lahf: %d\n", is.LAHF());
    printf("lzcnt: %d\n", is.LZCNT());
    printf("abm: %d\n", is.ABM());
    printf("sse4a: %d\n", is.SSE4a());
    printf("xop: %d\n", is.XOP());
    printf("tbm: %d\n", is.TBM());
    printf("syscall: %d\n", is.SYSCALL());
    printf("mmxext: %d\n", is.MMXEXT());
    printf("rdtscp: %d\n", is.RDTSCP());
    printf("3dnowext: %d\n", is._3DNOWEXT());
    printf("3dnow: %d\n", is._3DNOW());
    printf("avx512_vbmi: %d\n", is.AVX512_VBMI());
    printf("avx512_vnni: %d\n", is.AVX512_VNNI());
    printf("avx512_fp16: %d\n", is.AVX512_FP16());
    printf("avx512_bf16: %d\n", is.AVX512_BF16());
    printf("amx_tile: %d\n", is.AMX_TILE());
    printf("amx_int8: %d\n", is.AMX_INT8());
    printf("amx_fp16: %d\n", is.AMX_FP16());
    printf("amx_bf16: %d\n", is.AMX_BF16());
}
#endif

/*
Notes:杨小兵-2025-06-30

1、在当前编译单元中声明定义一个静态函数，这个函数的作用是计算当前 CPU 的特性分数。
2、将这个函数声明为静态的是为了限制其作用域仅限于当前编译单元，避免与其他编译单元中的同名函数冲突，更为深层次的原因是为了
强制执行了接口与实现的分离，提高了代码的封装性和可维护性。
    2.1 公共接口: ggml_backend_score()。它的名字和签名是固定的，是主程序和所有后端DLL之间的“契约”。
    2.2 内部实现: ggml_backend_cpu_x86_score()。这是实现该契约的具体逻辑。它的名字、参数甚至存在与否，都可能随着版本迭代而改变。
*/
static int ggml_backend_cpu_x86_score() {
    // FIXME: this does not check for OS support

    /*
    Notes:杨小兵-2025-06-30
    
    1、声明定义一个整数 score，用于存储 CPU 特性的分数，初始值为 0。
    2、声明定义一个 cpuid_x86 类型的变量 is，用于查询 CPU 的特性，在定义的时候其结构体内部的构造函数已经将 CPU 的特性查询完毕，
    只需要直接使用其中定义的接口进行查询即可。
    */
    int score = 0;
    cpuid_x86 is;

#ifdef GGML_FMA
    if (!is.FMA()) { return 0; }
    score += 1;
#endif
#ifdef GGML_F16C
    if (!is.F16C()) { return 0; }
    score += 1<<1;
#endif
#ifdef GGML_SSE42
    if (!is.SSE42()) { return 0; }
    score += 1<<2;
#endif
#ifdef GGML_AVX
    if (!is.AVX()) { return 0; }
    score += 1<<4;
#endif
#ifdef GGML_AVX2
    if (!is.AVX2()) { return 0; }
    score += 1<<5;
#endif
#ifdef GGML_AVX_VNNI
    if (!is.AVX_VNNI()) { return 0; }
    score += 1<<6;
#endif
#ifdef GGML_AVX512
    if (!is.AVX512F()) { return 0; }
    if (!is.AVX512CD()) { return 0; }
    if (!is.AVX512VL()) { return 0; }
    if (!is.AVX512DQ()) { return 0; }
    if (!is.AVX512BW()) { return 0; }
    score += 1<<7;
#endif
#ifdef GGML_AVX512_VBMI
    if (!is.AVX512_VBMI()) { return 0; }
    score += 1<<8;
#endif
#ifdef GGML_AVX512_BF16
    if (!is.AVX512_BF16()) { return 0; }
    score += 1<<9;
#endif
#ifdef GGML_AVX512_VNNI
    if (!is.AVX512_VNNI()) { return 0; }
    score += 1<<10;
#endif
#ifdef GGML_AMX_INT8
    if (!is.AMX_INT8()) { return 0; }
    score += 1<<11;
#endif

    return score;
}

GGML_BACKEND_DL_SCORE_IMPL(ggml_backend_cpu_x86_score)

#endif // defined(__x86_64__) || (defined(_MSC_VER) && defined(_M_AMD64))
