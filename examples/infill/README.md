# 1 llama.cpp/example/infill
```c
/*
Note:杨小兵-2025-01-19

1、这是llama.cpp项目中的一个名为infill的示例
2、infill是llama.cpp项目中的一个子项目名称
3、“infill” 的中文翻译因其应用领域的不同而有所变化。常见的译法包括“填充”、“内填”、“填充准则”、“填充数据”等。
*/
```
This example shows how to use the infill mode with Code Llama models supporting infill mode.
Currently the 7B and 13B models support infill mode.
```c
/*
Note:杨小兵-2025-01-19

1、此示例展示了如何在支持infill mode的 Code Llama 模型中使用infill mode。目前 7B 和 13B 模型支持填充模式。
*/
```
Infill supports most of the options available in the main example.
```c
/*
Note:杨小兵-2025-01-19

1、Infill项目支持llama.cpp/example/main/中可用的大多数选项。
*/
```

For further information have a look at the main README.md in llama.cpp/example/main/README.md
```c
/*
Note:杨小兵-2025-01-19

1、想要了解更多的信息可以查看llama.cpp/example/main/README.md中的内容。
*/
```

## 1.2 Common Options
```c
/*
Note:杨小兵-2025-01-20

1、这部分内容将会解释一些infill项目中使用到的一些常见选项。
2、llama-infill可执行文件的一些选项同llama-cli可执行文件中的一些选项是相同的。
*/
```

In this section, we cover the most commonly used options for running the `infill` program with the LLaMA models:

-   `-m FNAME, --model FNAME`: Specify the path to the LLaMA model file (e.g., `models/7B/ggml-model.bin`).
-   `-i, --interactive`: Run the program in interactive mode, allowing you to provide input directly and receive real-time responses.
-   `-n N, --n-predict N`: Set the number of tokens to predict when generating text. Adjusting this value can influence the length of the generated text.
-   `-c N, --ctx-size N`: Set the size of the prompt context. The default is 4096, but if a LLaMA model was built with a longer context, increasing this value will provide better results for longer input/inference.
-   `--spm-infill`: Use Suffix/Prefix/Middle pattern for infill (instead of Prefix/Suffix/Middle) as some models prefer this.
```c
/*
Note:杨小兵-2025-01-20

1、在这个部分中，为了使用llama模型运行llama-infill可执行程序，我们将会覆盖一些最为常用的命令行参数。
    1.1 `-m FNAME, --model FNAME`:制定llama 模型文件所在的路径
    1.2 `-i, --interactive`:以interactive mode运行llama-infill可执行文件，这个命令行参数运行你直接提供输入和获取得到real-time回复。
    1.3 `-n N, --n-predict N`:在生成文本的过程中设置将会预测的tokens数量。调整这个值可以影响生成文本的长度。
    1.4 `-c N, --ctx-size N`:设置prompt context的大小，默认值是4096，但是如果一个llama模型使用一个更长上下文被构建的，对于更长的输入、推理增加这个值将会提供更好的结果。
    1.5 `--spm-infill`:使用后缀/前缀/中间模式进行填充（而不是前缀/后缀/中间），因为有些模型更喜欢这样。
*/
```

## 1.3 Input Prompts

The `infill` program provides several ways to interact with the LLaMA models using input prompts:

-   `--in-prefix PROMPT_BEFORE_CURSOR`: Provide the prefix directly as a command-line option.
-   `--in-suffix PROMPT_AFTER_CURSOR`: Provide the suffix directly as a command-line option.
-   `--interactive-first`: Run the program in interactive mode and wait for input right away. (More on this below.)
```c
/*
Note:杨小兵-2025-01-20

1、`llama-infill` 程序提供了几种使用输入提示与 LLaMA 模型交互的方法：
    1.1 `--in-prefix PROMPT_BEFORE_CURSOR`
    1.2 `--in-suffix PROMPT_AFTER_CURSOR`
    1.3 `--interactive-first`
*/
```


## 1.4 Interaction

The `infill` program offers a seamless way to interact with LLaMA models, allowing users to receive real-time infill suggestions. The interactive mode can be triggered using `--interactive`, and `--interactive-first`
```c
/*
Note:杨小兵-2025-01-20

1、`infill` 程序提供了一种与 LLaMA 模型无缝交互的方式，让用户可以接收实时填充建议。可以使用 `--interactive` 和 `--interactive-first` 触发交互模式。
2、infill 程序为与 LLaMA（Large Language Model Meta AI）模型的交互提供了无缝的解决方案，允许用户接收实时的补全文本建议。通过使用 --interactive 和 --interactive-first 标志，可以触发交互模式，从而增强用户体验。
3、infill 程序通过提供与 LLaMA 模型的无缝交互，实现在各种应用中实时的补全文本建议，从而提升用户互动和生产力。利用诸如 --interactive 和 --interactive-first 的交互模式，用户可以动态地与模型互动，接收即时且上下文相关的补全建议，简化工作流程。无论是用于写作、编码还是其他基于文本的任务，infill 程序都是利用先进语言模型能力的有价值工具。
*/
```

### 1.4.1 Interaction Options

-   `-i, --interactive`: Run the program in interactive mode, allowing users to get real time code suggestions from model.
-   `--interactive-first`: Run the program in interactive mode and immediately wait for user input before starting the text generation.
-   `--color`: Enable colorized output to differentiate visually distinguishing between prompts, user input, and generated text.
```c
/*
Note:杨小兵-2025-01-20

1、`-i, --interactive`:以交互模式运行程序从而运行用户可以从model中获取real time代码建议。
2、`--interactive-first`:以交互模式运行程序并且可以在开始文本生成之前立马进入等待获取用户输入。
3、`--color`:启用彩色输出以在视觉上区分提示、用户输入和生成的文本。
*/
```

### 1.4.2 Example

Download a model that supports infill, for example CodeLlama:
```console
scripts/hf.sh --repo TheBloke/CodeLlama-13B-GGUF --file codellama-13b.Q5_K_S.gguf --outdir models
```

```bash
./llama-infill -t 10 -ngl 0 -m models/codellama-13b.Q5_K_S.gguf -c 4096 --temp 0.7 --repeat_penalty 1.1 -n 20 --in-prefix "def helloworld():\n    print(\"hell" --in-suffix "\n   print(\"goodbye world\")\n    "
```
```c
/*
Note:杨小兵-2025-01-20

1、这里的示例没有及时更新。
*/
```
