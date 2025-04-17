# Pull requests (for contributors)

- Test your changes:
    - Execute [the full CI locally on your machine](ci/README.md) before publishing
    - Verify that the perplexity and the performance are not affected negatively by your changes (use `llama-perplexity` and `llama-bench`)
    - If you modified the `ggml` source, run the `test-backend-ops` tool to check whether different backend implementations of the `ggml` operators produce consistent results (this requires access to at least two different `ggml` backends)
    - If you modified a `ggml` operator or added a new one, add the corresponding test cases to `test-backend-ops`
- Consider allowing write access to your branch for faster reviews, as reviewers can push commits directly
- If your PR becomes stale, don't hesitate to ping the maintainers in the comments
```c
/*
Notes:杨小兵-2025-04-16

1、当贡献者想要提交PRs的时候需要注意的事情。
2、对自己的更改需要进行测试
    2.1 在提交PRs之前应该在本地执行全量的CI。（这里提到的CI locally是比较有意思的，目前我只能在Github Actions中执行全量的CI，不知道如何在本地进行CI）
    2.2 验证做出更改不会对困惑度即perplexity和性能即performance产生负面影响（使用“llama-perplexity”和“llama-bench”测试分析）
    2.3 如果PRs中修改了 `ggml` 源，请运行 `test-backend-ops` 工具来检查 `ggml` 运算符的不同后端实现是否产生一致的结果（这需要访问至少两个不同的 `ggml` 后端）。
    2.4 如果PRs中修改了 `ggml` 运算符或添加了新的运算符，请将相应的测试用例添加到 `test-backend-ops`
3、考虑允许对你的分支进行写访问，以便更快地进行审阅，因为审阅者可以直接推送提交。
4、如果你的 PR 变得过时（这里指的应该是等待时间过久），请随时在评论中联系维护者。
*/
```

# Pull requests (for collaborators)

- Squash-merge PRs
- Use the following format for the squashed commit title: `<module> : <commit title> (#<issue_number>)`. For example: `utils : fix typo in utils.py (#1234)`
- Optionally pick a `<module>` from here: https://github.com/ggml-org/llama.cpp/wiki/Modules
- Consider adding yourself to [CODEOWNERS](CODEOWNERS)
```c
/*
Notes:杨小兵-2025-04-17

1、当协作者想要提交PRs的时候需要注意的事情。
2、“Squash merge” 是一种合并策略：它会把你在这个 PR 里所有的提交（可能有好几个“小步改动”）“压扁”成一个提交，然后再合并到主分支。这样做的好处是：
    2.1 主分支的历史更加的干净，只会看到一个大的功能或者修复提交。
    2.2 不会把每次小改动都留在主分支上，减少噪音.
*/
```

# Coding guidelines

- Avoid adding third-party dependencies, extra files, extra headers, etc.
- Always consider cross-compatibility with other operating systems and architectures
- Avoid fancy-looking modern STL constructs, use basic `for` loops, avoid templates, keep it simple
- Vertical alignment makes things more readable and easier to batch edit
- Clean-up any trailing whitespaces, use 4 spaces for indentation, brackets on the same line, `void * ptr`, `int & a`
- Use sized integer types such as `int32_t` in the public API, e.g. `size_t` may also be appropriate for allocation sizes or byte offsets
- Declare structs with `struct foo {}` instead of `typedef struct foo {} foo`
    - In C++ code omit optional `struct` and `enum` keyword whenever they are not necessary
    ```cpp
    // OK
    llama_context * ctx;
    const llama_rope_type rope_type;

    // not OK
    struct llama_context * ctx;
    const enum llama_rope_type rope_type;
    ```

    _(NOTE: this guideline is yet to be applied to the `llama.cpp` codebase. New code should follow this guideline.)_

