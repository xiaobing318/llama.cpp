#include "ggml.h"
#include "ggml-backend.h"
#include "ggml-impl.h"
#include "gguf.h"

#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

template <typename T>
struct type_to_gguf_type;

template <>
struct type_to_gguf_type<uint8_t> {
    static constexpr enum gguf_type value = GGUF_TYPE_UINT8;
};

template <>
struct type_to_gguf_type<int8_t> {
    static constexpr enum gguf_type value = GGUF_TYPE_INT8;
};

template <>
struct type_to_gguf_type<uint16_t> {
    static constexpr enum gguf_type value = GGUF_TYPE_UINT16;
};

template <>
struct type_to_gguf_type<int16_t> {
    static constexpr enum gguf_type value = GGUF_TYPE_INT16;
};

template <>
struct type_to_gguf_type<uint32_t> {
    static constexpr enum gguf_type value = GGUF_TYPE_UINT32;
};

template <>
struct type_to_gguf_type<int32_t> {
    static constexpr enum gguf_type value = GGUF_TYPE_INT32;
};

template <>
struct type_to_gguf_type<float> {
    static constexpr enum gguf_type value = GGUF_TYPE_FLOAT32;
};

template <>
struct type_to_gguf_type<bool> {
    static constexpr enum gguf_type value = GGUF_TYPE_BOOL;
};

template <>
struct type_to_gguf_type<std::string> {
    static constexpr enum gguf_type value = GGUF_TYPE_STRING;
};

template <>
struct type_to_gguf_type<uint64_t> {
    static constexpr enum gguf_type value = GGUF_TYPE_UINT64;
};

template <>
struct type_to_gguf_type<int64_t> {
    static constexpr enum gguf_type value = GGUF_TYPE_INT64;
};

template <>
struct type_to_gguf_type<double> {
    static constexpr enum gguf_type value = GGUF_TYPE_FLOAT64;
};

static const std::map<gguf_type, size_t> GGUF_TYPE_SIZE = {
    {GGUF_TYPE_UINT8,   sizeof(uint8_t)},
    {GGUF_TYPE_INT8,    sizeof(int8_t)},
    {GGUF_TYPE_UINT16,  sizeof(uint16_t)},
    {GGUF_TYPE_INT16,   sizeof(int16_t)},
    {GGUF_TYPE_UINT32,  sizeof(uint32_t)},
    {GGUF_TYPE_INT32,   sizeof(int32_t)},
    {GGUF_TYPE_FLOAT32, sizeof(float)},
    {GGUF_TYPE_BOOL,    sizeof(int8_t)},
    {GGUF_TYPE_STRING,  0}, // undefined
    {GGUF_TYPE_ARRAY,   0}, // undefined
    {GGUF_TYPE_UINT64,  sizeof(uint64_t)},
    {GGUF_TYPE_INT64,   sizeof(int64_t)},
    {GGUF_TYPE_FLOAT64, sizeof(double)},
};
static_assert(GGUF_TYPE_COUNT == 13, "GGUF_TYPE_COUNT != 13");

static const std::map<gguf_type, const char *> GGUF_TYPE_NAME = {
    {GGUF_TYPE_UINT8,   "u8"},
    {GGUF_TYPE_INT8,    "i8"},
    {GGUF_TYPE_UINT16,  "u16"},
    {GGUF_TYPE_INT16,   "i16"},
    {GGUF_TYPE_UINT32,  "u32"},
    {GGUF_TYPE_INT32,   "i32"},
    {GGUF_TYPE_FLOAT32, "f32"},
    {GGUF_TYPE_BOOL,    "bool"},
    {GGUF_TYPE_STRING,  "str"},
    {GGUF_TYPE_ARRAY,   "arr"},
    {GGUF_TYPE_UINT64,  "u64"},
    {GGUF_TYPE_INT64,   "i64"},
    {GGUF_TYPE_FLOAT64, "f64"},
};
static_assert(GGUF_TYPE_COUNT == 13, "GGUF_TYPE_COUNT != 13");

size_t gguf_type_size(enum gguf_type type) {
    auto it = GGUF_TYPE_SIZE.find(type);
    return it == GGUF_TYPE_SIZE.end() ? 0 : it->second;
}

struct gguf_kv {
    std::string key;

    bool is_array;
    enum gguf_type type;

    std::vector<int8_t>      data;
    std::vector<std::string> data_string;

    template <typename T>
    gguf_kv(const std::string & key, const T value)
            : key(key), is_array(false), type(type_to_gguf_type<T>::value) {
        GGML_ASSERT(!key.empty());
        data.resize(sizeof(T));
        memcpy(data.data(), &value, sizeof(T));
    }

    template <typename T>
    gguf_kv(const std::string & key, const std::vector<T> & value)
            : key(key), is_array(true), type(type_to_gguf_type<T>::value) {
        GGML_ASSERT(!key.empty());
        data.resize(value.size()*sizeof(T));
        for (size_t i = 0; i < value.size(); ++i) {
            const T tmp = value[i];
            memcpy(data.data() + i*sizeof(T), &tmp, sizeof(T));
        }
    }

    gguf_kv(const std::string & key, const std::string & value)
            : key(key), is_array(false), type(GGUF_TYPE_STRING) {
        GGML_ASSERT(!key.empty());
        data_string.push_back(value);
    }

    gguf_kv(const std::string & key, const std::vector<std::string> & value)
            : key(key), is_array(true), type(GGUF_TYPE_STRING) {
        GGML_ASSERT(!key.empty());
        data_string = value;
    }

    const std::string & get_key() const {
        return key;
    }

    const enum gguf_type & get_type() const {
        return type;
    }

    size_t get_ne() const {
        if (type == GGUF_TYPE_STRING) {
            const size_t ne = data_string.size();
            GGML_ASSERT(is_array || ne == 1);
            return ne;
        }
        const size_t type_size = gguf_type_size(type);
        GGML_ASSERT(data.size() % type_size == 0);
        const size_t ne = data.size() / type_size;
        GGML_ASSERT(is_array || ne == 1);
        return ne;
    }

    template <typename T>
    const T & get_val(const size_t i = 0) const {
        GGML_ASSERT(type_to_gguf_type<T>::value == type);
        if constexpr (std::is_same<T, std::string>::value) {
            GGML_ASSERT(data_string.size() >= i+1);
            return data_string[i];
        }
        const size_t type_size = gguf_type_size(type);
        GGML_ASSERT(data.size() % type_size == 0);
        GGML_ASSERT(data.size() >= (i+1)*type_size);
        return reinterpret_cast<const T *>(data.data())[i];
    }

    void cast(const enum gguf_type new_type) {
        const size_t new_type_size = gguf_type_size(new_type);
        GGML_ASSERT(data.size() % new_type_size == 0);
        type = new_type;
    }
};

struct gguf_tensor_info {
    struct ggml_tensor t; // for holding the equivalent info
    uint64_t offset;      // offset from start of `data`, must be a multiple of `ALIGNMENT`
};

struct gguf_context {
    uint32_t version = GGUF_VERSION;

    std::vector<struct gguf_kv> kv;
    std::vector<struct gguf_tensor_info> info;

    size_t alignment = GGUF_DEFAULT_ALIGNMENT;
    size_t offset    = 0; // offset of `data` from beginning of file
    size_t size      = 0; // size of `data` in bytes

    void * data = nullptr;
};

struct gguf_reader {
    /*
    Notes:杨小兵-2025-07-16

    1、fread 是 C 标准库中用于从文件流中批量读取数据的函数，其原型为：
        size_t fread(void *ptr, size_t size, size_t count, FILE *stream);
    它一次最多读取 count 个元素，每个元素大小为 size 字节，并将读到的数据存入 ptr 所指内存。返回值为实际读取到的元素个数。
    2、fread 函数原型与参数说明
        2.1 ptr：目标缓冲区指针，读到的数据会存放于此
        2.2 size：每个元素的字节大小
        2.3 count：要读取的元素数量
        2.4 stream：已打开的 FILE* 文件流指针
        2.5 返回值：实际成功读取的元素数量；若小于 count，可通过 feof 或 ferror 判断是否到达文件末尾或发生错误
    */
    FILE * file;

    gguf_reader(FILE * file) : file(file) {}

    template <typename T>
    bool read(T & dst) const {
        return fread(&dst, 1, sizeof(dst), file) == sizeof(dst);
    }
    /*
    Notes:杨小兵-2025-07-16

    1、这里使用到了 C++ 中的模板函数，模板函数允许我们编写通用的函数，可以接受不同类型的参数。
    2、上述的 read 函数可以接受各种基本类型，例如 int、float、double 等等，并将读取到的数据存储到 dst 中。
    3、函数实现细节
        3.1 首先使用 fread 函数从文件流中读取数据到 dst 中，读取元素数量为目标变量的内存字节大小，单个元素在内存中的大小为 1 。
        3.2 如果 fread 函数返回的元素数量等于 sizeof(dst)，则表示读取成功，函数返回 true。
        3.3 如果 fread 函数返回的元素数量小于 sizeof(dst)，则表示读取失败，函数返回 false。
    */

    template <typename T>
    bool read(std::vector<T> & dst, const size_t n) const {
        //  将动态数组 dst 的大小调整为 n 个元素，内部会重新分配内存，但是目前不需要知道具体细节。
        dst.resize(n);
        //  循环处理动态数组 dst 中的每个元素，使用 read 函数读取数据。
        for (size_t i = 0; i < dst.size(); ++i) {
            //  编译时检查 T 是否为bool类型，如果 T 是 bool 类型，则执行对应的代码块，否则执行 else 代码块。
            if constexpr (std::is_same<T, bool>::value) {
                //  创建一个临时变量 tmp，用于存储读取的 bool 值。
                bool tmp;
                //  如果读取失败，则返回 false。
                if (!read(tmp)) {
                    return false;
                }
                //  代码执行到这里，说明读取成功，将临时变量 tmp 的值赋给动态数组 dst 中的第 i 个元素。
                dst[i] = tmp;
            } else {
                //  如果 T 不是 bool 类型，则直接调用 read 函数读取数据到动态数组 dst 中的第 i 个元素。
                if (!read(dst[i])) {
                    return false;
                }
            }
        }
        //  如果循环执行完毕，说明所有元素都读取成功，返回 true。
        return true;
    }
    /*
    Notes:杨小兵-2025-07-16

    1、上述函数是一个模板函数，用于从文件中读取 n 个某种类型的元素，并将它们存储到动态数组 dst 中。
    2、根据不同的类型 T，函数会执行不同的读取操作。
    */

    bool read(bool & dst) const {
        //  创建一个临时变量 tmp，用于存储读取的 bool 值，说明 bool 类型在内存中占用一个字节。
        int8_t tmp = -1;
        /*
            如果读取失败，则返回 false ，这里使用 int8_t 是因为在 GGUF 格式中，bool 值是以 int8_t 类型存储的，并且 fopen 函数读取的
        最小单位、处理单位都是字节。
        */
        if (!read(tmp)) {
            return false;
        }
        //  代码执行到这里，说明读取成功，将临时变量 tmp 的值转换为 0 或者 1 ，并赋值给 dst。
        dst = tmp != 0;
        return true;
    }
    /*
    Notes:杨小兵-2025-07-16

    1、上述函数是一个成员函数，用于从文件中读取一个 bool 类型的值，并将其存储到 dst 中。
    */

    bool read(enum ggml_type & dst) const {
        //  创建一个 int32_t 类型的临时变量 tmp，用于存储从 GGUF 格式文件中读取到的四个字节内容，这四个字节内容表示 GGML 类型。
        int32_t tmp = -1;
        //  调用 read 函数从文件中读取四个字节内容到临时变量 tmp 中，如果读取失败，则返回 false。
        if (!read(tmp)) {
            //  返回 false, 表示读取失败。
            return false;
        }
        //  代码执行到这里，说明读取成功，将临时变量 tmp 的值转换为 ggml_type 枚举类型，并赋值给 dst。
        dst = ggml_type(tmp);
        return true;
    }
    /*
    Notes:杨小兵-2025-07-16

    1、上述函数是一个成员函数，用于从文件中读取一个枚举类型的值，并将其存储到 dst 中，从底层实现来看则是从 GGUF 格式文件中读取了四个字节。
    */

