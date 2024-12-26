# GBNF Guide
```c
/*
Note:杨小兵-2024-12-24

1、GBNF：从该文档中后面有说明（GBNF：GGML BNF，需要注意的是GBNF是BNF的扩展）
2、这个文件讲述的是关于GBNF相关内容（提供的能力、编写的规则等等）
3、根据之前的经验知道GBNF的作用就是为了严格限制LLMs的输出内容，但是具体的效果还不知道是什么样子？自己提供的*.gbnf文件目前不起作用
4、针对GBNF的作用后续还需要深入理解
*/
```
GBNF (GGML BNF) is a format for defining [formal grammars](https://en.wikipedia.org/wiki/Formal_grammar) to constrain model outputs in `llama.cpp`. For example, you can use it to force the model to generate valid JSON, or speak only in emojis. GBNF grammars are supported in various ways in `examples/main` and `examples/server`.
```c
/*
Note:杨小兵-2024-12-24

1、GBNF：GGML BNF（其中GGML我知道是llama.cpp项目所依赖的ggml库的名称，其中BNF不理解）
2、BNF：在编译器、自然语言处理、编程语言中是一个重要的概念，自己没有深入接触compiler相关内容因此接触不到
3、GBNF是一种定义 [formal_grammar]以约束 llama.cpp 中的模型输出的格式（llama.cpp项目中支持的LLMs）。例如，您可以使用它来强制模型生成有效的 JSON，或者只使用表情符号说话。examples/main 和 examples/server 以各种方式支持 GBNF 语法。
  3.1 强制模型生成有效的JSON
  3.2 强制模型生成emojis
4、注意：这里的GBNF是为了llama.cpp项目服务的，可能对符合BNF格式的文件llama.cpp支持不好
*/
```

## 1 Background

[Backus-Naur Form (BNF)](https://en.wikipedia.org/wiki/Backus%E2%80%93Naur_form) is a notation for describing the syntax of formal languages like programming languages, file formats, and protocols. GBNF is an extension of BNF that primarily adds a few modern regex-like features.
```c
/*
Note:杨小兵-2024-12-24

1、BNF(Backus-Naur Form)是一种用于描述形式语言（如编程语言、文件格式和协议）语法的符号。GBNF 是 BNF 的扩展，主要添加了一些现代正则表达式类功能。
  1.1 formal languages的例子
    file formats
    programming languages
    protocols
2、BNF 是由 John Backus 和 Peter Naur 提出的形式化表示法，用于描述上下文无关文法（Context-Free Grammar, CFG）。它的语法结构非常简洁，主要由以下几个部分组成：
  2.1 非终结符（Nonterminal symbols）：通常是用尖括号 < > 包裹的符号，表示语法规则的左侧变量。例如，<expression> 或 <number>。
  2.2 终结符（Terminal symbols）：语法规则中的实际字符或符号，不能进一步分解。例如，数字 0-9 或字符 +、-。
  2.3 产生式规则（Production rules）：定义非终结符如何用其他符号进行替换。例如，<expression> ::= <number> "+" <number>。
3、GBNF 对 BNF 做了扩展，引入了更加灵活的正则表达式（regex）特性，能够更精确地描述一些复杂的语言模式。下面是一些 BNF 的一些特性：
  3.1 重复（Repetition）：使用 * 或 + 来表示重复的模式，类似于正则表达式中的量词。（* 表示零次或多次；+ 表示一次或多次）
  3.2 选择（Alternation）：使用 | 来表示多个选项，类似于正则表达式中的 |。
  3.3 分组（Grouping）：使用圆括号 () 来分组模式，允许对多个选项进行优先级控制。
  3.4 可选项（Optional）：使用 ? 来表示可选项，即某部分可以出现一次或者不出现。
4、GBNF 是 BNF 的一个扩展，加入了更多现代化的特性，特别是正则表达式的功能，使得它在定义正式语言的语法时更加灵活和强大。GBNF 适用于需要精准控制和描述输出格式的场景，如约束语言模型的生成、验证输入数据格式等。
*/
```

## 2 Basics

In GBNF, we define *production rules* that specify how a *non-terminal* (rule name) can be replaced with sequences of *terminals* (characters, specifically Unicode [code points](https://en.wikipedia.org/wiki/Code_point)) and other non-terminals. The basic format of a production rule is `nonterminal ::= sequence...`.
```c
/*
Note:杨小兵-2024-12-24

1、在 GBNF 中，我们定义production rules，它指定了如何将non-terminal（规则名称）替换为terminals（字符，特别是 Unicode 码点）和其他非终结符的序列。产生式规则的基本格式是nonterminal ::= sequence...
2、GBNF 的产生式规则是定义正式语言语法的核心，它通过将非终结符替换为终结符和其他非终结符的序列，建立了语言的结构和约束。
*/
```

## 3 Example

Before going deeper, let's look at some of the features demonstrated in `grammars/chess.gbnf`, a small chess notation grammar:
```
# `root` specifies the pattern for the overall output
root ::= (
    # it must start with the characters "1. " followed by a sequence
    # of characters that match the `move` rule, followed by a space, followed
    # by another move, and then a newline
    "1. " move " " move "\n"

    # it's followed by one or more subsequent moves, numbered with one or two digits
    ([1-9] [0-9]? ". " move " " move "\n")+
)

# `move` is an abstract representation, which can be a pawn, nonpawn, or castle.
# The `[+#]?` denotes the possibility of checking or mate signs after moves
move ::= (pawn | nonpawn | castle) [+#]?

pawn ::= ...
nonpawn ::= ...
castle ::= ...
```
```c
/*
Note:杨小兵-2024-12-24

1、在深入学习GBNF之前，让我们首先查看一下在grammars/chess.gbnf文件中演示的一些features，一个小的chess notation语法
2、解释
  在 `grammars/chess.gbnf` 文件中，定义了一个小型国际象棋记谱法的文法规则。这个文法规则的目标是描述一个棋局的表示方式，包括棋步的顺序和一些特殊符号（如将军、将死）。以下是文法的关键点：

1. **`root`**: 这是文法的根规则，定义了整体输出的模式。根规则的内容要求：
   - 输出必须从字符 `"1. "` 开始，表示第一步棋。
   - 紧接着是一个棋步，符合 `move` 规则。
   - 接着是一个空格和另一个棋步，再接着是一个换行符 `\n`，这表示第一步棋的描述结束。
   - 然后是一个或多个后续棋步，每个棋步由一到两位数字编号，后面是棋步内容，并以换行符结束。

2. **`move`**: 定义了棋步的类型，棋步可以是：
   - **`pawn`**: 代表兵的移动。
   - **`nonpawn`**: 代表非兵类的棋子的移动（如马、车、象等）。
   - **`castle`**: 代表王车易位。
   - `move` 规则后面可以加上 `[#|+]`，表示可能存在“将军（#）”或“将死（+）”的标记。

3. **子规则**： 
   - **`pawn`**、**`nonpawn`**、**`castle`**：这些规则定义了具体棋步的语法，尽管在代码中它们是简化的（用 `...` 表示具体内容）。这些子规则详细描述了兵、非兵和王车易位的合法格式。

结构解析：
- **`root` 规则**：整个棋局的格式是以 `1. ` 开头，后面跟着两步棋（符合 `move` 规则），然后是后续多步棋，使用 `[1-9] [0-9]? ". "` 来表示每步棋的编号。
- **`move` 规则**：每步棋的具体形式可以是兵的移动、非兵类棋子的移动或王车易位。
- **棋步后缀**：每个棋步可能会有 `+` 或 `#` 后缀，表示是否存在“将军”或“将死”的标志。

  这个文法规则示例用于描述棋局的记谱法，并通过定义特定的文法规则来规范棋步的表示方式。
*/
```

## 4 Non-Terminals and Terminals

Non-terminal symbols (rule names) stand for a pattern of terminals and other non-terminals. They are required to be a dashed lowercase word, like `move`, `castle`, or `check-mate`.

Terminals are actual characters ([code points](https://en.wikipedia.org/wiki/Code_point)). They can be specified as a sequence like `"1"` or `"O-O"` or as ranges like `[1-9]` or `[NBKQR]`.
```c
/*
Note:杨小兵-2024-12-25

1、Non-terminal symbols (rule names)表示terminals和其他non-terminals的一种模式，它们必须是带破折号的小写单词，例如“move”、“castle”或“check-mate”。
2、终结符是实际的字符（[代码点](https://en.wikipedia.org/wiki/Code_point)）。它们可以指定为序列，如“1”或“O-O”，也可以指定为范围，如“[1-9]”或“[NBKQR]”。
*/
```

## 5 Characters and character ranges

Terminals support the full range of Unicode. Unicode characters can be specified directly in the grammar, for example `hiragana ::= [ぁ-ゟ]`, or with escapes: 8-bit (`\xXX`), 16-bit (`\uXXXX`) or 32-bit (`\UXXXXXXXX`).

Character ranges can be negated with `^`:
```
single-line ::= [^\n]+ "\n"
```
```c
/*
Note:杨小兵-2024-12-25

1、terminals支持unicode全部范围。Unicode 字符可以直接在语法中指定，例如 `hiragana ::= [ぁ-ゟ]`，或使用转义符：8 位 (`\xXX`)、16 位 (`\uXXXX`) 或 32 位 (`\UXXXXXXXX`)。
2、通过^符号可以将指定字符剔除掉
3、single-line ::= [^\n]+ "\n"
  3.1 single-line可以由[^\n]+(不包含\n，可以重复0次或者多次)、"\n"构成
*/
```

## 6 Sequences and Alternatives

The order of symbols in a sequence matters. For example, in `"1. " move " " move "\n"`, the `"1. "` must come before the first `move`, etc.

Alternatives, denoted by `|`, give different sequences that are acceptable. For example, in `move ::= pawn | nonpawn | castle`, `move` can be a `pawn` move, a `nonpawn` move, or a `castle`.

Parentheses `()` can be used to group sequences, which allows for embedding alternatives in a larger rule or applying repetition and optional symbols (below) to a sequence.
```c
/*
Note:杨小兵-2024-12-25

1、序列中符号的顺序很重要。例如，在 `"1. " move " " move "\n"` 中，`"1. "` 必须位于第一个 `move` 之前，等等。
2、备选方案（用 `|` 表示）给出可接受的不同序列。例如，在 `move ::= pawn | nonpawn | cas​​tle` 中，`move` 可以是 `pawn` 移动、`nonpawn` 移动或 `castle`。
3、括号 `()` 可用于对序列进行分组，从而允许在更大的规则中嵌入备选方案或将重复和可选符号（如下）应用于序列。
*/
```

## 7 Repetition and Optional Symbols

- `*` after a symbol or sequence means that it can be repeated zero or more times (equivalent to `{0,}`).
- `+` denotes that the symbol or sequence should appear one or more times (equivalent to `{1,}`).
- `?` makes the preceding symbol or sequence optional (equivalent to `{0,1}`).
- `{m}` repeats the precedent symbol or sequence exactly `m` times
- `{m,}` repeats the precedent symbol or sequence at least `m` times
- `{m,n}` repeats the precedent symbol or sequence at between `m` and `n` times (included)
- `{0,n}` repeats the precedent symbol or sequence at most `n` times (included)
```c
/*
Note:杨小兵-2024-12-25

1、在symbol或者sequence后面存在*符号意味着symbol/sequence可以重复0次或者多次（等价于{0,}）
2、在symbol或者sequence后面存在+符号意味着symbol/sequence可以重复1次或者多次（等价于{1,}）
3、在symbol或者sequence后面存在+符号意味着symbol/sequence是可选的，有或者没有都是可以的（等价于{1,}）
4、在symbol或者sequence后面存在{m}符号意味着symbol/sequence有且只能重复m次
5、在symbol或者sequence后面存在{m,}符号意味着symbol/sequence最少重复m次
6、在symbol或者sequence后面存在{m,n}符号意味着symbol/sequence重复m-n次之间
7、在symbol或者sequence后面存在{0,n}符号意味着symbol/sequence最多重复n次之间
*/
```

## 8 Comments and newlines

Comments can be specified with `#`:
```
# defines optional whitespace
ws ::= [ \t\n]+
```

Newlines are allowed between rules and between symbols or sequences nested inside parentheses. Additionally, a newline after an alternate marker `|` will continue the current rule, even outside of parentheses.
```c
/*
Note:杨小兵-2024-12-25

1、在GBNF中可以通过#来进行添加注释从而对*.gbnf中的内容进行说明解释
2、规则之间以及括号内嵌套的符号或序列之间允许使用换行符。此外，替代标记“|”后的换行符将继续当前规则，即使在括号外也是如此。
*/
```

## 9 The root rule

In a full grammar, the `root` rule always defines the starting point of the grammar. In other words, it specifies what the entire output must match.

```
# a grammar for lists
root ::= ("- " item)+
item ::= [^\n]+ "\n"
```
```c
/*
Note:杨小兵-2024-12-25

1、在完整的语法中，“root”规则始终定义语法的起点。换句话说，它指定整个输出必须匹配的内容。
*/
```

## 10 Next steps

This guide provides a brief overview. Check out the GBNF files in this directory (`grammars/`) for examples of full grammars. You can try them out with:
```
./llama-cli -m <model> --grammar-file grammars/some-grammar.gbnf -p 'Some prompt'
```

`llama.cpp` can also convert JSON schemas to grammars either ahead of time or at each request, see below.
```c
/*
Note:杨小兵-2024-12-25

1、这个guide提供了一个简要的概述。查看目录grammars中的GBNF文件从而获取更加完整的grammars例子。
2、可以通过--grammar-file来指定加载使用的GBNF文件
3、`llama.cpp` 还可以提前或在每次请求时将 JSON 模式转换为语法，见13 JSON Schemas -> GBNF。
*/
```

## 11 Troubleshooting

Grammars currently have performance gotchas (see https://github.com/ggerganov/llama.cpp/issues/4218).
```c
/*
Note:杨小兵-2024-12-25

1、语法目前存在性能缺陷（参见https://github.com/ggerganov/llama.cpp/issues/4218）
2、不知道现在是否解决（目前对项目不熟悉不知道如何跟进）
*/
```

### Efficient optional repetitions

A common pattern is to allow repetitions of a pattern `x` up to N times.

While semantically correct, the syntax `x? x? x?.... x?` (with N repetitions) may result in extremely slow sampling. Instead, you can write `x{0,N}` (or `(x (x (x ... (x)?...)?)?)?` w/ N-deep nesting in earlier llama.cpp versions).
```c
/*
Note:杨小兵-2024-12-25

1、一种常见的模式是允许模式“x”重复 N 次。虽然从语义上讲是正确的，但语法“x? x? x?.... x?”（重复 N 次）可能会导致采样速度极慢。相反，您可以编写“x{0,N}”（或“(x (x (x ... (x)?...)?)?)?”，在早期的 llama.cpp 版本中使用 N 层深嵌套）。
2、在编写GBNF文件的时候可以采用这里提到的建议
*/
```

## 12 Using GBNF grammars

You can use GBNF grammars:

- In [llama-server](../examples/server)'s completion endpoints, passed as the `grammar` body field
- In [llama-cli](../examples/main), passed as the `--grammar` & `--grammar-file` flags
- With [llama-gbnf-validator](../examples/gbnf-validator) tool, to test them against strings.
```c
/*
Note:杨小兵-2024-12-25

1、在[llama-server]的completion endpoints中通过传递grammar body字段从而使用grammar
2、在[llama-cli]中通过传递`--grammar` & `--grammar-file`参数从而使用grammar
3、使用 [llama-gbnf-validator]工具，根据字符串对它们进行测试。（在windows中该工具被disable，理由：disabled on Windows because it uses internal functions not exported with LLAMA_API）
*/
```

## 13 JSON Schemas → GBNF

`llama.cpp` supports converting a subset of https://json-schema.org/ to GBNF grammars:
```c
/*
Note:杨小兵-2024-12-25

1、llama.cpp项目支持将https://json-schema.org子集转化为GBNF grammars
  1.1 {subset of https://json-schema.org/} ----> GBNF grammars
2、有了https://json-schema.org子集，便可以使用llama.cpp项目中的功能将子集转化为GBNF grammar
*/
```

- In [llama-server](../examples/server):
    - For any completion endpoints, passed as the `json_schema` body field
    - For the `/chat/completions` endpoint, passed inside the `response_format` body field (e.g. `{"type", "json_object", "schema": {"items": {}}}` or `{ type: "json_schema", json_schema: {"schema": ...} }`)
    ```c
    /*
    Note:杨小兵-2024-12-25

    1、对于任何的completion endpoints可以通过传入json_schema字段
    2、对于/chat/completions endpoint可以在response_format body field内部传入schema
    */
    ```
- In [llama-cli](../examples/main), passed as the `--json` / `-j` flag
```c
/*
Note:杨小兵-2024-12-25

1、在llama-cli中可以通过传递--json/-j参数来实现schema
*/
```
- To convert to a grammar ahead of time:
    - in CLI, with [examples/json_schema_to_grammar.py](../examples/json_schema_to_grammar.py)
    - in JavaScript with [json-schema-to-grammar.mjs](../examples/server/public_legacy/json-schema-to-grammar.mjs) (this is used by the [server](../examples/server)'s Web UI)
    ```c
    /*
    Note:杨小兵-2024-12-25

    1、为了提前转换为语法
      1.1 在command line interface使用examples/json_schema_to_grammar.py实现提前转化
      1.2 在JavaScript中使用json-schema-to-grammar.mjs](../examples/server/public_legacy/json-schema-to-grammar.mjs提前进行转化，将会使用到Web UI中
    */
    ```

Take a look at [tests](../tests/test-json-schema-to-grammar.cpp) to see which features are likely supported (you'll also find usage examples in https://github.com/ggerganov/llama.cpp/pull/5978, https://github.com/ggerganov/llama.cpp/pull/6659 & https://github.com/ggerganov/llama.cpp/pull/6555).
```c
/*
Note:杨小兵-2024-12-25

1、查看 [tests](../tests/test-json-schema-to-grammar.cpp) 以了解可能支持哪些功能（您还可以在 https://github.com/ggerganov/llama.cpp/pull/5978、https://github.com/ggerganov/llama.cpp/pull/6659 和 https://github.com/ggerganov/llama.cpp/pull/6555 中找到使用示例）。
2、可以通过查看tests中的内容来了解支持哪些功能
3、可以通过查看tests中的内容来了解是如何实现从json schema ---> GBNF
*/
```

```bash
llama-cli \
  -hfr bartowski/Phi-3-medium-128k-instruct-GGUF \
  -hff Phi-3-medium-128k-instruct-Q8_0.gguf \
  -j '{
    "type": "array",
    "items": {
        "type": "object",
        "properties": {
            "name": {
                "type": "string",
                "minLength": 1,
                "maxLength": 100
            },
            "age": {
                "type": "integer",
                "minimum": 0,
                "maximum": 150
            }
        },
        "required": ["name", "age"],
        "additionalProperties": false
    },
    "minItems": 10,
    "maxItems": 100
  }' \
  -p 'Generate a {name, age}[] JSON array with famous actors of all ages.'
```

<details>

<summary>Show grammar</summary>

You can convert any schema in command-line with:
```c
/*
Note:杨小兵-2024-12-25

1、可以通过命令行实现任意的schema转化
*/
```

```bash
examples/json_schema_to_grammar.py name-age-schema.json
```

```
char ::= [^"\\\x7F\x00-\x1F] | [\\] (["\\bfnrt] | "u" [0-9a-fA-F]{4})
item ::= "{" space item-name-kv "," space item-age-kv "}" space
item-age ::= ([0-9] | ([1-8] [0-9] | [9] [0-9]) | "1" ([0-4] [0-9] | [5] "0")) space
item-age-kv ::= "\"age\"" space ":" space item-age
item-name ::= "\"" char{1,100} "\"" space
item-name-kv ::= "\"name\"" space ":" space item-name
root ::= "[" space item ("," space item){9,99} "]" space
space ::= | " " | "\n" [ \t]{0,20}
```

</details>

Here is also a list of known limitations (contributions welcome):

- `additionalProperties` defaults to `false` (produces faster grammars + reduces hallucinations).
- `"additionalProperties": true` may produce keys that contain unescaped newlines.
- Unsupported features are skipped silently. It is currently advised to use the command-line Python converter (see above) to see any warnings, and to inspect the resulting grammar / test it w/ [llama-gbnf-validator](../examples/gbnf-validator/gbnf-validator.cpp).
- Can't mix `properties` w/ `anyOf` / `oneOf` in the same type (https://github.com/ggerganov/llama.cpp/issues/7703)
- [prefixItems](https://json-schema.org/draft/2020-12/json-schema-core#name-prefixitems) is broken (but [items](https://json-schema.org/draft/2020-12/json-schema-core#name-items) works)
- `minimum`, `exclusiveMinimum`, `maximum`, `exclusiveMaximum`: only supported for `"type": "integer"` for now, not `number`
- Nested `$ref`s are broken (https://github.com/ggerganov/llama.cpp/issues/8073)
- [pattern](https://json-schema.org/draft/2020-12/json-schema-validation#name-pattern)s must start with `^` and end with `$`
- Remote `$ref`s not supported in the C++ version (Python & JavaScript versions fetch https refs)
- `string` [formats](https://json-schema.org/draft/2020-12/json-schema-validation#name-defined-formats) lack `uri`, `email`
- No [`patternProperties`](https://json-schema.org/draft/2020-12/json-schema-core#name-patternproperties)

And a non-exhaustive list of other unsupported features that are unlikely to be implemented (hard and/or too slow to support w/ stateless grammars):

- [`uniqueItems`](https://json-schema.org/draft/2020-12/json-schema-validation#name-uniqueitems)
- [`contains`](https://json-schema.org/draft/2020-12/json-schema-core#name-contains) / `minContains`
- `$anchor` (cf. [dereferencing](https://json-schema.org/draft/2020-12/json-schema-core#name-dereferencing))
- [`not`](https://json-schema.org/draft/2020-12/json-schema-core#name-not)
- [Conditionals](https://json-schema.org/draft/2020-12/json-schema-core#name-keywords-for-applying-subsche) `if` / `then` / `else` / `dependentSchemas`
```c
/*
Note:杨小兵-2024-12-25

1、上述罗列出的是一些转化存在的限制
*/
```

### A word about additionalProperties

> [!WARNING]
> The JSON schemas spec states `object`s accept [additional properties](https://json-schema.org/understanding-json-schema/reference/object#additionalproperties) by default.
> Since this is slow and seems prone to hallucinations, we default to no additional properties.
> You can set `"additionalProperties": true` in the the schema of any object to explicitly allow additional properties.
```c
/*
Note:杨小兵-2024-12-25

1、JSON 架构规范规定 `object` 默认接受 [附加属性](https://json-schema.org/understanding-json-schema/reference/object#additionalproperties)。
2、由于这很慢并且似乎容易产生幻觉，我们默认不接受附加属性。
3、您可以在任何对象的架构中设置 `"additionalProperties": true` 以明确允许附加属性。
*/
```

If you're using [Pydantic](https://pydantic.dev/) to generate schemas, you can enable additional properties with the `extra` config on each model class:
```c
/*
Note:杨小兵-2024-12-25

1、如果你使用 [Pydantic](https://pydantic.dev/) 来生成架构，则可以在每个模型类上使用 `extra` 配置启用附加属性
*/
```

```python
# pip install pydantic
import json
from typing import Annotated, List
from pydantic import BaseModel, Extra, Field
class QAPair(BaseModel):
    class Config:
        extra = 'allow'  # triggers additionalProperties: true in the JSON schema
    question: str
    concise_answer: str
    justification: str

class Summary(BaseModel):
    class Config:
        extra = 'allow'
    key_facts: List[Annotated[str, Field(pattern='- .{5,}')]]
    question_answers: List[Annotated[List[QAPair], Field(min_items=5)]]

print(json.dumps(Summary.model_json_schema(), indent=2))
```

<details>
<summary>Show JSON schema & grammar</summary>

```json
{
  "$defs": {
    "QAPair": {
      "additionalProperties": true,
      "properties": {
        "question": {
          "title": "Question",
          "type": "string"
        },
        "concise_answer": {
          "title": "Concise Answer",
          "type": "string"
        },
        "justification": {
          "title": "Justification",
          "type": "string"
        }
      },
      "required": [
        "question",
        "concise_answer",
        "justification"
      ],
      "title": "QAPair",
      "type": "object"
    }
  },
  "additionalProperties": true,
  "properties": {
    "key_facts": {
      "items": {
        "pattern": "^- .{5,}$",
        "type": "string"
      },
      "title": "Key Facts",
      "type": "array"
    },
    "question_answers": {
      "items": {
        "items": {
          "$ref": "#/$defs/QAPair"
        },
        "minItems": 5,
        "type": "array"
      },
      "title": "Question Answers",
      "type": "array"
    }
  },
  "required": [
    "key_facts",
    "question_answers"
  ],
  "title": "Summary",
  "type": "object"
}
```

```
QAPair ::= "{" space QAPair-question-kv "," space QAPair-concise-answer-kv "," space QAPair-justification-kv ( "," space ( QAPair-additional-kv ( "," space QAPair-additional-kv )* ) )? "}" space
QAPair-additional-k ::= ["] ( [c] ([o] ([n] ([c] ([i] ([s] ([e] ([_] ([a] ([n] ([s] ([w] ([e] ([r] char+ | [^"r] char*) | [^"e] char*) | [^"w] char*) | [^"s] char*) | [^"n] char*) | [^"a] char*) | [^"_] char*) | [^"e] char*) | [^"s] char*) | [^"i] char*) | [^"c] char*) | [^"n] char*) | [^"o] char*) | [j] ([u] ([s] ([t] ([i] ([f] ([i] ([c] ([a] ([t] ([i] ([o] ([n] char+ | [^"n] char*) | [^"o] char*) | [^"i] char*) | [^"t] char*) | [^"a] char*) | [^"c] char*) | [^"i] char*) | [^"f] char*) | [^"i] char*) | [^"t] char*) | [^"s] char*) | [^"u] char*) | [q] ([u] ([e] ([s] ([t] ([i] ([o] ([n] char+ | [^"n] char*) | [^"o] char*) | [^"i] char*) | [^"t] char*) | [^"s] char*) | [^"e] char*) | [^"u] char*) | [^"cjq] char* )? ["] space
QAPair-additional-kv ::= QAPair-additional-k ":" space value
QAPair-concise-answer-kv ::= "\"concise_answer\"" space ":" space string
QAPair-justification-kv ::= "\"justification\"" space ":" space string
QAPair-question-kv ::= "\"question\"" space ":" space string
additional-k ::= ["] ( [k] ([e] ([y] ([_] ([f] ([a] ([c] ([t] ([s] char+ | [^"s] char*) | [^"t] char*) | [^"c] char*) | [^"a] char*) | [^"f] char*) | [^"_] char*) | [^"y] char*) | [^"e] char*) | [q] ([u] ([e] ([s] ([t] ([i] ([o] ([n] ([_] ([a] ([n] ([s] ([w] ([e] ([r] ([s] char+ | [^"s] char*) | [^"r] char*) | [^"e] char*) | [^"w] char*) | [^"s] char*) | [^"n] char*) | [^"a] char*) | [^"_] char*) | [^"n] char*) | [^"o] char*) | [^"i] char*) | [^"t] char*) | [^"s] char*) | [^"e] char*) | [^"u] char*) | [^"kq] char* )? ["] space
additional-kv ::= additional-k ":" space value
array ::= "[" space ( value ("," space value)* )? "]" space
boolean ::= ("true" | "false") space
char ::= [^"\\\x7F\x00-\x1F] | [\\] (["\\bfnrt] | "u" [0-9a-fA-F]{4})
decimal-part ::= [0-9]{1,16}
dot ::= [^\x0A\x0D]
integral-part ::= [0] | [1-9] [0-9]{0,15}
key-facts ::= "[" space (key-facts-item ("," space key-facts-item)*)? "]" space
key-facts-item ::= "\"" "- " key-facts-item-1{5,} "\"" space
key-facts-item-1 ::= dot
key-facts-kv ::= "\"key_facts\"" space ":" space key-facts
null ::= "null" space
number ::= ("-"? integral-part) ("." decimal-part)? ([eE] [-+]? integral-part)? space
object ::= "{" space ( string ":" space value ("," space string ":" space value)* )? "}" space
question-answers ::= "[" space (question-answers-item ("," space question-answers-item)*)? "]" space
question-answers-item ::= "[" space question-answers-item-item ("," space question-answers-item-item){4,} "]" space
question-answers-item-item ::= QAPair
question-answers-kv ::= "\"question_answers\"" space ":" space question-answers
root ::= "{" space key-facts-kv "," space question-answers-kv ( "," space ( additional-kv ( "," space additional-kv )* ) )? "}" space
space ::= | " " | "\n" [ \t]{0,20}
string ::= "\"" char* "\"" space
value ::= object | array | string | number | boolean | null
```

</details>

If you're using [Zod](https://zod.dev/), you can make your objects to explicitly allow extra properties w/ `nonstrict()` / `passthrough()` (or explicitly no extra props w/ `z.object(...).strict()` or `z.strictObject(...)`) but note that [zod-to-json-schema](https://github.com/StefanTerdell/zod-to-json-schema) currently always sets `"additionalProperties": false` anyway.

```js
import { z } from 'zod';
import { zodToJsonSchema } from 'zod-to-json-schema';

const Foo = z.object({
  age: z.number().positive(),
  email: z.string().email(),
}).strict();

console.log(zodToJsonSchema(Foo));
```

<details>
<summary>Show JSON schema & grammar</summary>

```json
{
  "type": "object",
  "properties": {
    "age": {
      "type": "number",
      "exclusiveMinimum": 0
    },
    "email": {
      "type": "string",
      "format": "email"
    }
  },
  "required": [
    "age",
    "email"
  ],
  "additionalProperties": false,
  "$schema": "http://json-schema.org/draft-07/schema#"
}
```

```
age-kv ::= "\"age\"" space ":" space number
char ::= [^"\\\x7F\x00-\x1F] | [\\] (["\\bfnrt] | "u" [0-9a-fA-F]{4})
decimal-part ::= [0-9]{1,16}
email-kv ::= "\"email\"" space ":" space string
integral-part ::= [0] | [1-9] [0-9]{0,15}
number ::= ("-"? integral-part) ("." decimal-part)? ([eE] [-+]? integral-part)? space
root ::= "{" space age-kv "," space email-kv "}" space
space ::= | " " | "\n" [ \t]{0,20}
string ::= "\"" char* "\"" space
```

</details>