- Try to follow the existing patterns in the code (indentation, spaces, etc.). In case of doubt use `clang-format` to format the added code
- For anything not covered in the current guidelines, refer to the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- Tensors store data in row-major order. We refer to dimension 0 as columns, 1 as rows, 2 as matrices
- Matrix multiplication is unconventional: [`C = ggml_mul_mat(ctx, A, B)`](https://github.com/ggml-org/llama.cpp/blob/880e352277fc017df4d5794f0c21c44e1eae2b84/ggml.h#L1058-L1064) means $C^T = A B^T \Leftrightarrow C = B A^T.$

![matmul](media/matmul.png)
```c
/*
Notes:杨小兵-2025-04-17

1、避免添加第三方库依赖、额外的文件、额外的头文件等等。
2、总是要考虑同其他操作系统和硬件架构的跨平台兼容性。
3、避免看起来很时髦的现代STL构造器，使用基本的for循环，避免使用templates从而保持代码的简单。
4、垂直方向对齐从而使得批量编辑变得更加的容易以及保持代码的可读性。
5、清除任何尾随的空格，使用4个空格进行缩进，括号在同一行（之前一直不是非常习惯），void * ptr, int & a这种编写代码风格。
6、在公共 API 中使用大小整数类型（例如 int32_t），例如 size_t 也可能适用于分配大小或字节偏移量。
7、声明结构体的时候使用struct foo{}而不是使用typedef struct foo{} foo的方式（个人风格倒是喜欢使用后者）。
    7.1 在 C++ 代码里，声明和使用 struct/enum 类型时不要写多余的关键字，并且 不要再用 C 里那种 typedef struct … 的写法。（对于C语言而言，定义一个结构体之后在后续用到结构体的时候需要添加struct关键词或者需要通过typedef再次定义标签，经过了两个步骤，但是对于C++而言，在设计C++的时候就已经将这两个步骤合成一个步骤了，因此在使用C++定义结构体的时候不需要使用typedef，这样将会多余。）
8、尽量遵循代码中现有的模式（缩进、空格等）。如有疑问，请使用 clang-format（来自 clang-tools v15+）来格式化添加的代码。
9、对于没有在当前guideline中包含的其他任何东西，请参考C++ Core Guidelines文件内容。
10、tensors以row-major order的方式存储数据，我们将维度 0 称为列，维度 1 称为行，维度 2 称为矩阵。
11、矩阵乘法不是传统的矩阵乘法（说明这里存在一些不同）：其中C = ggml_mul_mat(ctx, A, B)表示的是$C^T = A B^T \Leftrightarrow C = B A^T.$（这里的ggml_mul_mat是GGML中的接口）。
    11.1 这部分内容后续进行深入理解。
*/
```

# Naming guidelines

- Use `snake_case` for function, variable and type names
- Naming usually optimizes for longest common prefix (see https://github.com/ggml-org/ggml/pull/302#discussion_r1243240963)

    ```cpp
    // not OK
    int small_number;
    int big_number;

    // OK
    int number_small;
    int number_big;
    ```

- Enum values are always in upper case and prefixed with the enum name

    ```cpp
    enum llama_vocab_type {
        LLAMA_VOCAB_TYPE_NONE = 0,
        LLAMA_VOCAB_TYPE_SPM  = 1,
        LLAMA_VOCAB_TYPE_BPE  = 2,
        LLAMA_VOCAB_TYPE_WPM  = 3,
        LLAMA_VOCAB_TYPE_UGM  = 4,
        LLAMA_VOCAB_TYPE_RWKV = 5,
    };
    ```

- The general naming pattern is `<class>_<method>`, with `<method>` being `<action>_<noun>`

    ```cpp
    llama_model_init();           // class: "llama_model",         method: "init"
    llama_sampler_chain_remove(); // class: "llama_sampler_chain", method: "remove"
    llama_sampler_get_seed();     // class: "llama_sampler",       method: "get_seed"
    llama_set_embeddings();       // class: "llama_context",       method: "set_embeddings"
    llama_n_threads();            // class: "llama_context",       method: "n_threads"
    llama_adapter_lora_free();    // class: "llama_adapter_lora",  method: "free"
    ```

    - The `get` `<action>` can be omitted
    - The `<noun>` can be omitted if not necessary
    - The `_context` suffix of the `<class>` is optional. Use it to disambiguate symbols when needed
    - Use `init`/`free` for constructor/destructor `<action>`

- Use the `_t` suffix when a type is supposed to be opaque to the user - it's not relevant to them if it is a struct or anything else

    ```cpp
    typedef struct llama_context * llama_context_t;

    enum llama_pooling_type llama_pooling_type(const llama_context_t ctx);
    ```

    _(NOTE: this guideline is yet to be applied to the `llama.cpp` codebase. New code should follow this guideline)_

- C/C++ filenames are all lowercase with dashes. Headers use the `.h` extension. Source files use the `.c` or `.cpp` extension
- Python filenames are all lowercase with underscores

- _(TODO: abbreviations usage)_
```c
/*
Notes:杨小兵-2025-04-17

1、这部分内容目前暂时用不到，后续在需要的时候再仔细理解这部分内容。
*/
```

# Preprocessor directives

- _(TODO: add guidelines with examples and apply them to the codebase)_

    ```cpp
    #ifdef FOO
    #endif // FOO
    ```

# Documentation

- Documentation is a community effort
- When you need to look into the source code to figure out how to use an API consider adding a short summary to the header file for future reference
- When you notice incorrect or outdated documentation, please update it

# Resources

The Github issues, PRs and discussions contain a lot of information that can be useful to get familiar with the codebase. For convenience, some of the more important information is referenced from Github projects:

https://github.com/ggml-org/llama.cpp/projects