    bool read(enum gguf_type & dst) const {
        //  创建一个 int32_t 类型的临时变量 tmp，用于存储从 GGUF 格式文件中读取到的四个字节内容，这四个字节内容表示 GGUF KV 数据类型。
        int32_t tmp = -1;
        //  调用 read 函数从文件中读取四个字节内容到临时变量 tmp 中，如果读取失败，则返回 false。
        if (!read(tmp)) {
            //  返回 false, 表示读取失败。
            return false;
        }
        //  将从 GGUF 格式文件中读取到的临时变量 tmp 的值转换为 gguf_type 枚举类型，并赋值给 dst。
        dst = gguf_type(tmp);
        //  代码执行到这里，说明读取成功，返回 true。
        return true;
    }
    /*
    Notes:杨小兵-2025-07-16

    1、上述函数是一个成员函数，用于从文件中读取一个枚举类型的值，并将其存储到 dst 中，从底层实现来看则是从 GGUF 格式文件中读取了四个字节。
    */

    bool read(std::string & dst) const {
        //  创建一个 uint64_t 类型的临时变量 size，用于存储从 GGUF 格式文件中读取到的字符串长度，这里的字符串长度是字节数。
        uint64_t size = -1;
        //  调用 read 函数从文件中读取八个字节内容到临时变量 size 中，如果读取失败，则返回 false。
        if (!read(size)) {
            //  返回 false, 表示读取失败。
            return false;
        }
        //  代码执行到这里，说明读取成功，接下来需要从 GGUF 格式文件中读取字符串内容，首先调整字符串 dst 的大小为 size 字节。
        dst.resize(size);
        //  调用 fread 函数从文件中读取 size 字节内容到字符串 dst 中，如果读取的元素数量不等于 size，则表示读取失败，返回 false。
        return fread(dst.data(), 1, dst.length(), file) == dst.length();
    }
    /*
    Notes:杨小兵-2025-07-16

    1、上述函数是一个成员函数，用于从 GGUF 格式文件中读取一定长度的字符串，并且将读取到的字符串内容保存在 dst 中。
    */

    bool read(void * dst, const size_t size) const {
        /*
            调用 fread 函数从文件中读取 size 字节内容到 dst 中，如果读取的元素数量不等于 size，则表示读取失败，返回 false。从字节
        流的角度来看，dst 是一个指向内存的指针，size 是要读取的字节数。
        */
        return fread(dst, 1, size, file) == size;
    }
    /*
    Notes:杨小兵-2025-07-16

    1、上述函数是一个成员函数，用于从 GGUF 格式文件中读取一定数量的字节，并且将读取到的字节流保存在 dst 指向的内存空间中。
    */
};

struct gguf_context * gguf_init_empty(void) {
    return new gguf_context;
}

template<typename T>
bool gguf_read_emplace_helper(const struct gguf_reader & gr, std::vector<struct gguf_kv> & kv, const std::string & key, const bool is_array, const size_t n) {
    //  如果 is_array 为 true，则表示要读取一个数组类型的值，否则表示要读取一个单一值。
    if (is_array) {
        //  创建一个类型为 T 的动态数组 value，用于存储从 GGUF 格式文件中读取到的数组值。
        std::vector<T> value;
        //  使用 C++ 中的 try-catch 语句来捕获可能发生的异常。
        try {
            //  调用 gguf_reader 的 read 函数从 GGUF 格式文件中读取 n 个 T 类型的值到动态数组 value 中，根据读取结果来执行是否返回。
            if (!gr.read(value, n)) {
                //  返回 false，表示读取失败。
                return false;
            }
        } catch (std::length_error &) {
            //  如果发生了 std::length_error 异常，表示读取的数组长度超过了预期的长度，则输出错误信息并返回 false。
            fprintf(stderr, "%s: encountered length_error while reading value for key '%s'\n", __func__, key.c_str());
            //  返回 false，表示读取失败。
            return false;
        } catch (std::bad_alloc &) {
            //  如果发生了 std::bad_alloc 异常，表示内存分配失败，则输出错误信息并返回 false。
            fprintf(stderr, "%s: encountered bad_alloc error while reading value for key '%s'\n", __func__, key.c_str());
            //  返回 false，表示读取失败。
            return false;
        }
        //  代码执行到这里，说明读取成功，将 key 和 读取到的 value 使用 emplace_back 函数添加到 kv 中保存起来。
        kv.emplace_back(key, value);
    } else {
        //  创建一个类型为 T 的变量 value，用于存储从 GGUF 格式文件中读取到的单一值。
        T value;
        //  调用 gguf_reader 的 read 函数从 GGUF 格式文件中读取一个 T 类型的值到变量 value 中，根据读取结果来执行是否返回。
        if (!gr.read(value)) {
            //  返回 false，表示读取失败。
            return false;
        }
        //  代码执行到这里，说明读取成功，将 key 和 读取到的 value 使用 emplace_back 函数添加到 kv 中保存起来。
        kv.emplace_back(key, value);
    }
    //  返回 true，表示读取成功。
    return true;
}

/*
Notes:杨小兵-2025-07-20

1、这个函数中的 gguf_init_from_file_impl 函数是一个实现函数，用于从文件中初始化 GGUF 上下文。
2、函数中的有些实现细节还没有完全理解清楚，可能需要进一步的学习和实践。
*/
struct gguf_context * gguf_init_from_file_impl(FILE * file, struct gguf_init_params params) {
    //  在当前函数作用域中创建一个结构体常量类型的 gguf_reader 对象 gr，用于读取 GGUF 格式文件。
    const struct gguf_reader gr(file);
    //  在当前函数作用域中创建一个临时的、指针类型的 ctx 变量，用来指向新创建的 gguf_context 对象，即保存新创建的 gguf_context 对象的地址。
    struct gguf_context * ctx = new gguf_context;

    //  在当前函数作用域中创建一个布尔类型的变量，用来表示读取 GGUF 格式文件的状态情况，如果读取成功则为 true，否则为 false。
    bool ok = true;
    /*
    Notes:杨小兵-2025-07-20

    1、使用参数一 file 创建一个 gguf_reader 对象，从而使用其成员函数读取解析 GGUF 文件。
    2、创建一个 gguf_context 对象 ctx，用于存储 GGUF 文件的上下文信息，使用指针 ctx 指向该对象，将 GGUF 格式文件中保存的信息读取到内存中
    自定义的 C++ 对象。
    */


    // file magic
    {
        //  创建一个字节类型的动态数组用来保存从 GGUF 格式文件中读取出来的前四个字节内容。
        std::vector<char> magic;
        //  读取 GGUF 格式文件中的前四个字节内容到 magic 动态数组中，根据读取结果来设置变量 ok 的值。
        ok = ok && gr.read(magic, 4);
        //  如果读取 GGUF 格式文件的前四个字节内容失败，则输出错误信息、释放已经分配的 gguf_context 对象 ctx，并返回 nullptr。
        if (!ok) {
            //  使用 fprintf 函数输出错误信息，表示读取 GGUF 格式文件的前四个字节内容失败。
            fprintf(stderr, "%s: failed to read magic\n", __func__);
            //  使用自定义函数 gguf_free 释放已经分配的 gguf_context 对象 ctx，整体的目标就是释放资源。
            gguf_free(ctx);
            //  返回 nullptr，表示读取 GGUF 格式文件的前四个字节内容失败。
            return nullptr;
        }
        //  执行到这里，说明已经成功读取 GGUF 格式文件的前四个字节内容到 magic 动态数组中，循环对 magic 中的每个字符进行检查。
        for (uint32_t i = 0; i < magic.size(); i++) {
            /*  如果 magic 动态数组中的当前字符不等于 GGUF_MAGIC 中定义的对应字符，则输出错误信息、释放已经分配的gguf_context
            对象 ctx，并返回 nullptr。
            */
            if (magic[i] != GGUF_MAGIC[i]) {
                //  使用 fprintf 函数输出错误信息，表示读取 GGUF 格式文件中的前四个字节并不是预期的 GGUF 四个字符。
                fprintf(stderr, "%s: invalid magic characters: '%c%c%c%c', expected 'GGUF'\n", __func__, magic[0], magic[1], magic[2], magic[3]);
                //  使用自定义函数 gguf_free 释放已经分配的 gguf_context 对象 ctx，整体的目标就是释放资源。
                gguf_free(ctx);
                //  返回 nullptr，表示读取 GGUF 格式文件的前四个字节内容失败。
                return nullptr;
            }
        }
    }
    /*
    Notes:杨小兵-2025-07-20

    1、这段代码块的整体作用就是读取 GGUF 格式文件的前四个字节内容，并检查这些字节是否符合预期的 GGUF 四个字符，其目的就是检查文件的 magic
    number 是否为 GGUF ？但是不将读取的前四个字节内容保存到 ctx 中。
    */

    // header
    int64_t n_kv      = 0;
    int64_t n_tensors = 0;

    // 如果前面读取 GGUF 格式文件的前四个字节内容成功，并且检查前四个字节内容符合预期的 GGUF 四个字符，则继续读取 GGUF 格式文件的版本信息。
    if (ok && gr.read(ctx->version)) {
        //  如果从 GGUF 格式文件中读取的版本信息为 1，则输出错误信息，表示 GGUFv1 已经不再支持，请使用更新的版本。
        if (ctx->version == 1) {
            //  使用 fprintf 函数输出错误信息，表示 GGUFv1 已经不再支持，请使用更新的版本。
            fprintf(stderr, "%s: GGUFv1 is no longer supported, please use a more up-to-date version\n", __func__);
            //  设置变量 ok 为 false，表示读取 GGUF 格式文件的版本信息失败。
            ok = false;
        }
        //  如果从 GGUF 格式文件中读取的版本信息大于 GGUF_VERSION，则输出错误信息，表示该 GGUF 文件的版本大于当前软件支持的最大版本。
        if (ctx->version > GGUF_VERSION) {
            //  使用 fprintf 函数输出错误信息，表示该 GGUF 文件的版本大于当前软件支持的最大版本。
            fprintf(stderr, "%s: this GGUF file is version %" PRIu32 " but this software only supports up to version %d\n",
                __func__, ctx->version, GGUF_VERSION);
            ok = false;
        }
        //  如果从 GGUF 格式文件中读取的版本信息不是上述两种情况，则继续执行后续的代码。
    } else {
        //  设置变量 ok 为 false，表示读取 GGUF 格式文件的版本信息失败。
        ok = false;
    }
    /*
    Notes:杨小兵-2025-07-16

    1、这段代码块的整体作用就是读取 GGUF 格式文件的版本信息，并检查该版本信息是否符合预期。
    2、读取 GGUF 格式文件中的内容时有一个关键点：从文件的当前位置读取对应类型大小的数据并填充到目标变量中，即文件指针会向前移动。
    */

