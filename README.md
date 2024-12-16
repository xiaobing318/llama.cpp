# llama.cpp

```c
/*
notes:杨小兵-2024-12-16

1、这个项目名称叫做llama.cpp
*/
```

![llama](https://user-images.githubusercontent.com/1991296/230134379-7181e485-c521-4d23-a0d6-f7b3b61ba524.png)
```c
/*
杨小兵-2024-12-16

1、这个markdown语法为了在渲染的时候展示一张项目的logo图片
2、其中[llama]为了给这个图片命名
3、这个logo图片是从互联网中获取得到的
*/
```

[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![Server](https://github.com/ggerganov/llama.cpp/actions/workflows/server.yml/badge.svg)](https://github.com/ggerganov/llama.cpp/actions/workflows/server.yml)
```c
/*
杨小兵-2024-12-16

1、通过点击图片的方式在浏览器中打开对应的网址
2、这里涉及到两个网址：许可证、项目服务器
*/
```


[Roadmap](https://github.com/users/ggerganov/projects/7) / [Project status](https://github.com/ggerganov/llama.cpp/discussions/3471) / [Manifesto](https://github.com/ggerganov/llama.cpp/discussions/205) / [ggml](https://github.com/ggerganov/ggml)
```c
/*
杨小兵-2024-12-16

1、通过点击文字链接的方式跳转到该项目的不同网址查看对应的内容
*/
```

Inference of Meta's [LLaMA](https://arxiv.org/abs/2302.13971) model (and others) in pure C/C++
```c
/*
杨小兵-2024-12-16

1、只使用C/C++对Meta公司的LLaMA模型、其他公司的模型进行inference
2、这里的亮点就是只使用C、C++来完成整个模型的推理工作而不会使用到其他的高级语言，对于不同平台的适配将会变得容易
*/
```

## 1 Recent API changes

- [Changelog for `libllama` API](https://github.com/ggerganov/llama.cpp/issues/9289)
- [Changelog for `llama-server` REST API](https://github.com/ggerganov/llama.cpp/issues/9291)
```c
/*
杨小兵-2024-12-16

1、这部分内容为了说明最近函数API的一些改变情况
2、通过日志的方式来说明libllama接口变化情况
3、通过日志的方式来说明llama-server接口变化情况
*/
```

## 2 Hot topics

- **Introducing GGUF-my-LoRA** https://github.com/ggerganov/llama.cpp/discussions/10123
- Hugging Face Inference Endpoints now support GGUF out of the box! https://github.com/ggerganov/llama.cpp/discussions/9669
- Hugging Face GGUF editor: [discussion](https://github.com/ggerganov/llama.cpp/discussions/9268) | [tool](https://huggingface.co/spaces/CISCai/gguf-editor)
```c
/*
杨小兵-2024-12-16

1、这部分内容为了说明一些热点话题
2、首先通过网址的方式来介绍GGUF-my-LoRA的话题
3、hugging face inference endpoints使得用户可以更方便地在 Hugging Face 上部署使用 GGUF 格式的模型，无需进行格式转换或额外的配置步骤
4、提供了对GGUF文件的编辑功能
*/
```
----

## 3 Description

The main goal of `llama.cpp` is to enable LLM inference with minimal setup and state-of-the-art performance on a wide
range of hardware - locally and in the cloud.
```c
/*
杨小兵-2024-12-16

1、llama.cpp项目的主要目标就是为了使用最小的配置和先进的性能在很多硬件上实现LLM的inference任务
2、可以在本地、云端部署模型
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
杨小兵-2024-12-16

1、没有任何依赖的纯C/C++实现
2、苹果的芯片是被优先考虑的，通过 ARM NEON、Accelerate 和 Metal 框架进行了优化
3、对于x86架构，AVX, AVX2, AVX512 and AMX指令集是被支持的
4、为了更快的推理任务、更好的内存需求1.5-bit, 2-bit, 3-bit, 4-bit, 5-bit, 6-bit, and 8-bit整数量化是被支持的
5、为了在NVIDIA GPUS上运行LLMs，自定义了一些CUDA内核（通过HIP支持AMD GPUs、通过MUSA支持Moore Threads MTT GPUs）
6、支持Vulkan and SYCL backend
7、CPU+GPU 混合推理，部分加速大于 VRAM 总容量的模型
*/
```

The `llama.cpp` project is the main playground for developing new features for the [ggml](https://github.com/ggerganov/ggml) library.
```c
/*
杨小兵-2024-12-16

1、`llama.cpp` 项目是开发 [ggml](https://github.com/ggerganov/ggml) 库新功能的主要平台。
2、llama.cpp项目用到了ggml项目中的东西，llama项目利用ggml项目的计算能力实现对不同的LLMs推理工作
*/
```

<details>
<summary>Models</summary>

Typically finetunes of the base models below are supported as well.

Instructions for adding support for new models: [HOWTO-add-model.md](docs/development/HOWTO-add-model.md)
```c
/*
杨小兵-2024-12-16

1、这里提到的模型也包含一些基础模型的微调版本，这些微调版本的模型也是被llama.cpp项目支持的
2、如果想要在llama.cpp项目中添加对新模型的支持，这里的HOWTO-add-model.md文件提供了参考
*/
```
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
- [X] [BERT](https://github.com/ggerganov/llama.cpp/pull/5423)
- [X] [Koala](https://bair.berkeley.edu/blog/2023/04/03/koala/)
- [X] [Baichuan 1 & 2](https://huggingface.co/models?search=baichuan-inc/Baichuan) + [derivations](https://huggingface.co/hiyouga/baichuan-7b-sft)
- [X] [Aquila 1 & 2](https://huggingface.co/models?search=BAAI/Aquila)
- [X] [Starcoder models](https://github.com/ggerganov/llama.cpp/pull/3187)
- [X] [Refact](https://huggingface.co/smallcloudai/Refact-1_6B-fim)
- [X] [MPT](https://github.com/ggerganov/llama.cpp/pull/3417)
- [X] [Bloom](https://github.com/ggerganov/llama.cpp/pull/3553)
- [x] [Yi models](https://huggingface.co/models?search=01-ai/Yi)
- [X] [StableLM models](https://huggingface.co/stabilityai)
- [x] [Deepseek models](https://huggingface.co/models?search=deepseek-ai/deepseek)
- [x] [Qwen models](https://huggingface.co/models?search=Qwen/Qwen)
- [x] [PLaMo-13B](https://github.com/ggerganov/llama.cpp/pull/3557)
- [x] [Phi models](https://huggingface.co/models?search=microsoft/phi)
- [x] [GPT-2](https://huggingface.co/gpt2)
- [x] [Orion 14B](https://github.com/ggerganov/llama.cpp/pull/5118)
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
- [x] [ChatGLM3-6b](https://huggingface.co/THUDM/chatglm3-6b) + [ChatGLM4-9b](https://huggingface.co/THUDM/glm-4-9b)
- [x] [SmolLM](https://huggingface.co/collections/HuggingFaceTB/smollm-6695016cad7167254ce15966)
- [x] [EXAONE-3.0-7.8B-Instruct](https://huggingface.co/LGAI-EXAONE/EXAONE-3.0-7.8B-Instruct)
- [x] [FalconMamba Models](https://huggingface.co/collections/tiiuae/falconmamba-7b-66b9a580324dd1598b0f6d4a)
- [x] [Jais](https://huggingface.co/inceptionai/jais-13b-chat)
- [x] [Bielik-11B-v2.3](https://huggingface.co/collections/speakleash/bielik-11b-v23-66ee813238d9b526a072408a)
- [x] [RWKV-6](https://github.com/BlinkDL/RWKV-LM)
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
- C#/.NET: [SciSharp/LLamaSharp](https://github.com/SciSharp/LLamaSharp)
- C#/VB.NET (more features - community license): [LM-Kit.NET](https://docs.lm-kit.com/lm-kit-net/index.html)
- Scala 3: [donderom/llm4s](https://github.com/donderom/llm4s)
- Clojure: [phronmophobic/llama.clj](https://github.com/phronmophobic/llama.clj)
- React Native: [mybigday/llama.rn](https://github.com/mybigday/llama.rn)
- Java: [kherud/java-llama.cpp](https://github.com/kherud/java-llama.cpp)
- Zig: [deins/llama.cpp.zig](https://github.com/Deins/llama.cpp.zig)
- Flutter/Dart: [netdur/llama_cpp_dart](https://github.com/netdur/llama_cpp_dart)
- Flutter: [xuegao-tzx/Fllama](https://github.com/xuegao-tzx/Fllama)
- PHP (API bindings and features built on top of llama.cpp): [distantmagic/resonance](https://github.com/distantmagic/resonance) [(more info)](https://github.com/ggerganov/llama.cpp/pull/6326)
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

</details>

<details>
<summary>Games</summary>

- [Lucy's Labyrinth](https://github.com/MorganRO8/Lucys_Labyrinth) - A simple maze game where agents controlled by an AI model will try to trick you.

</details>

## 4 Supported backends

| Backend | Target devices |
| --- | --- |
| [Metal](docs/build.md#metal-build) | Apple Silicon |
| [BLAS](docs/build.md#blas-build) | All |
| [BLIS](docs/backend/BLIS.md) | All |
| [SYCL](docs/backend/SYCL.md) | Intel and Nvidia GPU |
| [MUSA](docs/build.md#musa) | Moore Threads MTT GPU |
| [CUDA](docs/build.md#cuda) | Nvidia GPU |
| [hipBLAS](docs/build.md#hipblas) | AMD GPU |
| [Vulkan](docs/build.md#vulkan) | GPU |
| [CANN](docs/build.md#cann) | Ascend NPU |

## 5 Building the project

The main product of this project is the `llama` library. Its C-style interface can be found in [include/llama.h](include/llama.h).
The project also includes many example programs and tools using the `llama` library. The examples range from simple, minimal code snippets to sophisticated sub-projects such as an OpenAI-compatible HTTP server. Possible methods for obtaining the binaries:
```c
/*
杨小兵-2024-12-16

1、这部分内容讲述的是如何构建llama.cpp项目
2、这个项目的主要产品成果就是llama库。这个项目的C风格接口可以在include/llama.h中找到
3、这个项目同时也包含很多使用llama库的示例程序和工具
4、这些示例程序从简单的、最小化的代码片段到复杂的子项目例如兼容OpenAI的HTTP服务器。
5、可以通过下列内容获取这些二进制的制作方式
*/
```

- Clone this repository and build locally, see [how to build](docs/build.md)
- On MacOS or Linux, install `llama.cpp` via [brew, flox or nix](docs/install.md)
- Use a Docker image, see [documentation for Docker](docs/docker.md)
- Download pre-built binaries from [releases](https://github.com/ggerganov/llama.cpp/releases)
```c
/*
杨小兵-2024-12-16

1、将这个仓库clone到本地并且在本地进行构建，查看docs/build.md文档获取帮助
2、在MacOS或者Linux上可以通过brew, flox or nix来对llama.cpp项目成果进行安装
3、如果想要使用Docker镜像，那么可以查看docs/docker.md获取帮助
4、可以从https://github.com/ggerganov/llama.cpp/releases直接下载已经构建好的二进制文件
*/
```

## 6 Obtaining and quantizing models

The [Hugging Face](https://huggingface.co) platform hosts a [number of LLMs](https://huggingface.co/models?library=gguf&sort=trending) compatible with `llama.cpp`:

- [Trending](https://huggingface.co/models?library=gguf&sort=trending)
- [LLaMA](https://huggingface.co/models?sort=trending&search=llama+gguf)
```c
/*
杨小兵-2024-12-16

1、获取模型、量化模型
2、Hugging Face 平台拥有许多与 llama.cpp 兼容的LLMs
3、上述的两个链接为了在hugging face上查找GGUF相关的LLMs
*/
```
After downloading a model, use the CLI tools to run it locally - see below.

`llama.cpp` requires the model to be stored in the [GGUF](https://github.com/ggerganov/ggml/blob/master/docs/gguf.md) file format. Models in other data formats can be converted to GGUF using the `convert_*.py` Python scripts in this repo.
```c
/*
杨小兵-2024-12-16

1、将模型加载之后使用CLI工具在本地运行模型
2、llama.cpp项目要求模型以GGUF文件格式进行保存，以其他数据格式保存的模型可以使用该仓库中的convert_*.py的python脚本进行转化
*/
```

The Hugging Face platform provides a variety of online tools for converting, quantizing and hosting models with `llama.cpp`:
```c
/*
杨小兵-2024-12-16

1、Hugging Face平台提供了多种在线工具，用于使用`llama.cpp`转换，量化和托管模型
*/
```

- Use the [GGUF-my-repo space](https://huggingface.co/spaces/ggml-org/gguf-my-repo) to convert to GGUF format and quantize model weights to smaller sizes
- Use the [GGUF-my-LoRA space](https://huggingface.co/spaces/ggml-org/gguf-my-lora) to convert LoRA adapters to GGUF format (more info: https://github.com/ggerganov/llama.cpp/discussions/10123)
- Use the [GGUF-editor space](https://huggingface.co/spaces/CISCai/gguf-editor) to edit GGUF meta data in the browser (more info: https://github.com/ggerganov/llama.cpp/discussions/9268)
- Use the [Inference Endpoints](https://ui.endpoints.huggingface.co/) to directly host `llama.cpp` in the cloud (more info: https://github.com/ggerganov/llama.cpp/discussions/9669)
```c
/*
杨小兵-2024-12-16

1、使用GGUF-my-repo space对模型格式进行转化并且对模型进行量化从而减少模型存储大小
2、使用GGUF-my-LoRA space将LoRA adapters转化为GGUF格式，可以通过https://github.com/ggerganov/llama.cpp/discussions/10123查看更多的信息
3、使用GGUF-editor space在浏览器中来对GGUF文件的meta数据进行编辑
4、使用Inference Endpoints直接在云中部署llama.cpp项目
*/
```

To learn more about model quantization, [read this documentation](examples/quantize/README.md)
```c
/*
杨小兵-2024-12-16

1、查看examples/quantize/README.md文件来获取关于模型量化的更多内容
*/
```

## 7 [`llama-cli`](examples/main)

#### A CLI tool for accessing and experimenting with most of `llama.cpp`'s functionality.
```c
/*
杨小兵-2024-12-16

1、用于访问和试验“llama.cpp”大部分功能的 CLI 工具
2、llama-cli用来测试和实验llama.cpp项目的大部分功能
*/
```

- <details open>
    <summary>Run simple text completion</summary>

    ```bash
    llama-cli -m model.gguf -p "I believe the meaning of life is" -n 128

    # I believe the meaning of life is to find your own truth and to live in accordance with it. For me, this means being true to myself and following my passions, even if they don't align with societal expectations. I think that's what I love about yoga – it's not just a physical practice, but a spiritual one too. It's about connecting with yourself, listening to your inner voice, and honoring your own unique journey.
    ```

    </details>

- <details>
    <summary>Run in conversation mode</summary>

    ```bash
    llama-cli -m model.gguf -p "You are a helpful assistant" -cnv

    # > hi, who are you?
    # Hi there! I'm your helpful assistant! I'm an AI-powered chatbot designed to assist and provide information to users like you. I'm here to help answer your questions, provide guidance, and offer support on a wide range of topics. I'm a friendly and knowledgeable AI, and I'm always happy to help with anything you need. What's on your mind, and how can I assist you today?
    #
    # > what is 1+1?
    # Easy peasy! The answer to 1+1 is... 2!
    ```

    </details>

- <details>
    <summary>Run with custom chat template</summary>

    ```bash
    # use the "chatml" template
    llama-cli -m model.gguf -p "You are a helpful assistant" -cnv --chat-template chatml

    # use a custom template
    llama-cli -m model.gguf -p "You are a helpful assistant" -cnv --in-prefix 'User: ' --reverse-prompt 'User:'
    ```

    [Supported templates](https://github.com/ggerganov/llama.cpp/wiki/Templates-supported-by-llama_chat_apply_template)

    </details>

- <details>
    <summary>Constrain the output with a custom grammar</summary>

    ```bash
    llama-cli -m model.gguf -n 256 --grammar-file grammars/json.gbnf -p 'Request: schedule a call at 8pm; Command:'

    # {"appointmentTime": "8pm", "appointmentDetails": "schedule a a call"}
    ```

    The [grammars/](grammars/) folder contains a handful of sample grammars. To write your own, check out the [GBNF Guide](grammars/README.md).

    For authoring more complex JSON grammars, check out https://grammar.intrinsiclabs.ai/

    </details>


## 8 [`llama-server`](examples/server)
```c
/*
杨小兵-2024-12-16

1、这个部分内容用来描述这个项目中的一个llama-server工具
*/
```
#### A lightweight, [OpenAI API](https://github.com/openai/openai-openapi) compatible, HTTP server for serving LLMs.
```c
/*
杨小兵-2024-12-16

1、一个轻量级的、openai api兼容的、为了运行LLMs的HTTP服务
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
    ```c
    /*
    杨小兵-2024-12-16

    1、`-m model.gguf` 指定了主要使用的模型文件为 `model.gguf`
    2、`-md draft.gguf` 指定了一个辅助模型文件 `draft.gguf`，用于推测性解码
    3、`draft.gguf` 模型应该是目标模型 `model.gguf` 的一个小型变体。这意味着 `draft.gguf` 是一个轻量级或精简版的模型，用于辅助或优化目标模型的运行
    */
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


## 9 [`llama-perplexity`](examples/perplexity)

#### A tool for measuring the perplexity [^1][^2] (and other quality metrics) of a model over a given text.
```c
/*
杨小兵-2024-12-16

1、llama-perplexity是一个用来测量
*/
```
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

[^1]: [examples/perplexity/README.md](examples/perplexity/README.md)
[^2]: [https://huggingface.co/docs/transformers/perplexity](https://huggingface.co/docs/transformers/perplexity)

## 10 [`llama-bench`](example/bench)

#### Benchmark the performance of the inference for various parameters.
```c
/*
杨小兵-2024-12-16

1、对于不同的参数用来测量inference的性能
*/
```

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

## 11 [`llama-run`](examples/run)

#### A comprehensive example for running `llama.cpp` models. Useful for inferencing. Used with RamaLama [^3].
```c
/*
杨小兵-2024-12-16

1、运行“llama.cpp”模型的综合示例，适用于推理，与 RamaLama 一起使用
*/
```
- <details>
    <summary>Run a model with a specific prompt (by default it's pulled from Ollama registry)</summary>

    ```bash
    llama-run granite-code
    ```

    </details>

[^3]: [https://github.com/containers/ramalama](RamaLama)

## 12 [`llama-simple`](examples/simple)

#### A minimal example for implementing apps with `llama.cpp`. Useful for developers.
```c
/*
杨小兵-2024-12-16

1、使用llama.cpp项目实现应用程序的一个最小的例子
2、这个例子对开发者很容易
*/
```
- <details>
    <summary>Basic text completion</summary>

    ```bash
    llama-simple -m model.gguf

    # Hello my name is Kaitlyn and I am a 16 year old girl. I am a junior in high school and I am currently taking a class called "The Art of
    ```

    </details>


## 13 Contributing

- Contributors can open PRs
- Collaborators can push to branches in the `llama.cpp` repo and merge PRs into the `master` branch
- Collaborators will be invited based on contributions
- Any help with managing issues, PRs and projects is very appreciated!
- See [good first issues](https://github.com/ggerganov/llama.cpp/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22) for tasks suitable for first contributions
- Read the [CONTRIBUTING.md](CONTRIBUTING.md) for more information
- Make sure to read this: [Inference at the edge](https://github.com/ggerganov/llama.cpp/discussions/205)
- A bit of backstory for those who are interested: [Changelog podcast](https://changelog.com/podcast/532)
```c
/*
杨小兵-2024-12-16

1、贡献者可以开一个PRs
2、协作者可以将branches推送到llama.cpp项目仓库中，并且可以将PRs合并到master分支中
3、对于管理问题、PRs、项目的任何帮助将会十分感谢
4、请参阅[good first issues]以了解适合首次贡献的任务
5、读取CONTRIBUTING.md文件内容获取更多的信息
6、确保将[Inference at the edge]内容了解了
7、给那些感兴趣的人讲一些背景故事
*/
```
## 14 Other documentation

- [main (cli)](examples/main/README.md)
- [server](examples/server/README.md)
- [GBNF grammars](grammars/README.md)
```c
/*
杨小兵-2024-12-16

1、上述三个文件就是对应程序的README.md文件
*/
```
#### Development documentation

- [How to build](docs/build.md)
- [Running on Docker](docs/docker.md)
- [Build on Android](docs/android.md)
- [Performance troubleshooting](docs/development/token_generation_performance_tips.md)
- [GGML tips & tricks](https://github.com/ggerganov/llama.cpp/wiki/GGML-Tips-&-Tricks)

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
```c
/*
杨小兵-2024-12-16

1、开创性的论文和模型背景
2、如果您的问题与模型生成质量有关，请至少浏览以下链接和论文，以了解 LLaMA 模型的局限性。在选择合适的模型大小并理解 LLaMA 模型与 ChatGPT 之间的显著和细微差异时，这一点尤为重要
*/
```
#### References

