# LLGuidance Support in llama.cpp
```c
/*
Notes:杨小兵-2025-04-14

1、该文档内容将会解释llama.cpp中的LLGuidance支持。
2、问题
    2.1 LLGuidance是什么？
    2.2 LLGuidance解决了问题？
*/
```

[LLGuidance](https://github.com/guidance-ai/llguidance) is a library for constrained decoding (also called constrained sampling or structured outputs) for Large Language Models (LLMs). Initially developed as the backend for the [Guidance](https://github.com/guidance-ai/guidance) library, it can also be used independently.

LLGuidance supports JSON Schemas and arbitrary context-free grammars (CFGs) written in a [variant](https://github.com/guidance-ai/llguidance/blob/main/docs/syntax.md) of Lark syntax. It is [very fast](https://github.com/guidance-ai/jsonschemabench/tree/main/maskbench) and has [excellent](https://github.com/guidance-ai/llguidance/blob/main/docs/json_schema.md) JSON Schema coverage but requires the Rust compiler, which complicates the llama.cpp build process.
```c
/*
Notes:杨小兵-2025-04-14

1、LLGudiance
    1.1 LLGudiance是一个软件库
    1.2 LLGudiance是用来帮助LLMs生成结构化数据的一个软件库（这里的重点是：结构化数据）
    1.3 支持
        1.3.1 JSON 模式
        1.3.2 上下文无关文法（CFG）（这部分内容不理解）
    1.4 LLGudiance软件库是使用Rust编程语言进行编写的。
    1.5 LLGudiance可以限制LLMs来输出特定格式的文本，这一点是通过定义规则来实现的，这里的定义规则指的就是使用上述提到的 JSON 模式和 CFG 方式实现的。
2、大型语言模型如 GPT-3 或 BERT，可以理解和生成类人文本，但其输出可能不总是符合预期格式。例如，如果需要 JSON 数据，模型可能生成无效的 JSON，这会增加后续解析的难度。LLGuidance 通过定义约束（如 JSON 模式或 CFG）来解决这个问题。
3、sampling(采样)
    3.1 想象你在用 C 语言写一个随机数生成器，比如 rand()。在语言模型（LLM）里，sampling 就像是模型每次要“猜”下一个词时，从一堆可能的词里随机挑一个。不过，这个“随机”不是完全瞎猜，而是根据每个词的可能性（概率）来选。比如，模型可能算出“the”有 30% 的机会，“a”有 20% 的机会，sampling 就是从这些概率里挑一个。
4、constrained decoding(约束解码)
    4.1 现在，假设你不想要完全随机的结果，而是希望控制这个随机过程。比如你在 C 里用 if 或 switch 语句限制随机数只能是某些特定值（比如只能是 1、3、5）。在 LLM 里，constrained decoding 就是在 sampling 的基础上加规则，比如“输出的必须是一个名字”或者“必须符合某种格式”，限制模型只能挑符合条件的词。
    4.2 从这个角度来说，constrained decoding就是限制sampling的过程，原来还可以从这个角度出发来进行思考，这里还需要对LLMs推理过程、具体的代码实现有一个详细具体的了解，如果想要深入了解则必须动手制作一个或者进行优化。
5、Structured Outputs（结构化输出）
    5.1 这就像 C 里的结构体（struct）。你定义了一个格式，比如一个包含名字和年龄的结构体 struct Person { char name[20]; int age; };，然后要求程序输出必须符合这个结构。在 LLM 里，structured outputs 是最终目标，比如让模型输出一个 JSON 格式的数据（{"name": "John", "age": 30}），而不是随便一堆词。
6、sampling/constrained decoding/structured outputs三者之间的关系
    6.1 Sampling 是最基础的“挑词”过程，就像 C 里的 rand()。
    6.2 Constrained decoding 是在 sampling 上加约束，就像用条件语句控制随机选择。
    6.3 Structured outputs 是最终结果，就像要求程序输出符合某个 struct 的数据。
*/
```

## Building

To enable LLGuidance support, build llama.cpp with the `LLAMA_LLGUIDANCE` option:

```sh
cmake -B build -DLLAMA_LLGUIDANCE=ON
make -C build -j
```

For Windows use `cmake --build build --config Release` instead of `make`.

This requires the Rust compiler and the `cargo` tool to be [installed](https://www.rust-lang.org/tools/install).
```c
/*
Notes:杨小兵-2025-04-14

1、windows平台上述使用cmake作为构建工具。
2、非windows平台上可以通过使用cmake的宏定义来启动LLGuidance支持。
3、如果想要成功构建有LLGuidance支持的llama.cpp，则需要Rust compiler和cargo工具。
    3.1 Rust compiler:后续进行了解
    3.2 cargo：后续进行了解
*/
```

## Interface

There are no new command-line arguments or modifications to `common_params`. When enabled, grammars starting with `%llguidance` are passed to LLGuidance instead of the [current](../grammars/README.md) llama.cpp grammars. Additionally, JSON Schema requests (e.g., using the `-j` argument in `llama-cli`) are also passed to LLGuidance.

For your existing GBNF grammars, you can use [gbnf_to_lark.py script](https://github.com/guidance-ai/llguidance/blob/main/python/llguidance/gbnf_to_lark.py) to convert them to LLGuidance Lark-like format.
```c
/*
Notes:杨小兵-2025-04-14

1、无新命令行参数或修改：现有的命令行参数和 common_params 保持不变，系统兼容性得以保留。
2、LLGuidance 的使用方式
    2.1 当启用 LLGuidance 支持后，以 %llguidance 开头的语法会被传递给 LLGuidance 处理，而不是传统的 llama.cpp 语法。
    2.2 JSON Schema 请求（例如在 llama-cli 中使用 -j 参数）也会直接交给 LLGuidance。
3、语法转换工具：对于已有的 GBNF 语法，用户可以使用 gbnf_to_lark.py 脚本将其转换为 LLGuidance 的 Lark-like 格式，方便迁移和使用。
*/
```

## Performance

Computing a "token mask" (i.e., the set of allowed tokens) for a llama3 tokenizer with 128k tokens takes, on average, 50μs of single-core CPU time for the [JSON Schema Bench](https://github.com/guidance-ai/jsonschemabench). The p99 time is 0.5ms, and the p100 time is 20ms. These results are due to the lexer/parser split and several [optimizations](https://github.com/guidance-ai/llguidance/blob/main/docs/optimizations.md).
```c
/*
Notes:杨小兵-2025-04-14

1、从上述的描述中可以知道LLGudian库的处理性能是非常高的。
*/
```

## JSON Schema

LLGuidance adheres closely to the JSON Schema specification. For example:

- `additionalProperties` defaults to `true`, unlike current grammars, though you can set `"additionalProperties": false` if needed.
- any whitespace is allowed.
- The definition order in the `"properties": {}` object is maintained, regardless of whether properties are required (current grammars always puts required properties first).

Unsupported schemas result in an error message—no keywords are silently ignored.
```c
/*
Notes:杨小兵-2025-04-14

1、LLGuidance 严格遵循 JSON Schema 规范，与现有语法相比，它提供了更高的灵活性和标准化：允许 additionalProperties 默认为 true（并提供将其设置为 false 的选项）、允许任何空格，并且无论属性是否必需，都保留 "properties": {} 对象中的定义顺序。此外，它还会针对不支持的 Schema 明确生成错误消息，而不是默默忽略它们，从而增强用户的感知能力。
*/
```

## Why Not Reuse GBNF Format?

GBNF lacks the concept of a lexer.

Most programming languages, including JSON, use a two-step process: a lexer (built with regular expressions) converts a byte stream into lexemes, which are then processed by a CFG parser. This approach is faster because lexers are cheaper to evaluate, and there is ~10x fewer lexemes than bytes.
LLM tokens often align with lexemes, so the parser is engaged in under 0.5% of tokens, with the lexer handling the rest.

However, the user has to provide the distinction between lexemes and CFG symbols. In [Lark](https://github.com/lark-parser/lark), lexeme names are uppercase, while CFG symbols are lowercase.
The [gbnf_to_lark.py script](https://github.com/guidance-ai/llguidance/blob/main/scripts/gbnf_to_lark.py) can often take care of this automatically.
See [LLGuidance syntax docs](https://github.com/guidance-ai/llguidance/blob/main/docs/syntax.md#terminals-vs-rules) for more details.
```c
/*
Notes:杨小兵-2025-04-14

1、上述观点的核心在于，GBNF 格式没有词法分析器这一概念，而大多数编程语言通常采用先通过正则表达式构建的词法分析器将字节流转换为词素，再由上下文无关语法 (CFG) 解析器处理词素的两步法。这种方法效率更高，因为词素数量远少于字节数，而且对于LLM来说，token通常就对应词素，只需极少部分token交由CFG解析器处理。不过，这也要求用户明确区分词法符号和CFG符号（如在Lark中分别用大写和小写表示），而相关脚本如gbnf_to_lark.py可以自动完成这种转换。
*/
```

## Error Handling

Errors are currently printed to `stderr`, and generation continues. Improved error handling may be added in the future.