    // 如果前面读取 GGUF 格式文件中的前四个字节内容和版本信息成功，则继续读取 GGUF 格式文件中的张量数量字段。
    if (ok && gr.read(n_tensors)) {
        /*
        Notes:杨小兵-2025-07-16

        1、static_assert(condition, message);
            1.1 condition：必须是编译时常量表达式，结果为bool类型
            1.2 message：字符串字面量，当断言失败时显示的错误消息
        2、 C++ 中编译时期将会进行的断言检查，确保 sizeof(size_t) 小于等于 8 且 sizeof(gguf_tensor_info) 大于等于 2，不会体现在最终
        代码层面。
        */
        static_assert(sizeof(size_t) <= 8 && sizeof(gguf_tensor_info) >= 2, "int64_t insufficient for indexing");
        //  检查从 GGUF 格式文件中读取的张量数量是否在合法范围内，即大于等于 0 且小于等于 SIZE_MAX/sizeof(gguf_tensor_info)。
        if (n_tensors < 0 || n_tensors > int64_t(SIZE_MAX/sizeof(gguf_tensor_info))) {
            //  如果从 GGUF 格式文件中读取的张量数量不在合法范围内，则输出错误信息。
            fprintf(stderr, "%s: number of tensors is %" PRIi64 " but must be in [0, %zu]\n",
                __func__, n_tensors, SIZE_MAX/sizeof(gguf_tensor_info));
            //  设置变量 ok 为 false，表示读取 GGUF 格式文件的信息失败。
            ok = false;
        }
    } else {
        //  如果读取 GGUF 格式文件中的张量数量字段失败，则将表示读取 GGUF 格式文件信息状态的变量设置为 false，表示读取 GGUF 格式文件失败。
        ok = false;
    }
    /*
    Notes:杨小兵-2025-07-16

    1、这段代码块的整体作用就是读取 GGUF 格式文件中的张量数量字段，并且检查该数量是否在合法范围内。
    2、读取 GGUF 格式文件中的内容时有一个关键点：从文件的当前位置读取对应类型大小的数据并填充到目标变量中，即文件指针会向前移动。
    */

    // 如果前面读取 GGUF 格式文件中的前四个字节内容、版本信息和张量数量字段成功，则继续读取 GGUF 格式文件中的 KV 对数量字段。
    if (ok && gr.read(n_kv)) {
        /*
        Notes:杨小兵-2025-07-16

        1、static_assert(condition, message);
            1.1 condition：必须是编译时常量表达式，结果为bool类型
            1.2 message：字符串字面量，当断言失败时显示的错误消息
        2、C++ 中编译时期将会进行的断言检查，确保 sizeof(size_t) 小于等于 8 且 sizeof(gguf_tensor_info) 大于等于 2，不会体现
        在最终代码层面。
        */
        static_assert(sizeof(size_t) <= 8 && sizeof(gguf_tensor_info) >= 2, "int64_t insufficient for indexing");
        //  检查从 GGUF 格式文件中读取的 KV 对数量是否在合法范围内，即大于等于 0 且小于等于 SIZE_MAX/sizeof(gguf_kv)。
        if (n_kv < 0 || n_kv > int64_t(SIZE_MAX/sizeof(gguf_kv))) {
            //  如果从 GGUF 格式文件中读取的 KV 对数量不在合法范围内，则输出错误信息。
            fprintf(stderr, "%s: number of key value pairs is %" PRIi64 " but must be in [0, %zu]\n",
                    __func__, n_kv, SIZE_MAX/sizeof(gguf_kv));
            //  设置变量 ok 为 false，表示读取 GGUF 格式文件的版本信息失败。
            ok = false;
        }
    } else {
        //  如果读取 GGUF 格式文件中的 KV 对数量字段失败，则将表示读取 GGUF 格式文件信息状态的变量设置为 false，表示读取 GGUF 格式文件失败。
        ok = false;
    }
    /*
    Notes:杨小兵-2025-07-16

    1、这段代码块的整体作用就是读取 GGUF 格式文件中的 KV 对数量字段，并且检查该数量是否在合法范围内。
    2、读取 GGUF 格式文件中的内容时有一个关键点：从文件的当前位置读取对应类型大小的数据并填充到目标变量中，即文件指针会向前移动。
    */

    /* 如果变量 ok 为 false，表示前面读取 GGUF 格式文件中的前四个字节内容、版本信息、张量数量字段和 KV 对数量字段某个失败了，则
    输出错误信息、释放已经分配的 gguf_context 对象 ctx，并返回 nullptr。
    */
    if (!ok) {
        //  使用 fprintf 函数输出错误信息，表示读取 GGUF 格式文件的头部信息失败，头部信息包含：魔数、版本信息、张量数量和 KV 对数量字段。
        fprintf(stderr, "%s: failed to read header\n", __func__);
        //  使用自定义函数 gguf_free 释放已经分配的 gguf_context 对象 ctx，整体的目标就是释放资源。
        gguf_free(ctx);
        //  返回 nullptr，即读取 GGUF 格式文件的头部信息失败。
        return nullptr;
    }
    /*
    Notes:杨小兵-2025-07-16

    1、如果读取 GGUF 格式文件中的前四个字节字段、版本信息字段、张量数量字段和 KV 对数量字段中的某个失败了，则相当于读取 GGUF 格式文件
    的头部信息失败了，整体的作用就是检查 GGUF 格式文件的头部信息读取是否成功。
    */

    // KV pairs 
    {
        //  代码执行到这里说明当前 GGUF 格式文件的头部信息读取成功，那么循环处理每一个 KV 对字段。
        for (int64_t i = 0; ok && i < n_kv; ++i) {
            //  创建一个字符串类型的变量用来存储当前 KV 对字段中的 key 值。
            std::string key;
            /*
                创建一个枚举类型的变量并且将其初始化为无效值，对于 C++ 17 来说如果枚举声明中没有对应的值，那么默认情况下则是无效的，这里
            不需要去纠结具体的语法问题，只需要知道达到的效果。
            */
            gguf_type   type     = gguf_type(-1);
            //  创建一个布尔类型的变量 is_array 并且将其初始化为 false，表示当前 KV 对字段中的值不是数组类型。
            bool        is_array = false;
            //  创建一个无符号整数类型的变量 n 并且将其初始化为 1，表示当前 KV 对字段中的值的元素数量为 1。
            uint64_t    n        = 1;

            //  使用 C++ 支持的异常处理机制来捕获可能发生的异常。
            try {
                /*
                    如果当前 GGUF 格式文件的头部信息读取成功，并且读取当前 GGUF 格式文件中的 key 值成功，则将其存储到key 字符串变量中，
                在这个过程中可能会抛出 std::length_error 或 std::bad_alloc 异常，指向文件的指针将会向前移动，读取的内容将会被存储到
                key 中，并且将目前的读取状态设置成 TRUE。
                */
                ok = ok && gr.read(key);
            } catch (std::length_error &) {
                //  如果捕获到 std::length_error 异常，则输出错误信息，表示读取 key 时发生了长度错误。
                fprintf(stderr, "%s: encountered length_error while reading key %" PRIi64 "\n", __func__, i);
                ok = false;
            } catch (std::bad_alloc &) {
                //  如果捕获到 std::bad_alloc 异常，则输出错误信息，表示读取 key 时发生了内存分配错误。
                fprintf(stderr, "%s: encountered bad_alloc error while reading key %" PRIi64 "\n", __func__, i);
                ok = false;
            }
            //  代码执行到这里，说明已经成功读取了 GGUF 格式文件中当前 key 值的内容到 key 字符串变量中。
            for (size_t j = 0; ok && j < ctx->kv.size(); ++j) {
                //  检查当前 key 值是否与之前的 KV 对中的 key 值重复，如果重复则输出错误信息并设置变量 ok 为 false。
                if (key == ctx->kv[j].key) {
                    //  使用 fprintf 函数输出错误信息，表示当前 key 值与之前的 KV 对中的 key 值重复。
                    fprintf(stderr, "%s: duplicate key '%s' for tensors %zu and %" PRIi64 " \n", __func__, key.c_str(), j, i);
                    //  设置变量 ok 为 false，表示读取 GGUF 格式文件的 KV 对字段失败。
                    ok = false;
                }
            }
            //  如果读取 GGUF 格式文件状态为 false，则表示读取 GGUF格式文件内容失败，跳出当前循环。
            if (!ok) {
                break;
            }
            //  如果读取 GGUF 头文件信息、当前 key 值成功，那么继续读取当前 KV 对字段中的类型信息。
            ok = ok && gr.read(type);
            //  如果读取到的类型信息为 GGUF_TYPE_ARRAY，则表示当前 KV 对字段中的值是一个数组类型。
            if (type == GGUF_TYPE_ARRAY) {
                //  将 is_array 设置为 true，表示当前 KV 对字段中的值是一个数组类型。
                is_array = true;
                //  读取标志数组元素类型的类型信息，并且设置读取 GGUF 格式文件状态。
                ok = ok && gr.read(type);
                //  读取数组元素数量 n，并且设置读取 GGUF 格式文件状态。
                ok = ok && gr.read(n);
            }
            //  如果读取 GGUF 格式文件状态为 false，则表示读取 GGUF格式文件内容失败，跳出当前循环。
            if (!ok) {
                break;
            }
            /*
                根据不同的类型信息 type，调用不同的模板函数 gguf_read_emplace_helper 来读取当前 KV 对字段中的值，并且将其存储到 ctx->kv 中，
            这里的 gguf_read_emplace_helper 函数就是将从 GGUF 格式文件中读取的 KV 对字段中的值存储到 ctx 中，即内存中表示 GGUF 格式文件的
            C++ 对象。
            */
            switch (type) {
                case GGUF_TYPE_UINT8:   ok = ok && gguf_read_emplace_helper<uint8_t>    (gr, ctx->kv, key, is_array, n); break;
                case GGUF_TYPE_INT8:    ok = ok && gguf_read_emplace_helper<int8_t>     (gr, ctx->kv, key, is_array, n); break;
                case GGUF_TYPE_UINT16:  ok = ok && gguf_read_emplace_helper<uint16_t>   (gr, ctx->kv, key, is_array, n); break;
                case GGUF_TYPE_INT16:   ok = ok && gguf_read_emplace_helper<int16_t>    (gr, ctx->kv, key, is_array, n); break;
                case GGUF_TYPE_UINT32:  ok = ok && gguf_read_emplace_helper<uint32_t>   (gr, ctx->kv, key, is_array, n); break;
                case GGUF_TYPE_INT32:   ok = ok && gguf_read_emplace_helper<int32_t>    (gr, ctx->kv, key, is_array, n); break;
                case GGUF_TYPE_FLOAT32: ok = ok && gguf_read_emplace_helper<float>      (gr, ctx->kv, key, is_array, n); break;
                case GGUF_TYPE_BOOL:    ok = ok && gguf_read_emplace_helper<bool>       (gr, ctx->kv, key, is_array, n); break;
                case GGUF_TYPE_STRING:  ok = ok && gguf_read_emplace_helper<std::string>(gr, ctx->kv, key, is_array, n); break;
                case GGUF_TYPE_UINT64:  ok = ok && gguf_read_emplace_helper<uint64_t>   (gr, ctx->kv, key, is_array, n); break;
                case GGUF_TYPE_INT64:   ok = ok && gguf_read_emplace_helper<int64_t>    (gr, ctx->kv, key, is_array, n); break;
                case GGUF_TYPE_FLOAT64: ok = ok && gguf_read_emplace_helper<double>     (gr, ctx->kv, key, is_array, n); break;
                case GGUF_TYPE_ARRAY:
                default:
                    {
                        //  使用 fprintf 函数输出错误信息，表示当前 KV 对字段中的 key 值有无效的 GGUF 类型。
                        fprintf(stderr, "%s: key '%s' has invalid GGUF type %d\n", __func__, key.c_str(), type);
                        //  设置读取 GGUF 格式文件状态为 false，表示读取 GGUF 格式文件中当前 KV 对字段失败。
                        ok = false;
                    } break;
            }
        }
        //  如果读取 GGUF 格式文件状态为false，则表示读取 GGUF 格式文件中当前 KV 对字段失败。
        if (!ok) {
            //  使用 C++ 标准库中的 fprintf 函数输出错误信息，表示读取 GGUF 格式文件中的 key-value 对失败。
            fprintf(stderr, "%s: failed to read key-value pairs\n", __func__);
            //  失败后释放已经分配的 gguf_context 对象 ctx，整体的目标就是释放资源。
            gguf_free(ctx);
            //  直接返回 nullptr
            return nullptr;
        }
        /*
            代码执行到这里，说明已经成功读取 GGUF 格式文件中的 key-value 对，并且将其存储到 ctx->kv 中，这里使用自定义的 GGML_ASSERT 宏
        来检查 ctx->kv 中的 KV 对数量是否与从 GGUF 格式文件中读取的 KV 对数量 n_kv 相等，如果不相等则输出错误信息并终止程序。
        */
        GGML_ASSERT(int64_t(ctx->kv.size()) == n_kv);

        //  在 GGUF 格式文件的上下文对象中查找是否存在 general.alignment 名称的 Key 值，如果存在返回其在 KV 对字段中的索引，反之则返回 -1。
        const int alignment_idx = gguf_find_key(ctx, GGUF_KEY_GENERAL_ALIGNMENT);
        /*
            如果没有在 GGUF 格式文件的上下文对象中查找到 general.alignment 名称的 Key 值，那么需要更新 GGUF 格式文件的上下文对象中的
        alignment 字段值。
        */
        ctx->alignment = alignment_idx == -1 ? GGUF_DEFAULT_ALIGNMENT : gguf_get_val_u32(ctx, alignment_idx);

        /*
        Notes:杨小兵-2025-07-20

        1、如果 GGUF 格式文件的上下文对象中的 alignment 字段为零或者
        2、假设 alignment 字段值为 3，则其二进制表示为 11 ，alignment - 1 字段值为 2 ，其二进制表示为 10，那么 alignment
        和 alignment - 1 之间的逐位与为 01，其结果不等于零，说明 alignment 字段值不是 2 的幂次方，可以通过具体的位级表示
        来验证该算法。
        3、这里学习到了一种检查一个数是否为 2 的幂次方的算法：如果一个数 n 的二进制表示中只有一个位为 1，那么 n & (n - 1) 的结果为 0。
            3.1 情况一：如果 n 为 0，则 n & (n - 1) 的结果为 0。
            3.2 情况二：如果 n 为 2 的幂次方，则 n & (n - 1) 的结果为 0。
        4、“alignment（对齐）”指的是把张量数据与文件或内存地址的某个固定边界（通常 32 或 64 字节）对齐。这样做既能让 CPU/GPU 一次取到
        完整 SIMD 块、保证量化块整数倍读取，也能让同一 GGUF 文件在不同系统上被 mmap / 直接 SEEK-READ 而无需逐张量 copy。这样做的最终
        目的就是为了能够提高数据读取的效率和性能，尤其是在处理大规模数据时。
        */
        if (ctx->alignment == 0 || (ctx->alignment & (ctx->alignment - 1)) != 0) {
            //  使用 C++ 标准库中的 fprintf 函数输出错误信息，表示 GGUF 格式文件的上下文对象中的 alignment 字段值不是 2 的幂次方。
            fprintf(stderr, "%s: alignment %zu is not a power of 2\n", __func__, ctx->alignment);
            //  释放已经分配的 gguf_context 对象 ctx，整体的目标就是释放资源。
            gguf_free(ctx);
            //  直接返回。
            return nullptr;
        }
    }
    /*
    Notes:杨小兵-2025-07-16

    1、上述代码块的整体作用就是读取 GGUF 格式文件中的 key-value 对，并且将其存储到 ctx->kv 中，同时检查 GGUF 格式文件的上下文对象中的
    alignment 字段值是否为 2 的幂次方，如果不是则输出错误信息并释放已经分配的 gguf_context 对象 ctx。
    */

