# llama.cpp
```c
/*
Notes:杨小兵-2025-04-08

1、当前分支是我为了学习整个llama.cpp项目fork出来的一个分支。
2、学习的目标
    2.1 对llama.cpp项目的整体理解。
    2.2 目前不对实现的细节进行过多深入了解，这是后续将会学习的内容。
    2.3 需要明确学习的目标从而有阶段性的深入理解。
3、llama.cpp是该项目的名称，存在一定的历史含义（最初是由meta公司的开源模型llama而来）。
*/
```

![llama](https://user-images.githubusercontent.com/1991296/230134379-7181e485-c521-4d23-a0d6-f7b3b61ba524.png)

[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![Server](https://github.com/ggml-org/llama.cpp/actions/workflows/server.yml/badge.svg)](https://github.com/ggml-org/llama.cpp/actions/workflows/server.yml)
```c
/*
Notes:杨小兵-2025-04-08

1、![llama](https://user-images.githubusercontent.com/1991296/230134379-7181e485-c521-4d23-a0d6-f7b3b61ba524.png)
    1.1 这种语法可以在markdown文件中显示一张图片，如果图片的资源是不存在的那么markdown渲染将会使用 llama 来代替图片。
    1.2 图片资源可以是网络中存在的或者是本地中存在的。
2、[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT)
    2.1 这种语法可以使得我们直接点击图片从而跳转到对应的网址中，这也是使用用户交互比较方便的一种语法，非常实用。如果图片资源是不存在的，那么markdown渲染器将会使用 License: MIT 来代替图片。
3、[![Server](https://github.com/ggml-org/llama.cpp/actions/workflows/server.yml/badge.svg)](https://github.com/ggml-org/llama.cpp/actions/workflows/server.yml)
    3.1 这种语法可以使得我们直接点击图片从而跳转到对应的网址中，这也是使用用户交互比较方便的一种语法，非常实用。如果图片资源是不存在的，那么markdown渲染器将会使用 Server 来代替图片。
*/
```
[Roadmap](https://github.com/users/ggml-org/projects/7) / [Project status](https://github.com/ggml-org/llama.cpp/discussions/3471) / [Manifesto](https://github.com/ggml-org/llama.cpp/discussions/205) / [ggml](https://github.com/ggml-org/ggml)
```c
/*
Notes:杨小兵-2025-04-08

1、上述的四个链接是markdown文件中特有的书写方式，被渲染之后可以通过点击对应的提示文字直接跳转到对应的网址中。
2、在markdown中渲染资源的方式
    2.1 渲染图片（单纯的图片展示）
    2.2 渲染链接（单纯的链接展示）
    2.3 渲染图片链接（通过点击图片跳转到对应的网址中）
    2.4 渲染文字链接（通过点击文字跳转到对应的网址中）
*/
```

Inference of Meta's [LLaMA](https://arxiv.org/abs/2302.13971) model (and others) in pure C/C++

> [!IMPORTANT]
> New `llama.cpp` package location: [ggml-org/llama.cpp](https://github.com/ggml-org/llama.cpp/pkgs/container/llama.cpp)
>
> Update your container URLs to: `ghcr.io/ggml-org/llama.cpp`
>
> More info: https://github.com/ggml-org/llama.cpp/discussions/11801
```c
/*
Notes:杨小兵-2025-04-08

1、使用纯C/C++实现Meta模型（其他模型）的推理
    1.1 只使用C/C++（问题：具体的编程语言版本是什么？针对每一个项目可以查看具体的构建脚本操作确定使用编程语言的版本信息，这里的构建脚本指的就是CMakeLists.txt）
    1.2 这里的'其他模型'现在已经成为一个重点。
2、重点解读
    2.1 llama.cpp package现在位于新的仓库位置（说明之前不是这个位置，已经发生了变化，应该是为了更好的结构化组织）
    2.2 目前在使用container进行部署llama.cpp的时候需要更新对应的URLs（这部分内容还没有接触过，在有对应的需求的时候可以探索这部分内容）
    2.3 可以到对应的页面查看更多的信息（关于仓库转移的内容，其实我现在不知道转移的动机是什么？）
        2.3.1 仓库转移（repository transfer）是指将一个 GitHub 仓库从一个用户或组织账户移动到另一个用户或组织账户的过程。
        2.3.2 对话背景：ggerganov 可能是 llama.cpp 项目的初始创建者，但随着项目发展，可能有更多贡献者加入，或者项目成为一个更广泛的社区努力（如 ggml-org 组织）。转移到组织账户可以让项目更正式地归属于一个团队，而不是单一的个人。
        2.3.3 分析：组织账户（如 ggml-org）允许多个成员拥有不同级别的权限（例如管理员、写权限、读权限），这比个人账户更适合管理大型项目。此外，组织账户可以更好地体现项目的“品牌”或“社区”身份，而不仅仅是某个个人的作品。
*/
```

## Recent API changes

- [Changelog for `libllama` API](https://github.com/ggml-org/llama.cpp/issues/9289)
- [Changelog for `llama-server` REST API](https://github.com/ggml-org/llama.cpp/issues/9291)

```c
/*
Notes:杨小兵-2025-04-08

1、最近的API变化
    1.1 通过指定链接来说明libllama API的变化内容
    1.2 通过指定链接来说明llama-server REST API的变化内容
*/
```

## Hot topics

- **How to use [MTLResidencySet](https://developer.apple.com/documentation/metal/mtlresidencyset?language=objc) to keep the GPU memory active?** https://github.com/ggml-org/llama.cpp/pull/11427
- **VS Code extension for FIM completions:** https://github.com/ggml-org/llama.vscode
- Universal tool call support in `llama-server`: https://github.com/ggml-org/llama.cpp/pull/9639
- Vim/Neovim plugin for FIM completions: https://github.com/ggml-org/llama.vim
- Introducing GGUF-my-LoRA https://github.com/ggml-org/llama.cpp/discussions/10123
- Hugging Face Inference Endpoints now support GGUF out of the box! https://github.com/ggml-org/llama.cpp/discussions/9669
- Hugging Face GGUF editor: [discussion](https://github.com/ggml-org/llama.cpp/discussions/9268) | [tool](https://huggingface.co/spaces/CISCai/gguf-editor)
```c
/*
Notes:杨小兵-2025-04-08

1、热门话题
    1.1 如何使用MTLResidencySet来使得GPU memory是活跃的（MTLResidencySet是metal中的一部分内容，用来管理GPU的VRAM，使得程序员可以控制资源驻留在VRAM中而不会使其在某些情况下被转移到Disk中，通过这种方式来降低资源转移带来的时间开销）
    1.2 在VS Code中存在对FIM补全的拓展（FIM 是 "Fill-In-the-Middle" 的缩写，意思是“填充中间”。它是一些语言模型提供的一种功能（有些LLMs将会提供这种功能），可以根据你给定的代码或文本的开头和结尾，智能地生成中间缺失的部分。简单来说，FIM 就像是一个超级厉害的代码补全工具，但它不仅仅是猜下一行代码，而是能理解上下文，生成整个函数或代码块。）
    1.3 在llama-server存在通用工具调用支持（这部分内容在后续会进行重点关注）
    1.4 对于FIM 补全有对应的Vim/Neovim插件支持
    1.5 有关GGUF-my-LoRA内容（这部分内容后续了解）
    1.6 Hugging Face Inference Endpoints现在支持GGUF（这部分内容后续了解）
    1.7 Hugging Face中现在有对GGUF进行修改的编辑器（这部分内容后续了解）
2、这部分中的大部分内容可以在后续进行了解，llama-server中的Universal tool call support将会成为重点关注对象。
*/
```
----

## Description

The main goal of `llama.cpp` is to enable LLM inference with minimal setup and state-of-the-art performance on a wide
range of hardware - locally and in the cloud.
```c
/*
Notes:杨小兵-2025-04-08

1、llama.cpp项目的主要目标
    1.1 最少的设置实现LLM inference
    1.2 最先进的性能实现LLM inference
    1.3 在很多不同的硬件（本地、云端）实现LLM inference
*/
```
- Plain C/C++ implementation without any dependencies
- Apple silicon is a first-class citizen - optimized via ARM NEON, Accelerate and Metal frameworks
- AVX, AVX2, AVX512 and AMX support for x86 architectures
- 1.5-bit, 2-bit, 3-bit, 4-bit, 5-bit, 6-bit, and 8-bit integer quantization for faster inference and reduced memory use
- Custom CUDA kernels for running LLMs on NVIDIA GPUs (support for AMD GPUs via HIP and Moore Threads MTT GPUs via MUSA)
- Vulkan and SYCL backend support
- CPU+GPU hybrid inference to partially accelerate models larger than the total VRAM capacity
```c
/*
Notes:杨小兵-2025-04-08

1、没有任何依赖的、纯C/C++实现
2、Apple silicon将会是被优先考虑的，通过ARM NEON、Metal frameworks进行优化（Apple silicon是苹果设备（如 Mac 的 M1、M2 芯片）用的 ARM 架构处理器，与传统 x86 不同。ARM NEON 是 ARM 处理器的一种 SIMD 指令集，允许一次处理多个数据，类似并行计算。Accelerate 是苹果提供的计算框架，优化线性代数和信号处理。Metal 是苹果的 GPU 编程接口，类似 OpenGL，但专为苹果设备设计。）
    2.1 ARM NEON 是 ARM 处理器的一个 SIMD（单指令多数据）指令集扩展。
    2.2 Accelerate 是 Apple 提供的一个框架，包含一系列优化库，专门用于高性能计算任务，比如矩阵运算、信号处理和图像操作。
    2.3 Metal 是 Apple 提供的一个低级 API，允许你直接访问 GPU 来执行图形渲染和并行计算任务。这特别适合需要高性能的应用程序，比如游戏或科学模拟。它让你可以利用 GPU 的并行处理能力来加速计算。
    2.4 总结一下，ARM NEON 是 CPU 的 SIMD 指令集，Accelerate 是优化计算的库集合，Metal 是用于 GPU 编程的框架（metal是软件接口，可以使用这个软件接口从而来控制GPU）。
3、对于x86指令集架构支持AVX, AVX2, AVX512, AMX（AVX（Advanced Vector Extensions）是 x86 处理器（如 Intel、AMD）的扩展指令集，支持 SIMD 操作。AVX 支持 256 位寄存器，AVX2 增加整数支持，AVX512 扩展到 512 位。AMX 可能为苹果矩阵协处理器，但这里可能误写，实际指 x86 的高级计算支持。这些指令让 CPU 并行处理数据，加速计算。）
4、对于更快速的推理和减少内存使用进行1.5-bit, 2-bit, 3-bit, 4-bit, 5-bit, 6-bit, and 8-bit integer quantization
5、通过定制化的 CUDA 内核（kernels）来优化大型语言模型（LLMs）在 NVIDIA GPU 上的运行，同时提到对 AMD GPU（通过 HIP）和 Moore Threads MTT GPU（通过 MUSA）的支持。
    5.1 CUDA Kerneal是使用C++语言编写的运行在NVIDIA GPU上的函数，CUDA Kernal不能运行在其他类型的GPU和CPU上，这就说明CUDA Kernal是专门为NVIDIA GPU设计的。
    5.2 如果想要运行CUDA Kernal那么需要NVIDIA GPU硬件和NVIDIA GPU driver。
    5.3 CUDA Kernal在NVIDIA GPU上运行利用的就是NVIDIA GPU的多线程并行能力。
6、llama.cpp项目支持Vulkan、SYCL后端（Vulkan 是跨平台的 GPU 计算接口，支持 Windows、Linux 等，类似 OpenGL，但更低级，适合高性能计算。SYCL 是一种统一编程模型，允许用 C++ 写代码运行在 CPU、GPU、FPGA 等硬件上，增强跨平台兼容性。支持这些后端让软件运行在多种硬件上。）
    6.1 Vulkan是需要了解的。
    6.2 SYCL也是需要了解的。
7、当模型所需要的VRAM要比硬件本身的VRAM capacity还要大的时候可以通过CPU+GPU混合推理实现部分加速。
*/
```
The `llama.cpp` project is the main playground for developing new features for the [ggml](https://github.com/ggml-org/ggml) library.
```c
/*
Notes:杨小兵-2025-04-08

1、llama.cpp 项目是开发和测试 ggml 库新功能的主要实验平台。ggml 是一个托管在 GitHub 上的开源库（位于 ggml-org/ggml 仓库），而 llama.cpp 作为一个独立项目，为开发者提供了一个实践环境，通过在其中实现和验证新特性，推动 ggml 库的功能完善和扩展。
    1.1 后续将会对GGML进行详细的了解。
2、下列内容展示的是llama.cpp项目支持的模型、工具等等。
*/
```

<details>
<summary>Models</summary>

Typically finetunes of the base models below are supported as well.

Instructions for adding support for new models: [HOWTO-add-model.md](docs/development/HOWTO-add-model.md)

#### Text-only

- [X] LLaMA 🦙
- [x] LLaMA 2 🦙🦙
- [x] LLaMA 3 🦙🦙🦙
- [X] [Mistral 7B](https://huggingface.co/mistralai/Mistral-7B-v0.1)
- [x] [Mixtral MoE](https://huggingface.co/models?search=mistral-ai/Mixtral)
- [x] [DBRX](https://huggingface.co/databricks/dbrx-instruct)
- [X] [Falcon](https://huggingface.co/models?search=tiiuae/falcon)
- [X] [Chinese LLaMA / Alpaca](https://github.com/ymcui/Chinese-LLaMA-Alpaca) and [Chinese LLaMA-2 / Alpaca-2](https://github.com/ymcui/Chinese-LLaMA-Alpaca-2)
- [X] [Vigogne (French)](https://github.com/bofenghuang/vigogne)
- [X] [BERT](https://github.com/ggml-org/llama.cpp/pull/5423)
- [X] [Koala](https://bair.berkeley.edu/blog/2023/04/03/koala/)
- [X] [Baichuan 1 & 2](https://huggingface.co/models?search=baichuan-inc/Baichuan) + [derivations](https://huggingface.co/hiyouga/baichuan-7b-sft)
- [X] [Aquila 1 & 2](https://huggingface.co/models?search=BAAI/Aquila)
- [X] [Starcoder models](https://github.com/ggml-org/llama.cpp/pull/3187)
- [X] [Refact](https://huggingface.co/smallcloudai/Refact-1_6B-fim)
- [X] [MPT](https://github.com/ggml-org/llama.cpp/pull/3417)
- [X] [Bloom](https://github.com/ggml-org/llama.cpp/pull/3553)
- [x] [Yi models](https://huggingface.co/models?search=01-ai/Yi)
- [X] [StableLM models](https://huggingface.co/stabilityai)
- [x] [Deepseek models](https://huggingface.co/models?search=deepseek-ai/deepseek)
- [x] [Qwen models](https://huggingface.co/models?search=Qwen/Qwen)
- [x] [PLaMo-13B](https://github.com/ggml-org/llama.cpp/pull/3557)
- [x] [Phi models](https://huggingface.co/models?search=microsoft/phi)
- [x] [PhiMoE](https://github.com/ggml-org/llama.cpp/pull/11003)
- [x] [GPT-2](https://huggingface.co/gpt2)
- [x] [Orion 14B](https://github.com/ggml-org/llama.cpp/pull/5118)
- [x] [InternLM2](https://huggingface.co/models?search=internlm2)
- [x] [CodeShell](https://github.com/WisdomShell/codeshell)
- [x] [Gemma](https://ai.google.dev/gemma)
- [x] [Mamba](https://github.com/state-spaces/mamba)
- [x] [Grok-1](https://huggingface.co/keyfan/grok-1-hf)
- [x] [Xverse](https://huggingface.co/models?search=xverse)
- [x] [Command-R models](https://huggingface.co/models?search=CohereForAI/c4ai-command-r)
- [x] [SEA-LION](https://huggingface.co/models?search=sea-lion)
- [x] [GritLM-7B](https://huggingface.co/GritLM/GritLM-7B) + [GritLM-8x7B](https://huggingface.co/GritLM/GritLM-8x7B)
- [x] [OLMo](https://allenai.org/olmo)
- [x] [OLMo 2](https://allenai.org/olmo)
- [x] [OLMoE](https://huggingface.co/allenai/OLMoE-1B-7B-0924)
- [x] [Granite models](https://huggingface.co/collections/ibm-granite/granite-code-models-6624c5cec322e4c148c8b330)
- [x] [GPT-NeoX](https://github.com/EleutherAI/gpt-neox) + [Pythia](https://github.com/EleutherAI/pythia)
- [x] [Snowflake-Arctic MoE](https://huggingface.co/collections/Snowflake/arctic-66290090abe542894a5ac520)
- [x] [Smaug](https://huggingface.co/models?search=Smaug)
- [x] [Poro 34B](https://huggingface.co/LumiOpen/Poro-34B)
- [x] [Bitnet b1.58 models](https://huggingface.co/1bitLLM)
- [x] [Flan T5](https://huggingface.co/models?search=flan-t5)
- [x] [Open Elm models](https://huggingface.co/collections/apple/openelm-instruct-models-6619ad295d7ae9f868b759ca)
- [x] [ChatGLM3-6b](https://huggingface.co/THUDM/chatglm3-6b) + [ChatGLM4-9b](https://huggingface.co/THUDM/glm-4-9b) + [GLMEdge-1.5b](https://huggingface.co/THUDM/glm-edge-1.5b-chat) + [GLMEdge-4b](https://huggingface.co/THUDM/glm-edge-4b-chat)
- [x] [SmolLM](https://huggingface.co/collections/HuggingFaceTB/smollm-6695016cad7167254ce15966)
- [x] [EXAONE-3.0-7.8B-Instruct](https://huggingface.co/LGAI-EXAONE/EXAONE-3.0-7.8B-Instruct)
- [x] [FalconMamba Models](https://huggingface.co/collections/tiiuae/falconmamba-7b-66b9a580324dd1598b0f6d4a)
- [x] [Jais](https://huggingface.co/inceptionai/jais-13b-chat)
- [x] [Bielik-11B-v2.3](https://huggingface.co/collections/speakleash/bielik-11b-v23-66ee813238d9b526a072408a)
- [x] [RWKV-6](https://github.com/BlinkDL/RWKV-LM)
- [x] [QRWKV-6](https://huggingface.co/recursal/QRWKV6-32B-Instruct-Preview-v0.1)
- [x] [GigaChat-20B-A3B](https://huggingface.co/ai-sage/GigaChat-20B-A3B-instruct)

#### Multimodal

- [x] [LLaVA 1.5 models](https://huggingface.co/collections/liuhaotian/llava-15-653aac15d994e992e2677a7e), [LLaVA 1.6 models](https://huggingface.co/collections/liuhaotian/llava-16-65b9e40155f60fd046a5ccf2)
- [x] [BakLLaVA](https://huggingface.co/models?search=SkunkworksAI/Bakllava)
- [x] [Obsidian](https://huggingface.co/NousResearch/Obsidian-3B-V0.5)
- [x] [ShareGPT4V](https://huggingface.co/models?search=Lin-Chen/ShareGPT4V)
- [x] [MobileVLM 1.7B/3B models](https://huggingface.co/models?search=mobileVLM)
- [x] [Yi-VL](https://huggingface.co/models?search=Yi-VL)
- [x] [Mini CPM](https://huggingface.co/models?search=MiniCPM)
- [x] [Moondream](https://huggingface.co/vikhyatk/moondream2)
- [x] [Bunny](https://github.com/BAAI-DCAI/Bunny)
- [x] [GLM-EDGE](https://huggingface.co/models?search=glm-edge)
- [x] [Qwen2-VL](https://huggingface.co/collections/Qwen/qwen2-vl-66cee7455501d7126940800d)

</details>

<details>
<summary>Bindings</summary>

- Python: [abetlen/llama-cpp-python](https://github.com/abetlen/llama-cpp-python)
- Go: [go-skynet/go-llama.cpp](https://github.com/go-skynet/go-llama.cpp)
- Node.js: [withcatai/node-llama-cpp](https://github.com/withcatai/node-llama-cpp)
- JS/TS (llama.cpp server client): [lgrammel/modelfusion](https://modelfusion.dev/integration/model-provider/llamacpp)
- JS/TS (Programmable Prompt Engine CLI): [offline-ai/cli](https://github.com/offline-ai/cli)
- JavaScript/Wasm (works in browser): [tangledgroup/llama-cpp-wasm](https://github.com/tangledgroup/llama-cpp-wasm)
- Typescript/Wasm (nicer API, available on npm): [ngxson/wllama](https://github.com/ngxson/wllama)
- Ruby: [yoshoku/llama_cpp.rb](https://github.com/yoshoku/llama_cpp.rb)
- Rust (more features): [edgenai/llama_cpp-rs](https://github.com/edgenai/llama_cpp-rs)
- Rust (nicer API): [mdrokz/rust-llama.cpp](https://github.com/mdrokz/rust-llama.cpp)
- Rust (more direct bindings): [utilityai/llama-cpp-rs](https://github.com/utilityai/llama-cpp-rs)
- Rust (automated build from crates.io): [ShelbyJenkins/llm_client](https://github.com/ShelbyJenkins/llm_client)
- C#/.NET: [SciSharp/LLamaSharp](https://github.com/SciSharp/LLamaSharp)
- C#/VB.NET (more features - community license): [LM-Kit.NET](https://docs.lm-kit.com/lm-kit-net/index.html)
- Scala 3: [donderom/llm4s](https://github.com/donderom/llm4s)
- Clojure: [phronmophobic/llama.clj](https://github.com/phronmophobic/llama.clj)
- React Native: [mybigday/llama.rn](https://github.com/mybigday/llama.rn)
- Java: [kherud/java-llama.cpp](https://github.com/kherud/java-llama.cpp)
- Zig: [deins/llama.cpp.zig](https://github.com/Deins/llama.cpp.zig)
- Flutter/Dart: [netdur/llama_cpp_dart](https://github.com/netdur/llama_cpp_dart)
- Flutter: [xuegao-tzx/Fllama](https://github.com/xuegao-tzx/Fllama)
- PHP (API bindings and features built on top of llama.cpp): [distantmagic/resonance](https://github.com/distantmagic/resonance) [(more info)](https://github.com/ggml-org/llama.cpp/pull/6326)
- Guile Scheme: [guile_llama_cpp](https://savannah.nongnu.org/projects/guile-llama-cpp)
- Swift [srgtuszy/llama-cpp-swift](https://github.com/srgtuszy/llama-cpp-swift)
- Swift [ShenghaiWang/SwiftLlama](https://github.com/ShenghaiWang/SwiftLlama)

</details>

<details>
<summary>UIs</summary>

*(to have a project listed here, it should clearly state that it depends on `llama.cpp`)*

- [AI Sublime Text plugin](https://github.com/yaroslavyaroslav/OpenAI-sublime-text) (MIT)
- [cztomsik/ava](https://github.com/cztomsik/ava) (MIT)
- [Dot](https://github.com/alexpinel/Dot) (GPL)
- [eva](https://github.com/ylsdamxssjxxdd/eva) (MIT)
- [iohub/collama](https://github.com/iohub/coLLaMA) (Apache-2.0)
- [janhq/jan](https://github.com/janhq/jan) (AGPL)
- [KanTV](https://github.com/zhouwg/kantv?tab=readme-ov-file) (Apache-2.0)
- [KodiBot](https://github.com/firatkiral/kodibot) (GPL)
- [llama.vim](https://github.com/ggml-org/llama.vim) (MIT)
- [LARS](https://github.com/abgulati/LARS) (AGPL)
- [Llama Assistant](https://github.com/vietanhdev/llama-assistant) (GPL)
- [LLMFarm](https://github.com/guinmoon/LLMFarm?tab=readme-ov-file) (MIT)
- [LLMUnity](https://github.com/undreamai/LLMUnity) (MIT)
- [LMStudio](https://lmstudio.ai/) (proprietary)
- [LocalAI](https://github.com/mudler/LocalAI) (MIT)
- [LostRuins/koboldcpp](https://github.com/LostRuins/koboldcpp) (AGPL)
- [MindMac](https://mindmac.app) (proprietary)
- [MindWorkAI/AI-Studio](https://github.com/MindWorkAI/AI-Studio) (FSL-1.1-MIT)
- [Mobile-Artificial-Intelligence/maid](https://github.com/Mobile-Artificial-Intelligence/maid) (MIT)
- [Mozilla-Ocho/llamafile](https://github.com/Mozilla-Ocho/llamafile) (Apache-2.0)
- [nat/openplayground](https://github.com/nat/openplayground) (MIT)
- [nomic-ai/gpt4all](https://github.com/nomic-ai/gpt4all) (MIT)
- [ollama/ollama](https://github.com/ollama/ollama) (MIT)
- [oobabooga/text-generation-webui](https://github.com/oobabooga/text-generation-webui) (AGPL)
- [PocketPal AI](https://github.com/a-ghorbani/pocketpal-ai) (MIT)
- [psugihara/FreeChat](https://github.com/psugihara/FreeChat) (MIT)
- [ptsochantaris/emeltal](https://github.com/ptsochantaris/emeltal) (MIT)
- [pythops/tenere](https://github.com/pythops/tenere) (AGPL)
- [ramalama](https://github.com/containers/ramalama) (MIT)
- [semperai/amica](https://github.com/semperai/amica) (MIT)
- [withcatai/catai](https://github.com/withcatai/catai) (MIT)
- [Autopen](https://github.com/blackhole89/autopen) (GPL)

</details>

<details>
<summary>Tools</summary>

- [akx/ggify](https://github.com/akx/ggify) – download PyTorch models from HuggingFace Hub and convert them to GGML
- [akx/ollama-dl](https://github.com/akx/ollama-dl) – download models from the Ollama library to be used directly with llama.cpp
- [crashr/gppm](https://github.com/crashr/gppm) – launch llama.cpp instances utilizing NVIDIA Tesla P40 or P100 GPUs with reduced idle power consumption
- [gpustack/gguf-parser](https://github.com/gpustack/gguf-parser-go/tree/main/cmd/gguf-parser) - review/check the GGUF file and estimate the memory usage
- [Styled Lines](https://marketplace.unity.com/packages/tools/generative-ai/styled-lines-llama-cpp-model-292902) (proprietary licensed, async wrapper of inference part for game development in Unity3d with pre-built Mobile and Web platform wrappers and a model example)

</details>

<details>
<summary>Infrastructure</summary>

- [Paddler](https://github.com/distantmagic/paddler) - Stateful load balancer custom-tailored for llama.cpp
- [GPUStack](https://github.com/gpustack/gpustack) - Manage GPU clusters for running LLMs
- [llama_cpp_canister](https://github.com/onicai/llama_cpp_canister) - llama.cpp as a smart contract on the Internet Computer, using WebAssembly
- [llama-swap](https://github.com/mostlygeek/llama-swap) - transparent proxy that adds automatic model switching with llama-server
- [Kalavai](https://github.com/kalavai-net/kalavai-client) - Crowdsource end to end LLM deployment at any scale

</details>

<details>
<summary>Games</summary>

- [Lucy's Labyrinth](https://github.com/MorganRO8/Lucys_Labyrinth) - A simple maze game where agents controlled by an AI model will try to trick you.

</details>

## Supported backends
```c
/*
Notes:杨小兵-2025-04-08

1、后端支持列表
Backend	    Target devices	        通俗解释
Metal	    Apple Silicon	        苹果设备的专属翻译官
BLAS/BLIS	All	                    通用的基础翻译官，哪种硬件都能用（计算CPU上矩阵相关计算）
SYCL	    Intel 和 Nvidia GPU	    英特尔和 Nvidia 的双语翻译官
MUSA	    Moore Threads MTT GPU	摩尔线程 GPU 的专属翻译官
CUDA	    Nvidia GPU	            Nvidia GPU 的专属翻译官
HIP	        AMD GPU	                AMD GPU 的专属翻译官
Vulkan	    GPU	                    通用 GPU 的多功能翻译官
CANN	    Ascend NPU	            昇腾 NPU 的专属翻译官
OpenCL	    Adreno GPU	            Adreno GPU 的专属翻译官

2、重点理解
    2.1 其中BLAS、BLIS、SYCL（支持相对较少）、Vulkan都是通用的，对这部分内容着重理解可以实现通用化的部署从而减少部署的困难性。
    2.2 后端存在的意义就像一个“翻译官”，让软件能用统一的代码与各种硬件顺畅沟通，不用为每种硬件单独写程序，使用不同的后端平台指挥不同类型的GPU硬件'干活'。
    2.3 可以尝试使用SYCL和Vulkan来测试部署llama.cpp项目。
*/
```
| Backend | Target devices |
| --- | --- |
| [Metal](docs/build.md#metal-build) | Apple Silicon |
| [BLAS](docs/build.md#blas-build) | All |
| [BLIS](docs/backend/BLIS.md) | All |
| [SYCL](docs/backend/SYCL.md) | Intel and Nvidia GPU |
| [MUSA](docs/build.md#musa) | Moore Threads MTT GPU |
| [CUDA](docs/build.md#cuda) | Nvidia GPU |
| [HIP](docs/build.md#hip) | AMD GPU |
| [Vulkan](docs/build.md#vulkan) | GPU |
| [CANN](docs/build.md#cann) | Ascend NPU |
| [OpenCL](docs/backend/OPENCL.md) | Adreno GPU |

## Building the project

The main product of this project is the `llama` library. Its C-style interface can be found in [include/llama.h](include/llama.h).
The project also includes many example programs and tools using the `llama` library. The examples range from simple, minimal code snippets to sophisticated sub-projects such as an OpenAI-compatible HTTP server. Possible methods for obtaining the binaries:

- Clone this repository and build locally, see [how to build](docs/build.md)
- On MacOS or Linux, install `llama.cpp` via [brew, flox or nix](docs/install.md)
- Use a Docker image, see [documentation for Docker](docs/docker.md)
- Download pre-built binaries from [releases](https://github.com/ggml-org/llama.cpp/releases)

```c
/*
Notes:杨小兵-2025-04-08

1、这部分内容将会解释构建llama.cpp项目的流程。
2、llama.cpp这个项目的主要成果是llama库。llama library的C风格的接口可以在include/llama.h中找到。llama.cpp项目也包含很多使用llama库的示例程序、工具。这些示例包含从简单的、最小的代码片段到复杂的子项目例如OpenAI-compatible HTTP server。获取这些二进制可执行文件有多种方式：
    2.1 克隆和在本地及逆行构建（查看对应的文档来获取本地构建的说明）
    2.2 在MacOS\Linux上可以通过brew\flox\nix来安装llama.cpp。
    2.3 可以使用Docker image（可以通过查看对应文档获取具体的内容）
    2.4 可以从releases中下载pre-built的二进制文件。
*/
```

## Obtaining and quantizing models
```c
/*
Notes:杨小兵-2025-04-09

1、这部分内容介绍关于获取模型、量化模型的一些内容。
2、获取模型
    2.1 可以在hugging face中直接下载对应的GGUF文件
    2.2 使用llama.cpp的时候可以通过对应的命令行参数下载对应的模型文件
*/
```
The [Hugging Face](https://huggingface.co) platform hosts a [number of LLMs](https://huggingface.co/models?library=gguf&sort=trending) compatible with `llama.cpp`:

- [Trending](https://huggingface.co/models?library=gguf&sort=trending)
- [LLaMA](https://huggingface.co/models?sort=trending&search=llama+gguf)

You can either manually download the GGUF file or directly use any `llama.cpp`-compatible models from Hugging Face by using this CLI argument: `-hf <user>/<model>[:quant]`
```c
/*
Notes:杨小兵-2025-04-09

1、Hugging Face 平台拥有许多与 llama.cpp 兼容的 LLM，可以通过trending和llama关键词或者标签对模型类型进行筛选。
2、两种获取模型文件的方式
    2.1 在hugging face平台上直接下载模型文件
    2.2 通过命令行参数进行下载模型文件
*/
```

After downloading a model, use the CLI tools to run it locally - see below.

`llama.cpp` requires the model to be stored in the [GGUF](https://github.com/ggml-org/ggml/blob/master/docs/gguf.md) file format. Models in other data formats can be converted to GGUF using the `convert_*.py` Python scripts in this repo.
```c
/*
Notes:杨小兵-2025-04-09

1、下载完一个模型文件之后，可以CLI工具本地运行模型，详细内容查看下列内容。
2、llama.cpp需要模型以GGUF文件格式保存起来。如果模型文件是其他的数据格式，可以使用convert_*.py脚本文件实现存储格式转化。
*/
```

The Hugging Face platform provides a variety of online tools for converting, quantizing and hosting models with `llama.cpp`:

- Use the [GGUF-my-repo space](https://huggingface.co/spaces/ggml-org/gguf-my-repo) to convert to GGUF format and quantize model weights to smaller sizes
- Use the [GGUF-my-LoRA space](https://huggingface.co/spaces/ggml-org/gguf-my-lora) to convert LoRA adapters to GGUF format (more info: https://github.com/ggml-org/llama.cpp/discussions/10123)
- Use the [GGUF-editor space](https://huggingface.co/spaces/CISCai/gguf-editor) to edit GGUF meta data in the browser (more info: https://github.com/ggml-org/llama.cpp/discussions/9268)
- Use the [Inference Endpoints](https://ui.endpoints.huggingface.co/) to directly host `llama.cpp` in the cloud (more info: https://github.com/ggml-org/llama.cpp/discussions/9669)

To learn more about model quantization, [read this documentation](examples/quantize/README.md)
```c
/*
Notes:杨小兵-2025-04-09

1、Hugging Face 平台提供了多种在线工具，用于使用 llama.cpp 转换、量化和托管模型。
    1.1 GGUF-my-repo space：实现GGUF格式转化、量化模型权重
    1.2 GGUF-my-LoRA space: 将LoRA adapters转化成GGUF格式
    1.3 GGUF-editor space：可以直接在浏览器中编辑GGUF元数据
    1.4 inference endpoints: 可以直接在云端托管llama.cpp
2、如果想要了解更多关于模型量化的内容，查看给出的文档内容。
    2.1 这部分内容对于量化文件参数的理解是有帮助的。
    2.2 可以通过查看更多相关资料来理解的模型量化相关内容。
*/
```

## [`llama-cli`](examples/main)
```c
/*
Notes:杨小兵-2025-04-09

1、在llama.cpp项目中一个子项目为llama-cli（可以通过命令行的方式运行模型。
2、该标题被渲染之后可以通过点击文字直接跳转到对应的目录结构中，这也是markdown格式的一个亮点，在后续的学习、使用中可以使用这里学习到的内容。
*/
```

#### A CLI tool for accessing and experimenting with most of `llama.cpp`'s functionality.

- <details open>
    <summary>Run in conversation mode</summary>

    ```c
    /*
    Notes:杨小兵-2025-04-09

    1、llama-cli以对话的模式运行模型。
    2、具有内置聊天模板的模型将自动激活对话模式。如果没有激活，您可以通过添加 `-cnv` 并使用 `--chat-template NAME` 指定合适的聊天模板来手动启用它。
    3、这里通过details标签来实现一部分内容的折叠，可以在后续的实践中使用这种用法。
    */
    ```
    Models with a built-in chat template will automatically activate conversation mode. If this doesn't occur, you can manually enable it by adding `-cnv` and specifying a suitable chat template with `--chat-template NAME`

    ```bash
    llama-cli -m model.gguf

    # > hi, who are you?
    # Hi there! I'm your helpful assistant! I'm an AI-powered chatbot designed to assist and provide information to users like you. I'm here to help answer your questions, provide guidance, and offer support on a wide range of topics. I'm a friendly and knowledgeable AI, and I'm always happy to help with anything you need. What's on your mind, and how can I assist you today?
    #
    # > what is 1+1?
    # Easy peasy! The answer to 1+1 is... 2!
    ```

    </details>

- <details>
    <summary>Run in conversation mode with custom chat template</summary>

    ```c
    /*
    Notes:杨小兵-2025-04-09

    1、llama-cli以使用自定义对话模版的模式运行模型。
    2、
    */
    ```

    ```bash
    # use the "chatml" template (use -h to see the list of supported templates)
    llama-cli -m model.gguf -cnv --chat-template chatml

    # use a custom template
    llama-cli -m model.gguf -cnv --in-prefix 'User: ' --reverse-prompt 'User:'
    ```

    </details>

- <details>
    <summary>Run simple text completion</summary>

    ```c
    /*
    Notes:杨小兵-2025-04-09

    1、llama-cli以简单文本补全的方式运行模型。
    2、这种方式可以用来简单测试模型，实际使用场景是比较少的。
    */
    ```

    To disable conversation mode explicitly, use `-no-cnv`

    ```bash
    llama-cli -m model.gguf -p "I believe the meaning of life is" -n 128 -no-cnv

    # I believe the meaning of life is to find your own truth and to live in accordance with it. For me, this means being true to myself and following my passions, even if they don't align with societal expectations. I think that's what I love about yoga – it's not just a physical practice, but a spiritual one too. It's about connecting with yourself, listening to your inner voice, and honoring your own unique journey.
    ```

    </details>

- <details>
    <summary>Constrain the output with a custom grammar</summary>

    ```c
    /*
    Notes:杨小兵-2025-04-09

    1、llama-cli以自定义grammar限制模型输出的方式运行模型。
    2、应用场景：结构化的输出。
    3、在grammars目录中包含少量的示例grammars，想要编写自己的grammars可以查看对应的GBNF Guide内容获取更多内容。
    4、这部分内容可以在后续学习中深入了解。
    */
    ```

    ```bash
    llama-cli -m model.gguf -n 256 --grammar-file grammars/json.gbnf -p 'Request: schedule a call at 8pm; Command:'

    # {"appointmentTime": "8pm", "appointmentDetails": "schedule a a call"}
    ```

    The [grammars/](grammars/) folder contains a handful of sample grammars. To write your own, check out the [GBNF Guide](grammars/README.md).

    For authoring more complex JSON grammars, check out https://grammar.intrinsiclabs.ai/

    </details>


## [`llama-server`](examples/server)

#### A lightweight, [OpenAI API](https://github.com/openai/openai-openapi) compatible, HTTP server for serving LLMs.

```c
/*
Notes:杨小兵-2025-04-09

1、特点
    1.1 轻量级
    1.2 OpenAI API兼容的
2、llama-server是llama.cpp项目中的一个子项目，用来为LLMs提供服务。
3、相关的内容可以在后续深入了解，目前只是大致了解llama.cpp整体情况。
*/
```

- <details open>
    <summary>Start a local HTTP server with default configuration on port 8080</summary>

    ```bash
    llama-server -m model.gguf --port 8080

    # Basic web UI can be accessed via browser: http://localhost:8080
    # Chat completion endpoint: http://localhost:8080/v1/chat/completions
    ```

    </details>

- <details>
    <summary>Support multiple-users and parallel decoding</summary>

    ```bash
    # up to 4 concurrent requests, each with 4096 max context
    llama-server -m model.gguf -c 16384 -np 4
    ```

    </details>

- <details>
    <summary>Enable speculative decoding</summary>

    ```bash
    # the draft.gguf model should be a small variant of the target model.gguf
    llama-server -m model.gguf -md draft.gguf
    ```

    </details>

- <details>
    <summary>Serve an embedding model</summary>

    ```bash
    # use the /embedding endpoint
    llama-server -m model.gguf --embedding --pooling cls -ub 8192
    ```

    </details>

- <details>
    <summary>Serve a reranking model</summary>

    ```bash
    # use the /reranking endpoint
    llama-server -m model.gguf --reranking
    ```

    </details>

- <details>
    <summary>Constrain all outputs with a grammar</summary>

    ```bash
    # custom grammar
    llama-server -m model.gguf --grammar-file grammar.gbnf

    # JSON
    llama-server -m model.gguf --grammar-file grammars/json.gbnf
    ```

    </details>


## [`llama-perplexity`](examples/perplexity)

```c
/*
Notes:杨小兵-2025-04-09

1、llama-perplexity是llama.cpp项目中的一个子项目。
2、llama-perplexity这个工具是用来测量一个模型对于一个给定文本的perplexity以及其他的一些metric（目前来说不是特别重要，可以在后续需要的时候查看相应内容）
*/
```

#### A tool for measuring the perplexity [^1][^2] (and other quality metrics) of a model over a given text.

- <details open>
    <summary>Measure the perplexity over a text file</summary>

    ```bash
    llama-perplexity -m model.gguf -f file.txt

    # [1]15.2701,[2]5.4007,[3]5.3073,[4]6.2965,[5]5.8940,[6]5.6096,[7]5.7942,[8]4.9297, ...
    # Final estimate: PPL = 5.4007 +/- 0.67339
    ```

    </details>

- <details>
    <summary>Measure KL divergence</summary>

    ```bash
    # TODO
    ```

    </details>

[^1]: [examples/perplexity/README.md](./examples/perplexity/README.md)
[^2]: [https://huggingface.co/docs/transformers/perplexity](https://huggingface.co/docs/transformers/perplexity)

## [`llama-bench`](examples/llama-bench)

```c
/*
Notes:杨小兵-2025-04-09

1、llama-bench是llama.cpp项目中的一个子项目。
2、llama-bench工具是用来测试模型对于给定不同参数其推理的性能（目前来说不是特别重要，不过可以尝试一下）。
*/
```

#### Benchmark the performance of the inference for various parameters.

- <details open>
    <summary>Run default benchmark</summary>

    ```bash
    llama-bench -m model.gguf

    # Output:
    # | model               |       size |     params | backend    | threads |          test |                  t/s |
    # | ------------------- | ---------: | ---------: | ---------- | ------: | ------------: | -------------------: |
    # | qwen2 1.5B Q4_0     | 885.97 MiB |     1.54 B | Metal,BLAS |      16 |         pp512 |      5765.41 ± 20.55 |
    # | qwen2 1.5B Q4_0     | 885.97 MiB |     1.54 B | Metal,BLAS |      16 |         tg128 |        197.71 ± 0.81 |
    #
    # build: 3e0ba0e60 (4229)
    ```

    </details>

## [`llama-run`](examples/run)

```c
/*
Notes:杨小兵-2025-04-09

1、llama-run是llama.cpp项目中的一个子项目。
2、llama-run是一个运行llama.cpp模型的综合实力程序，对于inference来说是有用的（后续需要的时候再深入了解其内容）。
*/
```

#### A comprehensive example for running `llama.cpp` models. Useful for inferencing. Used with RamaLama [^3].

- <details>
    <summary>Run a model with a specific prompt (by default it's pulled from Ollama registry)</summary>

    ```bash
    llama-run granite-code
    ```

    </details>

[^3]: [RamaLama](https://github.com/containers/ramalama)

## [`llama-simple`](examples/simple)

```c
/*
Notes:杨小兵-2025-04-09

1、llama-simple是llama.cpp项目中的一个子项目。
2、llama-simple是一个使用llama.cpp实现apps的最小的示例程序，对于开发者来说是有用的（需要深入开发的时候再来了解这部分内容）。
*/
```
#### A minimal example for implementing apps with `llama.cpp`. Useful for developers.

- <details>
    <summary>Basic text completion</summary>

    ```bash
    llama-simple -m model.gguf

    # Hello my name is Kaitlyn and I am a 16 year old girl. I am a junior in high school and I am currently taking a class called "The Art of
    ```

    </details>


## Contributing

- Contributors can open PRs
- Collaborators can push to branches in the `llama.cpp` repo and merge PRs into the `master` branch
- Collaborators will be invited based on contributions
- Any help with managing issues, PRs and projects is very appreciated!
- See [good first issues](https://github.com/ggml-org/llama.cpp/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22) for tasks suitable for first contributions
- Read the [CONTRIBUTING.md](CONTRIBUTING.md) for more information
- Make sure to read this: [Inference at the edge](https://github.com/ggml-org/llama.cpp/discussions/205)
- A bit of backstory for those who are interested: [Changelog podcast](https://changelog.com/podcast/532)
```c
/*
Notes:杨小兵-2025-04-09

1、这部分内容介绍contribute相关内容。
2、有意思的是这里放出了llama.cpp项目的一些backstory。
*/
```

## Other documentation

- [main (cli)](examples/main/README.md)
- [server](examples/server/README.md)
- [GBNF grammars](grammars/README.md)

#### Development documentation

- [How to build](docs/build.md)
- [Running on Docker](docs/docker.md)
- [Build on Android](docs/android.md)
- [Performance troubleshooting](docs/development/token_generation_performance_tips.md)
- [GGML tips & tricks](https://github.com/ggml-org/llama.cpp/wiki/GGML-Tips-&-Tricks)

#### Seminal papers and background on the models

If your issue is with model generation quality, then please at least scan the following links and papers to understand the limitations of LLaMA models. This is especially important when choosing an appropriate model size and appreciating both the significant and subtle differences between LLaMA models and ChatGPT:
- LLaMA:
    - [Introducing LLaMA: A foundational, 65-billion-parameter large language model](https://ai.facebook.com/blog/large-language-model-llama-meta-ai/)
    - [LLaMA: Open and Efficient Foundation Language Models](https://arxiv.org/abs/2302.13971)
- GPT-3
    - [Language Models are Few-Shot Learners](https://arxiv.org/abs/2005.14165)
- GPT-3.5 / InstructGPT / ChatGPT:
    - [Aligning language models to follow instructions](https://openai.com/research/instruction-following)
    - [Training language models to follow instructions with human feedback](https://arxiv.org/abs/2203.02155)

## Completions
Command-line completion is available for some environments.

#### Bash Completion
```bash
$ build/bin/llama-cli --completion-bash > ~/.llama-completion.bash
$ source ~/.llama-completion.bash
```
Optionally this can be added to your `.bashrc` or `.bash_profile` to load it
automatically. For example:
```console
$ echo "source ~/.llama-completion.bash" >> ~/.bashrc
```

```c
/*
Notes:杨小兵-2025-04-09

1、上述内容展示了一个 Bash 脚本片段，用于为 llama-cli 命令行工具启用 Bash 自动补全功能。Bash 是一种常见的 Unix shell，支持命令补全功能，用户可以通过按 Tab 键自动补全命令或参数，从而提升操作效率。
*/
```

## References