    // read the tensor info
    //  如果前面读取 GGUF 格式文件中的前四个字节内容、版本信息、张量数量字段和 KV 对数量字段成功，则循环处理每一个张量信息字段。
    for (int64_t i = 0; ok && i < n_tensors; ++i) {
        //  创建一个 gguf_tensor_info 结构体变量 info，用来存储当前处理的张量信息。
        struct gguf_tensor_info info;

        // tensor name
        {
            //  创建一个字符串类型的变量 name，用来存储当前处理的张量名称。
            std::string name;
            //  使用 C++ 中的 try-catch 语句来捕获可能发生的异常。
            try {
                //  如果当前 GGUF 格式文件中的信息经过前面的读取步骤成功，并且读取当前张量名称成功，则将其存储到 name 字符串变量中。
                ok = ok && gr.read(name);
            } catch (std::length_error &) {
                //  如果捕获到 std::length_error 异常，则输出错误信息，表示读取张量名称时发生了长度错误。
                fprintf(stderr, "%s: encountered length_error while reading tensor name %" PRIi64 "\n", __func__, i);
                //  将 GGUF 格式文件读取状态设置成 false，表示读取 GGUF 格式文件中的信息失败。
                ok = false;
            } catch (std::bad_alloc &) {
                //  如果捕获到 std::bad_alloc 异常，则输出错误信息，表示读取张量名称时发生了内存分配错误。
                fprintf(stderr, "%s: encountered bad_alloc error while reading tensor name %" PRIi64 "\n", __func__, i);
                //  将 GGUF 格式文件读取状态设置成 false，表示读取 GGUF 格式文件中的信息失败。
                ok = false;
            }
            //  如果读取到 GGUF 格式文件中的张量名称长度超过了 GGML_MAX_NAME，则输出错误信息。
            if (name.length() >= GGML_MAX_NAME) {
                //  使用 fprintf 函数输出错误信息，表示当前张量名称长度超过了 GGML_MAX_NAME。
                fprintf(stderr, "%s: tensor name %" PRIi64 " is too long: %zu >= %d\n", __func__, i, name.length(), GGML_MAX_NAME);
                //  将 GGUF 格式文件读取状态设置成 false，表示读取 GGUF 格式文件中的信息失败。
                ok = false;
                //  直接跳出当前循环。
                break;
            }
            //  代码执行到这里，说明已经成功读取 GGUF 格式文件中的当前张量名称，将其存储到临时张量信息结构体变量 info 中。
            ggml_set_name(&info.t, name.c_str());

            // make sure there are no duplicate tensor names
            //  如果 GGUF 格式文件经过前面的读取步骤成功，并且在已经从 GGUF 格式文件中读取的张量信息中查找是否有重复的张量名称。
            for (int64_t j = 0; ok && j < i; ++j) {
                //  检查当前从 GGUF 格式文件中读取的张量名称是否与之前从 GGUF 格式文件中读取的张量名称重复，如果重复则输出错误信息。
                if (strcmp(info.t.name, ctx->info[j].t.name) == 0) {
                    //  使用 fprintf 函数输出错误信息，表示当前张量名称与之前的张量名称重复。
                    fprintf(stderr, "%s: duplicate tensor name '%s' for tensors %" PRIi64 " and %" PRIi64 "\n", __func__, info.t.name, j, i);
                    //  将 GGUF 格式文件读取状态设置成 false，表示读取 GGUF 格式文件中的信息失败。
                    ok = false;
                    //  直接跳出当前循环。
                    break;
                }
            }
        }
        if (!ok) {
            //  如果从 GGUF 格式文件中读取当前张量名称失败，则跳出读取张量信息的循环。
            break;
        }
        /*
        Notes:杨小兵-2025-07-16

        1、从 GGUF 格式文件中读取出当前张量名称，并且检查该张量名称是否符合 GGML_MAX_NAME 的限制，同时检查该张量名称是否与之前
        的张量名称重复。在这个读取过程中，指向 GGUF 格式文件相关的指针将会随之向后移动。如果一切读取顺利的话，那么将会把从 GGUF
        格式文件中读取的当前张量名称存储到临时张量信息结构体变量 info 中。
        */

        // tensor shape
        {
            //  创建一个临时变量 n_dims 用来存储当前张量的维度数量，并且将其初始化为 -1，表示当前张量的维度数量未知。
            uint32_t n_dims = -1;
            //  如果当前 GGUF 格式文件中的信息经过前面的读取步骤成功，并且读取当前张量的维度数量成功，则将其存储到 n_dims 变量中。
            ok = ok && gr.read(n_dims);
            //  如果从 GGUF 格式文件中读取的张量维度数量大于 GGML_MAX_DIMS，则输出错误信息。
            if (n_dims > GGML_MAX_DIMS) {
                //  使用 fprintf 函数输出错误信息，表示当前张量的维度数量超过了 GGML_MAX_DIMS。
                fprintf(stderr, "%s: tensor '%s' has invalid number of dimensions: %" PRIu32 " > %" PRIu32 "\n",
                    __func__, info.t.name, n_dims, GGML_MAX_DIMS);
                //  将 GGUF 格式文件读取状态设置成 false，表示读取 GGUF 格式文件中的信息失败。
                ok = false;
                //  跳出读取张量信息的循环
                break;
            }
            //  如果当前 GGUF 格式文件中的信息经过前面的读取步骤成功，循环读取当前张量各个维度的元素数量，并且将其存储到 info.t.ne 中。
            for (uint32_t j = 0; ok && j < GGML_MAX_DIMS; ++j) {
                //  将临时张量信息结构体变量 info 中的张量当前维度的元素数量初始化为 1，表示当前维度的元素数量为 1。
                info.t.ne[j] = 1;
                //  如果当前索引小于 n_dims，则表示当前维度的元素数量有效，继续读取当前维度的元素数量。
                if (j < n_dims) {
                    //  如果当前 GGUF 格式文件中的信息经过前面的读取步骤成功，并且读取当前维度的元素数量成功，则将其存储到 info.t.ne[j] 中。
                    ok = ok && gr.read(info.t.ne[j]);
                }

                // check that all ne are non-negative
                //  对于每个维度的元素数量进行检查，如果当前维度的元素数量小于 0，则输出错误信息。
                if (info.t.ne[j] < 0) {
                    //  使用 fprintf 函数输出错误信息，表示当前张量的维度 j 的元素数量小于 0。
                    fprintf(stderr, "%s: tensor '%s' dimension %" PRIu32 " has invalid number of elements: %" PRIi64 " < 0\n",
                        __func__, info.t.name, j, info.t.ne[j]);
                    //  将 GGUF 格式文件读取状态设置成 false，表示读取 GGUF 格式文件中的信息失败。
                    ok = false;
                    //  跳出读取当前张量各个维度元素数量的循环。
                    break;
                }
            }

            /*
            Notes:杨小兵-2025-07-17

            1、在 GGML 内部，一个张量使用 ne[4] 数组存放各维度的元素个数（Number of Elements），nb[4] 存放各维度的字节步长
            （Number of Bytes per stride），因此正确地解析 ne 是后续计算张量大小、定位数据偏移的前提。
            2、GGUF 解析函数 gguf_init_from_file_impl 会逐个读取文件中的张量描述并存入 info.t.ne[]；随后才会用 ggml_nbytes()
            之类的工具把“元素数 × 单元素字节数”累加到总模型大小里。
            3、溢出检测技巧
                3.1 对正整数 a × b 进行安全乘法，常见写法是先判断 a > MAX/b；若为真则乘法一定溢出。
                3.2 正整数乘法溢出检测技巧在 CSAPP 中有介绍，可以回顾复习。
            4、为什么必须这样做？
                4.1 张量维度来自文件，完全可能被恶意或损坏的 GGUF 设置成极大值。
                4.2 如果不做溢出判断，ne[0] * ne[1] * … 在 64 位有符号整数上回卷为很小的正数；后面根据该“假尺寸”去 malloc，得到
                的缓冲区比真实数据小得多，写入/读取即发生越界。
                4.3 2025-07-10 官方发布的安全公告就披露了 “GGUF 解析整数溢出→堆缓冲区越界” 漏洞，根因正是累积大小时缺乏上限检测。
                当前片段正是修补漏洞的防御代码。
            5、这段 GGUF 解析代码通过除法-再-比较的经典技巧，在读文件时预判张量维度乘积是否会超出 int64_t 的表达范围，提前报错并
            终止解析，避免了后续因为整数溢出导致的内存分配错误和潜在的安全漏洞。
            */
            // check that the total number of elements is representable
            if (ok && ((INT64_MAX/info.t.ne[1] <= info.t.ne[0]) ||
                       (INT64_MAX/info.t.ne[2] <= info.t.ne[0]*info.t.ne[1]) ||
                       (INT64_MAX/info.t.ne[3] <= info.t.ne[0]*info.t.ne[1]*info.t.ne[2]))) {

                //  使用 fprintf 函数输出错误信息，表示当前张量的总元素数量超过了 int64_t 的最大值。
                fprintf(stderr, "%s: total number of elements in tensor '%s' with shape "
                    "(%" PRIi64 ", %" PRIi64 ", %" PRIi64 ", %" PRIi64 ") is >= %" PRIi64 "\n",
                    __func__, info.t.name, info.t.ne[0], info.t.ne[1], info.t.ne[2], info.t.ne[3], INT64_MAX);
                //  将 GGUF 格式文件读取状态设置成 false，表示读取 GGUF 格式文件中的信息失败。
                ok = false;
                //  跳出读取张量信息的循环。
                break;
            }
        }
        //  如果读取 GGUF 格式文件状态不是 true ，那么跳出读取张量信息的循环
        if (!ok) {
            break;
        }
        /*
        Notes:杨小兵-2025-07-18

        1、从 GGUF 格式文件中读取出当前张量的维度数量、各个维度的元素数量，并且检查这些元素数量是否符合 GGML_MAX_DIMS 的限制。从整体上
        来看就是为了从 GGUF 格式文件中读取出当前张量的形状信息，并且检查这些形状信息是否符合 GGML_MAX_DIMS 的限制。
        2、这里有关张量的步长等计算还没有完全理解透彻，后续会继续学习相关内容。
        */

        // tensor type
        {
            //  如果当前 GGUF 格式文件中的信息经过前面的读取步骤成功，并且读取当前张量的类型信息成功，则将其存储到 info.t.type 中。
            ok = ok && gr.read(info.t.type);

            // check that tensor type is within defined range
            //  如果从 GGUF 格式文件中的读取的当前张量类型信息不在 GGML_TYPE_COUNT 的范围内，则输出错误信息。
            if (info.t.type < 0 || info.t.type >= GGML_TYPE_COUNT) {
                //  使用 C++ 标准库中的 fprintf 函数输出错误信息，表示当前张量的类型信息不在 GGML_TYPE_COUNT 的范围内。
                fprintf(stderr, "%s: tensor '%s' has invalid ggml type %d (%s)\n",
                    __func__, info.t.name, info.t.type, ggml_type_name(info.t.type));
                //  将 GGUF 格式文件读取状态设置成 false，表示读取 GGUF 格式文件中的信息失败。
                ok = false;
                //  跳出读取张量信息的循环
                break;
            }
            //  获取当前张量类型的大小，即当前张量类型所占用的字节数，并且将其存储到 size_t 类型的变量 type_size 中。
            const size_t  type_size = ggml_type_size(info.t.type);
            //  获取当前张量类型的块大小，即当前张量类型的每个块所占用的字节数，并且将其存储到 int64_t 类型的变量 blck_size 中。
            const int64_t blck_size = ggml_blck_size(info.t.type);

            // check that row size is divisible by block size
            //  如果当前张量类型的块大小为0，或者当前张量的第一个维度的元素数量不是块大小的倍数，则输出错误信息。
            if (blck_size == 0 || info.t.ne[0] % blck_size != 0) {
                //  使用 C++ 标准库中的 fprintf 函数输出错误信息，表示当前张量的第一个维度的元素数量不是块大小的倍数。
                fprintf(stderr, "%s: tensor '%s' of type %d (%s) has %" PRId64 " elements per row, "
                    "not a multiple of block size (%" PRId64 ")\n",
                    __func__, info.t.name, (int) info.t.type, ggml_type_name(info.t.type), info.t.ne[0], blck_size);
                //  将 GGUF 格式文件读取状态设置成 false，表示读取 GGUF 格式文件中的信息失败。
                ok = false;
                //  跳出读取张量信息的循环
                break;
            }

            /*
            Notes:杨小兵-2025-07-18

            1、这里对给定形状、类型的张量，计算每个维度的字节步长（nb）是为了后续在读取张量数据时能够正确地定位到每个元素。
            */
            // calculate byte offsets given the tensor shape and type
            //  计算当前张量的第零个维度的字节步长，即张量类型的字节大小。
            info.t.nb[0] = type_size;
            //  计算当前张量的第一个维度的字节步长，即第一个维度的元素数量乘以当前张量类型的大小除以块大小。
            info.t.nb[1] = info.t.nb[0]*(info.t.ne[0]/blck_size);
            //  如果当前张量的维度数量大于1，则计算当前张量的第二个维度的字节步长，即第一个维度的字节步长乘以第二个维度的元素数量。
            for (int j = 2; j < GGML_MAX_DIMS; ++j) {
                info.t.nb[j] = info.t.nb[j - 1]*info.t.ne[j - 1];
            }
        }
        //  如果读取 GGUF 格式文件状态不是 true ，那么跳出读取张量信息的循环。
        if (!ok) {
            //  跳出读取张量信息的循环
            break;
        }
        /*
        Notes:杨小兵-2025-07-18

        1、从 GGUF 格式文件中读取出当前张量的类型信息，并且计算当前张量的各个维度的字节步长信息。这里的字节步长信息是为了后续在读取张量
        数据时能够正确地定位到每个元素，对这部分内容现在还没有完全理解透彻。
        */

        // tensor data offset within buffer
        //  如果当前 GGUF 格式文件中的信息经过前面的读取步骤成功，并且读取当前张量的数据偏移量成功，则将其存储到 info.offset 中。
        ok = ok && gr.read(info.offset);

        //  将从 GGUF 格式文件中的读取的张量信息存储到 ctx->info 中。
        ctx->info.push_back(info);
    }
    /*
    Notes:杨小兵-2025-07-16

    1、循环从 GGUF 格式文件中读取每个张量的信息，包括张量名称、维度数量、各个维度的元素数量、张量类型和数据偏移量等信息，并且将
    这些信息存储到 ctx->info 中。
    */

    //  如果读取 GGUF 格式文件的状态不是 true ，那么输出错误信息并跳出读取张量信息的循环。
    if (!ok) {
        //  使用 C++ 标准库中的 fprintf 函数输出错误信息，表示读取 GGUF 格式文件中的张量信息失败。
        fprintf(stderr, "%s: failed to read tensor info\n", __func__);
        //  释放已经分配的 gguf_context 对象 ctx，整体的目标就是释放资源。
        gguf_free(ctx);
        //  直接返回。
        return nullptr;
    }
    /*
        检查读取到的实际张量数量是否与从 GGUF 格式文件中存储的张量数量 n_tensors 相等，如果不相等则输出错误信息并释放已经分配
    的 gguf_context 对象 ctx。
    */
    GGML_ASSERT(int64_t(ctx->info.size()) == n_tensors);

    // we require the data section to be aligned, so take into account any padding
    /*
    Notes:杨小兵-2025-07-18

    1、因为 GGUF 格式文件中要求数据部分对齐，因此要考虑任何填充，如果知道了填充的大小，那么就可以将文件指针移动到数据部分的开始位置。
    这样便可以对存储在 GGUF 格式文件中的张量数据进行正确的读取和处理。
    2、下列 fseek 函数的目的就是为了将文件指针移动到数据部分的开始位置，这样便可以对存储在 GGUF 格式文件中的张量数据进行正确的读取和处理。
    具体的实现细节目前不需要进行考虑，只需要知道其达到的效果即可。
    */
    if (fseek(file, GGML_PAD(ftell(file), ctx->alignment), SEEK_SET) != 0) {
        //  使用 fprintf 函数输出错误信息，表示无法将文件指针移动到数据部分的开始位置。
        fprintf(stderr, "%s: failed to seek to beginning of data section\n", __func__);
        //  释放已经分配的 gguf_context 对象 ctx，整体的目标就是释放资源。
        gguf_free(ctx);
        //  直接返回。
        return nullptr;
    }

    // store the current file offset - this is where the data section starts
    //  记录当前文件指针的位置，即数据部分的开始位置。
    ctx->offset = ftell(file);

    // compute the total size of the data section, taking into account the alignment
    //  计算数据部分的总大小，同时考虑对齐。
    {
        //  将 ctx 对象中的 size 成员设置成零。
        ctx->size = 0;
        //  循环对每一个张量信息中的数据部分大小进行计算
        for (size_t i = 0; i < ctx->info.size(); ++i) {
            //  创建一个引用对象，即 ctx 对象中的当前张量信息，需要注意的是这里的 ctx 就是 GGUF 格式文件被自定义结构之后的“内存表示”。
            const gguf_tensor_info & ti = ctx->info[i];
            //  如果当前张量信息中的偏移量和 ctx 对象中的 size 大小是不相等的，那么输出错误信息。
            if (ti.offset != ctx->size) {
                //  使用 fprintf 函数输出错误信息，表示当前张量信息中的偏移量和 ctx 对象中的 size 大小不相等。
                fprintf(stderr, "%s: tensor '%s' has offset %" PRIu64 ", expected %zu\n",
                    __func__, ti.t.name, ti.offset, ctx->size);
                fprintf(stderr, "%s: failed to read tensor data\n", __func__);
                //  释放已经分配的 gguf_context 对象 ctx，整体的目标就是释放资源。
                gguf_free(ctx);
                //  直接返回。
                return nullptr;
            }
            /*
                将当前张量信息中的数据部分大小（字节数、考虑对齐）累加到 ctx 对象中的 size 成员中，从这里可以知道 ctx 中的 size 用来表示
            GGUF 格式文件中的张量数据部分的总大小。
            */
            ctx->size += GGML_PAD(ggml_nbytes(&ti.t), ctx->alignment);
        }
    }
    /*
    Notes:杨小兵-2025-07-16

    1、计算 GGUF 格式文件中所有张量的数据部分的总大小。
    2、注意
        2.1 这里的大小指的是字节数量。
        2.2 考虑对齐。
    */

    // load the tensor data only if requested
    /*
        直接当真正需要使用张量数据的时候才加载具体的张量数据，这算是一种延迟加载的策略，参数 params 相当于是一种配置，用来控制读取 GGUF 格式
    文件过程中的行为。
    */
    if (params.ctx != nullptr) {
        /*
        Notes:杨小兵-2025-07-20

        1、当传入的 gguf_context 标志为 no_alloc（不分配内存）时，仅仅创建一组“空”的张量（ggml_tensor 结构体），但不去读取或加载它们
        的实际数据。否则（即允许分配内存的情况），会把整个二进制大块数据（binary blob）读入到 ggml_context 管理的内存中，并让每个张量
        的 data 指针指向这块大数据中对应存储它们数据的位置。
            1.1 这里的一个关键点是 GGUF 格式文件中的张量数据是以二进制 blob 的形式存储的，那么为了将其读取到内存中，就需要创建一个自定义
        的 C++ 对象，即 ggml_context，用来表示张量内存表示。这个 ggml_context 对象会包含所有张量的元数据（如名称、形状、类型等）
        以及实际的数据部分（即二进制 blob）。
            1.2 ggml_context 就是当前 GGUF 格式文件中所有张量的内存表示，它包含了所有张量的元数据和实际的数据部分？
        2、“Blob” 在这里指的是一整块连续的二进制数据区域，用来一次性存放模型的所有权重、偏置等张量数据。它本质上只是一个 uint8_t*（字节
        指针），后面通过偏移量来定位每个张量的起始地址和大小。
        */

        // if the provided gguf_context is no_alloc, then we create "empty" tensors and do not read the binary blob
        // otherwise, we load the binary blob into the created ggml_context as well, and point the "data" members of
        //   the ggml_tensor structs to the appropriate locations in the binary blob

        // compute the exact size needed for the new ggml_context

        /*
        Notes:杨小兵-2025-07-20

        1、计算在分配 ggml_context 所需要内存的确切大小，张量元数据本身所需要的字节大小 + 张量数据部分的字节大小。
        2、这里的 mem_size 变量表示的是 ggml_context 所需要的内存大小，根据不同的需求分为两种情况：
            2.1 如果 no_alloc 标志为 true，则表示不分配内存，仅仅创建一组“空”的张量（ggml_tensor 结构体），那么 mem_size 就是
            n_tensors * ggml_tensor_overhead()，即每个张量的元数据所需要的字节大小乘以张量数量。
            2.2 如果 no_alloc 标志为 false，则表示需要分配内存来存储张量数据，那么 mem_size 就是 (n_tensors + 1) * ggml_tensor_overhead() + ctx->size，
            即每个张量的元数据所需要的字节大小乘以张量数量加上 ctx->size（即所有张量的数据部分的总大小）。
        3、从 mem_size 变量的计算方式可以看出 ggml_context 对象表示的是 GGUF 格式文件中的所有张量内存表示而不是某一个张量的内存表示。
        */
        const size_t mem_size =
            params.no_alloc ?
            (n_tensors    )*ggml_tensor_overhead() :
            (n_tensors + 1)*ggml_tensor_overhead() + ctx->size;

        //  创建一个临时的 ggml_init_params 结构体用来保存初始化 ggml_context 所需要的参数。
        struct ggml_init_params pdata = {
            /*mem_size   =*/ mem_size,
            /*mem_buffer =*/ nullptr,
            /*no_alloc   =*/ params.no_alloc,
        };

        //  使用 ggml_init 函数来初始化 ggml_context，并且将其存储到传入的 params.ctx 中。
        *params.ctx = ggml_init(pdata);
        //  如果初始化 ggml_context 失败，则输出错误信息并释放已经分配的 gguf_context 对象 ctx。
        if (*params.ctx == nullptr) {
            //  使用 C++ 标准库中的 fprintf 函数输出错误信息，表示初始化 ggml_context 失败。
            fprintf(stderr, "%s: failed to initialize ggml context for storing tensors\n", __func__);
            //  释放已经分配的 gguf_context 对象 ctx，整体的目标就是释放资源。
            gguf_free(ctx);
            //  直接返回。
            return nullptr;
        }

        //  代码执行到这里，说明已经成功初始化 ggml_context，创建一个临时变量 ctx_data 用来保存 ggml_context 的指针。
        struct ggml_context * ctx_data = *params.ctx;

        //  创建一个临时变量用来保存执行张量数据在内存中的位置，初始值为 nullptr。
        struct ggml_tensor * data = nullptr;

        //  如果 no_alloc 标志不为 true，则表示需要分配内存来存储张量数据。
        if (!params.no_alloc) {
            //  使用 ggml_new_tensor_1d 函数来创建一个新的张量，张量存储数据的类型是 GGML_TYPE_I8（即 8 位整数），张量的大小为 ctx->size。
            data = ggml_new_tensor_1d(ctx_data, GGML_TYPE_I8, ctx->size);

            //  如果当前 GGUF 格式文件经过前面的读取步骤成功，并且创建张量 data 成功，那么将 GGUF 格式文件读取状态设置为 true。
            ok = ok && data != nullptr;

            //  如果 GGUF 格式文件读取状态是 true，则设置张量 data 的名称为 "GGUF tensor data binary blob"。
            if (ok) {
                //  使用 ggml_set_name 函数来设置张量 data 的名称为 "GGUF tensor data binary blob"。
                ggml_set_name(data, "GGUF tensor data binary blob");
            }

            // read the binary blob with the tensor data
            //  如果当前 GGUF 格式文件读取状态为 true，并且从 GGUF 格式文件中读取张量数据成功，则将读取的张量数据存储到 data->data 中。
            ok = ok && gr.read(data->data, ctx->size);

            //  如果当前 GGUF 格式文件读取状态不为 true 。
            if (!ok) {
                //  使用 C++ 标准库中的 fprintf 函数输出错误信息，表示读取张量数据的二进制 blob 失败。
                fprintf(stderr, "%s: failed to read tensor data binary blob\n", __func__);
                //  释放已经分配的 gguf_context 对象 ctx，整体的目标就是释放资源。
                ggml_free(ctx_data);
                //  将传入的 params.ctx 设置为 nullptr，表示没有有效的 ggml_context。
                *params.ctx = nullptr;
                //  释放已经分配的 gguf_context 对象 ctx，整体的目标就是释放资源。
                gguf_free(ctx);
                //  直接返回。
                return nullptr;
            }
            //  代码执行到这里，说明已经成功从 GGUF 格式文件中读取张量数据，并且将其存储到 data->data 中。
            ctx->data = data->data;
        }
        //  使用 ggml_set_no_alloc 函数来设置 ggml_context 的 no_alloc 标志为 true，表示不分配内存。
        ggml_set_no_alloc(ctx_data, true);

        // create the tensors
        //  循环在内存中创建每个张量所需要的 ggml_tensor 结构体，并且将其存储到 ctx->info 中。
        for (size_t i = 0; i < ctx->info.size(); ++i) {
            //  创建一个临时变量 info 用来引用 ctx->info 中的第 i 个张量信息，方便后续操作使用。
            const struct gguf_tensor_info & info = ctx->info[i];

            //  创建一个临时指针 cur 用来指向新创建的 ggml_tensor 结构体，使用 ggml_new_tensor 函数来创建一个新的张量。
            struct ggml_tensor * cur = ggml_new_tensor(ctx_data, info.t.type, GGML_MAX_DIMS, info.t.ne);

            //  如果当前 GGUF 格式文件读取状态为 true，并且新创建的张量 cur 不为 nullptr，则将当前 GGUF 格式文件读取状态设置为 true。
            ok = ok && cur != nullptr;
            //  如果当前 GGUF 格式文件读取状态不为 true，则直接跳出当前循环。
            if (!ok) {
                break;
            }

            //  使用 ggml_set_name 函数设置 cur 指向的张量的名称为 info.t.name，即当前张量信息中的名称。
            ggml_set_name(cur, info.t.name);

            // point the data member to the appropriate location in the binary blob using the tensor info
            //  如果参数 params.no_alloc 为 false，则表示需要分配内存来存储张量数据。
            if (!params.no_alloc) {
                //  将 cur 指向的张量的 data 成员指向 data->data + info.offset，即当前张量数据在二进制 blob 中的偏移位置。
                cur->data = (char *) data->data + info.offset;
            }
        }

        //  如果当前 GGUF 格式文件读取状态不为 true
        if (!ok) {
            //  使用 C++ 标准库中的 fprintf 函数输出错误信息，表示创建张量失败。
            fprintf(stderr, "%s: failed to create tensors\n", __func__);
            //  释放已经分配的 gguf_context 对象 ctx，整体的目标就是释放资源。
            ggml_free(ctx_data);
            //  将传入的 params.ctx 设置为 nullptr，表示没有有效的 ggml_context。
            *params.ctx = nullptr;
            //  释放已经分配的 gguf_context 对象 ctx，整体的目标就是释放资源。
            gguf_free(ctx);
            //  直接返回。
            return nullptr;
        }

        //  使用 ggml_set_no_alloc 函数来设置 ggml_context 的 no_alloc 标志为 false，表示允许分配内存。
        ggml_set_no_alloc(ctx_data, params.no_alloc);
    }

    //  返回创建的 gguf_context 对象 ctx。
    return ctx;
}

struct gguf_context * gguf_init_from_file(const char * fname, struct gguf_init_params params) {
    /*
        通过自定义的 ggml_fopen 函数打开 GGUF 格式文件 fname，并且以二进制模式读取文件内容，返回一个文件指针 file，该自定义函数为了实现
    在不同的操作系统平台上能够通过相同的接口打开指定文件，即做了跨平台操作。
    */
    FILE * file = ggml_fopen(fname, "rb");

    //  如果文件指针 file 为 nullptr，表示打开 GGUF 格式文件失败，则输出错误信息并返回 nullptr。
    if (!file) {
        //  使用 C++ 标准库中的 fprintf 函数输出错误信息，表示打开 GGUF 格式文件失败。
        fprintf(stderr, "%s: failed to open GGUF file '%s'\n", __func__, fname);
        //  返回 nullptr。
        return nullptr;
    }
    /*
    Notes:杨小兵-2025-07-16

    1、使用自定义的 ggml_fopen 函数来打开文件，ggml_fopen 函数是为了实现在不同的操作系统平台上都能打开模型文件，即针对不同的操作系统
    平台实现相同的功能。目前具体的细节先不需要了解，后续如果需要了解的话，可以查看 ggml_fopen 函数的实现。
    2、并且判断文件是否打开成功，如果打开失败，则在错误流中输出日志信息，并且返回 nullptr，反之则继续执行后续的代码。
    */

    /*
    1、通过 gguf_init_from_file_impl 函数从文件中读取 GGUF 格式的数据，并将其解析为 gguf_context 结构体。
    2、可以将 GGUF 格式的数据转换为 gguf_context 结构体，这样做是为了在后续的代码中可以方便地使用 gguf_context 结构体来访问
    GGUF 格式的数据。类似将文件中的数据转换为内存中的数据结构，从而更加方便地进行数据处理和访问。
    */
    struct gguf_context * result = gguf_init_from_file_impl(file, params);
    //  使用 C++ 标准库中的函数 fclose 来关闭文件指针 file，释放资源。
    fclose(file);
    //  返回解析后的 gguf_context 结构体指针 result。
    return result;
    /*
    Notes:杨小兵-2025-07-16

    1、函数 gguf_init_from_file_impl(file, params) 用于从文件中读取 GGUF 格式的数据，并将其解析为 gguf_context 结构体，即将 GGUF
    格式的数据转换为 gguf_context 结构体，这样做是为了在后续的代码中可以方便地使用 gguf_context 结构体来访问 GGUF 格式的数据。类似
    将文件中的数据转换为内存中的数据结构，从而更加方便地进行数据处理和访问。
    2、在读取 GGUF 格式的数据时，不论读取是否成功，都会关闭文件指针 file，并且返回解析后的 gguf_context 结构体指针 result，因为当前
    函数所需要的就是直接返回解析后的 gguf_context 结构体指针。
    */
}

void gguf_free(struct gguf_context * ctx) {
    if (ctx == nullptr) {
        return;
    }
    delete ctx;
}

const char * gguf_type_name(enum gguf_type type) {
    auto it = GGUF_TYPE_NAME.find(type);
    return it == GGUF_TYPE_NAME.end() ? nullptr : it->second;
}

uint32_t gguf_get_version(const struct gguf_context * ctx) {
    return ctx->version;
}

size_t gguf_get_alignment(const struct gguf_context * ctx) {
    return ctx->alignment;
}

size_t gguf_get_data_offset(const struct gguf_context * ctx) {
    return ctx->offset;
}

int64_t gguf_get_n_kv(const struct gguf_context * ctx) {
    return ctx->kv.size();
}

int64_t gguf_find_key(const struct gguf_context * ctx, const char * key) {
    // return -1 if key not found
    int64_t keyfound = -1;

    const int64_t n_kv = gguf_get_n_kv(ctx);

    for (int64_t i = 0; i < n_kv; ++i) {
        if (strcmp(key, gguf_get_key(ctx, i)) == 0) {
            keyfound = i;
            break;
        }
    }

    return keyfound;
}

const char * gguf_get_key(const struct gguf_context * ctx, int64_t key_id) {
    GGML_ASSERT(key_id >= 0 && key_id < gguf_get_n_kv(ctx));
    return ctx->kv[key_id].get_key().c_str();
}

enum gguf_type gguf_get_kv_type(const struct gguf_context * ctx, int64_t key_id) {
    GGML_ASSERT(key_id >= 0 && key_id < gguf_get_n_kv(ctx));
    return ctx->kv[key_id].is_array ? GGUF_TYPE_ARRAY : ctx->kv[key_id].get_type();
}

enum gguf_type gguf_get_arr_type(const struct gguf_context * ctx, int64_t key_id) {
    GGML_ASSERT(key_id >= 0 && key_id < gguf_get_n_kv(ctx));
    GGML_ASSERT(ctx->kv[key_id].is_array);
    return ctx->kv[key_id].get_type();
}

const void * gguf_get_arr_data(const struct gguf_context * ctx, int64_t key_id) {
    GGML_ASSERT(key_id >= 0 && key_id < gguf_get_n_kv(ctx));
    GGML_ASSERT(ctx->kv[key_id].get_type() != GGUF_TYPE_STRING);
    return ctx->kv[key_id].data.data();
}

const char * gguf_get_arr_str(const struct gguf_context * ctx, int64_t key_id, size_t i) {
    GGML_ASSERT(key_id >= 0 && key_id < gguf_get_n_kv(ctx));
    GGML_ASSERT(ctx->kv[key_id].get_type() == GGUF_TYPE_STRING);
    return ctx->kv[key_id].data_string[i].c_str();
}

size_t gguf_get_arr_n(const struct gguf_context * ctx, int64_t key_id) {
    GGML_ASSERT(key_id >= 0 && key_id < gguf_get_n_kv(ctx));

    if (ctx->kv[key_id].type == GGUF_TYPE_STRING) {
        return ctx->kv[key_id].data_string.size();
    }

    const size_t type_size = gguf_type_size(ctx->kv[key_id].type);
    GGML_ASSERT(ctx->kv[key_id].data.size() % type_size == 0);
    return ctx->kv[key_id].data.size() / type_size;
}

uint8_t gguf_get_val_u8(const struct gguf_context * ctx, int64_t key_id) {
    GGML_ASSERT(key_id >= 0 && key_id < gguf_get_n_kv(ctx));
    GGML_ASSERT(ctx->kv[key_id].get_ne() == 1);
    return ctx->kv[key_id].get_val<uint8_t>();
}

int8_t gguf_get_val_i8(const struct gguf_context * ctx, int64_t key_id) {
    GGML_ASSERT(key_id >= 0 && key_id < gguf_get_n_kv(ctx));
    GGML_ASSERT(ctx->kv[key_id].get_ne() == 1);
    return ctx->kv[key_id].get_val<int8_t>();
}

uint16_t gguf_get_val_u16(const struct gguf_context * ctx, int64_t key_id) {
    GGML_ASSERT(key_id >= 0 && key_id < gguf_get_n_kv(ctx));
    GGML_ASSERT(ctx->kv[key_id].get_ne() == 1);
    return ctx->kv[key_id].get_val<uint16_t>();
}

int16_t gguf_get_val_i16(const struct gguf_context * ctx, int64_t key_id) {
    GGML_ASSERT(key_id >= 0 && key_id < gguf_get_n_kv(ctx));
    GGML_ASSERT(ctx->kv[key_id].get_ne() == 1);
    return ctx->kv[key_id].get_val<int16_t>();
}

uint32_t gguf_get_val_u32(const struct gguf_context * ctx, int64_t key_id) {
    GGML_ASSERT(key_id >= 0 && key_id < gguf_get_n_kv(ctx));
    GGML_ASSERT(ctx->kv[key_id].get_ne() == 1);
    return ctx->kv[key_id].get_val<uint32_t>();
}

int32_t gguf_get_val_i32(const struct gguf_context * ctx, int64_t key_id) {
    GGML_ASSERT(key_id >= 0 && key_id < gguf_get_n_kv(ctx));
    GGML_ASSERT(ctx->kv[key_id].get_ne() == 1);
    return ctx->kv[key_id].get_val<int32_t>();
}

float gguf_get_val_f32(const struct gguf_context * ctx, int64_t key_id) {
    GGML_ASSERT(key_id >= 0 && key_id < gguf_get_n_kv(ctx));
    GGML_ASSERT(ctx->kv[key_id].get_ne() == 1);
    return ctx->kv[key_id].get_val<float>();
}

uint64_t gguf_get_val_u64(const struct gguf_context * ctx, int64_t key_id) {
    GGML_ASSERT(key_id >= 0 && key_id < gguf_get_n_kv(ctx));
    GGML_ASSERT(ctx->kv[key_id].get_ne() == 1);
    return ctx->kv[key_id].get_val<uint64_t>();
}

int64_t gguf_get_val_i64(const struct gguf_context * ctx, int64_t key_id) {
    GGML_ASSERT(key_id >= 0 && key_id < gguf_get_n_kv(ctx));
    GGML_ASSERT(ctx->kv[key_id].get_ne() == 1);
    return ctx->kv[key_id].get_val<int64_t>();
}

double gguf_get_val_f64(const struct gguf_context * ctx, int64_t key_id) {
    GGML_ASSERT(key_id >= 0 && key_id < gguf_get_n_kv(ctx));
    GGML_ASSERT(ctx->kv[key_id].get_ne() == 1);
    return ctx->kv[key_id].get_val<double>();
}

bool gguf_get_val_bool(const struct gguf_context * ctx, int64_t key_id) {
    GGML_ASSERT(key_id >= 0 && key_id < gguf_get_n_kv(ctx));
    GGML_ASSERT(ctx->kv[key_id].get_ne() == 1);
    return ctx->kv[key_id].get_val<bool>();
}

const char * gguf_get_val_str(const struct gguf_context * ctx, int64_t key_id) {
    GGML_ASSERT(key_id >= 0 && key_id < gguf_get_n_kv(ctx));
    GGML_ASSERT(ctx->kv[key_id].get_ne() == 1);
    return ctx->kv[key_id].get_val<std::string>().c_str();
}

const void * gguf_get_val_data(const struct gguf_context * ctx, int64_t key_id) {
    GGML_ASSERT(key_id >= 0 && key_id < gguf_get_n_kv(ctx));
    GGML_ASSERT(ctx->kv[key_id].get_ne() == 1);
    GGML_ASSERT(ctx->kv[key_id].get_type() != GGUF_TYPE_STRING);
    return ctx->kv[key_id].data.data();
}

int64_t gguf_get_n_tensors(const struct gguf_context * ctx) {
    return ctx->info.size();
}

int64_t gguf_find_tensor(const struct gguf_context * ctx, const char * name) {
    // return -1 if tensor not found
    int64_t tensor_id = -1;

    const int64_t n_tensors = gguf_get_n_tensors(ctx);

    for (int64_t i = 0; i < n_tensors; ++i) {
        if (strcmp(name, gguf_get_tensor_name(ctx, i)) == 0) {
            tensor_id = i;
            break;
        }
    }

    return tensor_id;
}

size_t gguf_get_tensor_offset(const struct gguf_context * ctx, int64_t tensor_id) {
    GGML_ASSERT(tensor_id >= 0 && tensor_id < gguf_get_n_tensors(ctx));
    return ctx->info[tensor_id].offset;
}

const char * gguf_get_tensor_name(const struct gguf_context * ctx, int64_t tensor_id) {
    GGML_ASSERT(tensor_id >= 0 && tensor_id < gguf_get_n_tensors(ctx));
    return ctx->info[tensor_id].t.name;
}

enum ggml_type gguf_get_tensor_type(const struct gguf_context * ctx, int64_t tensor_id) {
    GGML_ASSERT(tensor_id >= 0 && tensor_id < gguf_get_n_tensors(ctx));
    return ctx->info[tensor_id].t.type;
}

size_t gguf_get_tensor_size(const struct gguf_context * ctx, int64_t tensor_id) {
    GGML_ASSERT(tensor_id >= 0 && tensor_id < gguf_get_n_tensors(ctx));
    return ggml_nbytes(&ctx->info[tensor_id].t);
}

int64_t gguf_remove_key(struct gguf_context * ctx, const char * key) {
    const int64_t key_id = gguf_find_key(ctx, key);
    if (key_id >= 0) {
        ctx->kv.erase(ctx->kv.begin() + key_id);
    }
    return key_id;
}

template<typename T>
static void gguf_check_reserved_keys(const std::string & key, const T val) {
    if (key == GGUF_KEY_GENERAL_ALIGNMENT) {
        if constexpr (std::is_same<T, uint32_t>::value) {
            GGML_ASSERT(val > 0 && (val & (val - 1)) == 0 && GGUF_KEY_GENERAL_ALIGNMENT " must be power of 2");
        } else {
            GGML_ABORT(GGUF_KEY_GENERAL_ALIGNMENT " must be type u32");
        }
    }
}

void gguf_set_val_u8(struct gguf_context * ctx, const char * key, uint8_t val) {
    gguf_check_reserved_keys(key, val);
    gguf_remove_key(ctx, key);
    ctx->kv.emplace_back(key, val);
}

void gguf_set_val_i8(struct gguf_context * ctx, const char * key, int8_t val) {
    gguf_check_reserved_keys(key, val);
    gguf_remove_key(ctx, key);
    ctx->kv.emplace_back(key, val);
}

void gguf_set_val_u16(struct gguf_context * ctx, const char * key, uint16_t val) {
    gguf_check_reserved_keys(key, val);
    gguf_remove_key(ctx, key);
    ctx->kv.emplace_back(key, val);
}

void gguf_set_val_i16(struct gguf_context * ctx, const char * key, int16_t val) {
    gguf_check_reserved_keys(key, val);
    gguf_remove_key(ctx, key);
    ctx->kv.emplace_back(key, val);
}

void gguf_set_val_u32(struct gguf_context * ctx, const char * key, uint32_t val) {
    gguf_check_reserved_keys(key, val);
    gguf_remove_key(ctx, key);
    ctx->kv.emplace_back(key, val);
}

void gguf_set_val_i32(struct gguf_context * ctx, const char * key, int32_t val) {
    gguf_check_reserved_keys(key, val);
    gguf_remove_key(ctx, key);
    ctx->kv.emplace_back(key, val);
}

void gguf_set_val_f32(struct gguf_context * ctx, const char * key, float val) {
    gguf_check_reserved_keys(key, val);
    gguf_remove_key(ctx, key);
    ctx->kv.emplace_back(key, val);
}

void gguf_set_val_u64(struct gguf_context * ctx, const char * key, uint64_t val) {
    gguf_check_reserved_keys(key, val);
    gguf_remove_key(ctx, key);
    ctx->kv.emplace_back(key, val);
}

void gguf_set_val_i64(struct gguf_context * ctx, const char * key, int64_t val) {
    gguf_check_reserved_keys(key, val);
    gguf_remove_key(ctx, key);
    ctx->kv.emplace_back(key, val);
}

void gguf_set_val_f64(struct gguf_context * ctx, const char * key, double val) {
    gguf_check_reserved_keys(key, val);
    gguf_remove_key(ctx, key);
    ctx->kv.emplace_back(key, val);
}

void gguf_set_val_bool(struct gguf_context * ctx, const char * key, bool val) {
    gguf_check_reserved_keys(key, val);
    gguf_remove_key(ctx, key);
    ctx->kv.emplace_back(key, val);
}

void gguf_set_val_str(struct gguf_context * ctx, const char * key, const char * val) {
    gguf_check_reserved_keys(key, val);
    gguf_remove_key(ctx, key);
    ctx->kv.emplace_back(key, std::string(val));
}

void gguf_set_arr_data(struct gguf_context * ctx, const char * key, enum gguf_type type, const void * data, size_t n) {
    gguf_check_reserved_keys(key, data);
    gguf_remove_key(ctx, key);

    const size_t nbytes = n*gguf_type_size(type);
    std::vector<int8_t> tmp(nbytes);
    if (!tmp.empty()) {
        memcpy(tmp.data(), data, nbytes);
    }
    ctx->kv.emplace_back(key, tmp);
    ctx->kv.back().cast(type);
}

void gguf_set_arr_str(struct gguf_context * ctx, const char * key, const char ** data, size_t n) {
    gguf_check_reserved_keys(key, data);
    gguf_remove_key(ctx, key);

    std::vector<std::string> tmp(n);
    for (size_t i = 0; i < n; ++i) {
        tmp[i] = data[i];
    }
    ctx->kv.emplace_back(key, tmp);
}

// set or add KV pairs from another context
void gguf_set_kv(struct gguf_context * ctx, const struct gguf_context * src) {
    const int64_t n_kv = gguf_get_n_kv(src);
    for (int64_t i = 0; i < n_kv; ++i) {
        const struct gguf_kv & kv = src->kv[i];

        if (!kv.is_array) {
            switch (kv.get_type()) {
                case GGUF_TYPE_UINT8:   gguf_set_val_u8  (ctx, kv.get_key().c_str(), kv.get_val<uint8_t>());             break;
                case GGUF_TYPE_INT8:    gguf_set_val_i8  (ctx, kv.get_key().c_str(), kv.get_val<int8_t>());              break;
                case GGUF_TYPE_UINT16:  gguf_set_val_u16 (ctx, kv.get_key().c_str(), kv.get_val<uint16_t>());            break;
                case GGUF_TYPE_INT16:   gguf_set_val_i16 (ctx, kv.get_key().c_str(), kv.get_val<int16_t>());             break;
                case GGUF_TYPE_UINT32:  gguf_set_val_u32 (ctx, kv.get_key().c_str(), kv.get_val<uint32_t>());            break;
                case GGUF_TYPE_INT32:   gguf_set_val_i32 (ctx, kv.get_key().c_str(), kv.get_val<int32_t>());             break;
                case GGUF_TYPE_FLOAT32: gguf_set_val_f32 (ctx, kv.get_key().c_str(), kv.get_val<float>());               break;
                case GGUF_TYPE_UINT64:  gguf_set_val_u64 (ctx, kv.get_key().c_str(), kv.get_val<uint64_t>());            break;
                case GGUF_TYPE_INT64:   gguf_set_val_i64 (ctx, kv.get_key().c_str(), kv.get_val<int64_t>());             break;
                case GGUF_TYPE_FLOAT64: gguf_set_val_f64 (ctx, kv.get_key().c_str(), kv.get_val<double>());              break;
                case GGUF_TYPE_BOOL:    gguf_set_val_bool(ctx, kv.get_key().c_str(), kv.get_val<bool>());                break;
                case GGUF_TYPE_STRING:  gguf_set_val_str (ctx, kv.get_key().c_str(), kv.get_val<std::string>().c_str()); break;
                case GGUF_TYPE_ARRAY:
                default: GGML_ABORT("invalid type");
            }
            continue;
        }

        const size_t ne = kv.get_ne();

        switch (kv.get_type()) {
            case GGUF_TYPE_UINT8:
            case GGUF_TYPE_INT8:
            case GGUF_TYPE_UINT16:
            case GGUF_TYPE_INT16:
            case GGUF_TYPE_UINT32:
            case GGUF_TYPE_INT32:
            case GGUF_TYPE_FLOAT32:
            case GGUF_TYPE_UINT64:
            case GGUF_TYPE_INT64:
            case GGUF_TYPE_FLOAT64:
            case GGUF_TYPE_BOOL: {
                gguf_set_arr_data(ctx, kv.get_key().c_str(), kv.get_type(), kv.data.data(), ne);
            } break;
            case GGUF_TYPE_STRING: {
                std::vector<const char *> tmp(ne);
                for (size_t j = 0; j < ne; ++j) {
                    tmp[j] = kv.data_string[j].c_str();
                }
                gguf_set_arr_str(ctx, kv.get_key().c_str(), tmp.data(), ne);
            } break;
            case GGUF_TYPE_ARRAY:
            default: GGML_ABORT("invalid type");
        }
    }
}

void gguf_add_tensor(
             struct gguf_context * ctx,
        const struct ggml_tensor * tensor) {
    GGML_ASSERT(tensor);
    if (gguf_find_tensor(ctx, tensor->name) != -1) {
        GGML_ABORT("duplicate tensor name: %s", tensor->name);
    }

    struct gguf_tensor_info ti;
    ti.t = *tensor;
    ti.offset = ctx->info.empty() ? 0 :
        ctx->info.back().offset + GGML_PAD(ggml_nbytes(&ctx->info.back().t), ctx->alignment);
    ctx->info.push_back(ti);
}

void gguf_set_tensor_type(struct gguf_context * ctx, const char * name, enum ggml_type type) {
    const int64_t tensor_id = gguf_find_tensor(ctx, name);
    if (tensor_id < 0) {
        GGML_ABORT("tensor not found: %s", name);
    }
    struct ggml_tensor * tensor = &ctx->info[tensor_id].t;
    const size_t  type_size = ggml_type_size(type);
    const int64_t blck_size = ggml_blck_size(type);

    tensor->type = type;
    GGML_ASSERT(tensor->ne[0] % blck_size == 0 && "tensor row size not divisible by block size of new type");

    tensor->nb[0] = type_size;
    tensor->nb[1] = tensor->nb[0]*(tensor->ne[0]/blck_size);
    for (int i = 2; i < GGML_MAX_DIMS; i++) {
        tensor->nb[i] = tensor->nb[i - 1]*tensor->ne[i - 1];
    }

    // update offsets
    const int64_t n_tensors = gguf_get_n_tensors(ctx);
    for (int64_t i = tensor_id + 1; i < n_tensors; ++i) {
        ctx->info[i].offset = ctx->info[i - 1].offset + GGML_PAD(ggml_nbytes(&ctx->info[i - 1].t), ctx->alignment);
    }
}

void gguf_set_tensor_data(struct gguf_context * ctx, const char * name, const void * data) {
    const int64_t tensor_id = gguf_find_tensor(ctx, name);
    if (tensor_id < 0) {
        GGML_ABORT("tensor not found: %s", name);
    }

    ctx->info[tensor_id].t.data = (void *)(uintptr_t)data; // double cast suppresses warning about casting away const
}

struct gguf_writer {
    std::vector<int8_t> & buf;

    gguf_writer(std::vector<int8_t> & buf) : buf(buf) {}

    template <typename T>
    void write(const T & val) const {
        for (size_t i = 0; i < sizeof(val); ++i) {
            buf.push_back(reinterpret_cast<const int8_t *>(&val)[i]);
        }
    }

    void write(const std::vector<int8_t> & val) const {
        buf.insert(buf.end(), val.begin(), val.end());
    }

    void write(const bool & val) const {
        const int8_t val8 = val ? 1 : 0;
        write(val8);
    }

    void write(const std::string & val) const {
        {
            const uint64_t n = val.length();
            write(n);
        }
        for (size_t i = 0; i < val.length(); ++i) {
            buf.push_back(reinterpret_cast<const int8_t *>(val.data())[i]);
        }
    }

    void write(const char * val) const {
        write(std::string(val));
    }

    void write(const enum ggml_type & val) const {
        write(int32_t(val));
    }

    void write(const enum gguf_type & val) const {
        write(int32_t(val));
    }

    void write(const struct gguf_kv & kv) const {
        const uint64_t ne = kv.get_ne();

        write(kv.get_key());

        if (kv.is_array) {
            write(GGUF_TYPE_ARRAY);
            write(kv.get_type());
            write(ne);
        } else {
            write(kv.get_type());
        }

        switch (kv.get_type()) {
            case GGUF_TYPE_UINT8:
            case GGUF_TYPE_INT8:
            case GGUF_TYPE_UINT16:
            case GGUF_TYPE_INT16:
            case GGUF_TYPE_UINT32:
            case GGUF_TYPE_INT32:
            case GGUF_TYPE_FLOAT32:
            case GGUF_TYPE_UINT64:
            case GGUF_TYPE_INT64:
            case GGUF_TYPE_FLOAT64: {
                write(kv.data);
            } break;
            case GGUF_TYPE_BOOL: {
                for (size_t i = 0; i < ne; ++i) {
                    write(kv.get_val<bool>(i));
                }
            } break;
            case GGUF_TYPE_STRING: {
                for (size_t i = 0; i < ne; ++i) {
                    write(kv.get_val<std::string>(i));
                }
            } break;
            case GGUF_TYPE_ARRAY:
            default: GGML_ABORT("invalid type");
        }
    }

    void write_tensor_meta(const struct gguf_tensor_info & info) const {
        write(info.t.name);

        const uint32_t n_dims = ggml_n_dims(&info.t);
        write(n_dims);

        for (uint32_t j = 0; j < n_dims; ++j) {
            write(info.t.ne[j]);
        }
        write(info.t.type);
        write(info.offset);
    }

    void pad(const size_t alignment) const {
        while (buf.size() % alignment != 0) {
            const int8_t zero = 0;
            write(zero);
        }
    }

    void write_tensor_data(const struct gguf_tensor_info & info, const size_t offset_data, const size_t alignment) const {
        GGML_ASSERT(buf.size() - offset_data == info.offset);

        GGML_ASSERT(ggml_is_contiguous(&info.t));
        const size_t offset = buf.size();
        const size_t nbytes = ggml_nbytes(&info.t);

        buf.resize(offset + nbytes);
        if (info.t.buffer) {
            ggml_backend_tensor_get(&info.t, buf.data() + offset, 0, nbytes);
        } else {
            GGML_ASSERT(info.t.data);
            memcpy(buf.data() + offset, info.t.data, nbytes);
        }

        pad(alignment);
    }
};

void gguf_write_to_buf(const struct gguf_context * ctx, std::vector<int8_t> & buf, bool only_meta) {
    const struct gguf_writer gw(buf);

    const int64_t n_kv      = gguf_get_n_kv(ctx);
    const int64_t n_tensors = gguf_get_n_tensors(ctx);

    // write header
    gw.write(GGUF_MAGIC[0]);
    gw.write(GGUF_MAGIC[1]);
    gw.write(GGUF_MAGIC[2]);
    gw.write(GGUF_MAGIC[3]);
    gw.write(ctx->version);
    gw.write(n_tensors);
    gw.write(n_kv);

    // write key-value pairs
    for (int64_t i = 0; i < n_kv; ++i) {
        gw.write(ctx->kv[i]);
    }

    // write tensor info
    for (int64_t i = 0; i < n_tensors; ++i) {
        gw.write_tensor_meta(ctx->info[i]);
    }

    // we require the data section to be aligned
    gw.pad(ctx->alignment);

    if (only_meta) {
        return;
    }

    const size_t offset_data = gw.buf.size();

    // write tensor data
    for (int64_t i = 0; i < n_tensors; ++i) {
        gw.write_tensor_data(ctx->info[i], offset_data, ctx->alignment);
    }
}

bool gguf_write_to_file(const struct gguf_context * ctx, const char * fname, bool only_meta) {
    FILE * file = ggml_fopen(fname, "wb");

    if (!file) {
        fprintf(stderr, "%s: failed to open file '%s' for writing GGUF data\n", __func__, fname);
        return false;
    }

    std::vector<int8_t> buf;
    gguf_write_to_buf(ctx, buf, only_meta);
    const bool ok = fwrite(buf.data(), 1, buf.size(), file) == buf.size();
    fclose(file);
    return ok;
}

size_t gguf_get_meta_size(const struct gguf_context * ctx) {
    // only return size
    std::vector<int8_t> buf;
    gguf_write_to_buf(ctx, buf, /*only_meta =*/ true);
    return buf.size();
}

void gguf_get_meta_data(const struct gguf_context * ctx, void * data) {
    std::vector<int8_t> buf;
    gguf_write_to_buf(ctx, buf, /*only_meta =*/ true);
    memcpy(data, buf.data(), buf.size());
}
