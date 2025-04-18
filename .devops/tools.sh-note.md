*2025-04-18-杨小兵*

1. **整体作用**
   这个脚本充当 llama.cpp 工具链的“万能入口”，根据你传入的第一个参数 (`--convert/-c`、`--quantize/-q`、`--run/-r`、`--bench/-b`、`--perplexity/-p`、`--all-in-one/-a`、`--server/-s`) 依次调用对应的可执行程序或 Python 脚本：
   - `--convert` 调用 `convert_hf_to_gguf.py`，把 Hugging Face/PyTorch 格式模型转成 GGUF/ggml 格式
   - `--quantize` 调用 `llama-quantize`，对 ggml 二进制模型做量化
   - `--run` 调用 `llama-cli`，用已转换/量化好的模型做推理
   - `--bench` 调用 `llama-bench`，对推理性能做基准测试
   - `--perplexity` 调用 `llama-perplexity`，计算模型在给定文本上的困惑度
   - `--all-in-one` 先按路径批量转换 f16 模型到 q4_0（如果已经存在则跳过），再量化
   - `--server` 调用 `llama-server`，在本地以服务（API）方式运行模型
   - 如果第一个参数不识别，就会打印用法说明。

2. **使用示例**
   假设脚本名为 `run.sh`，且你在项目根目录下：

   ```bash
   # 1) 模型格式转换：把 Hugging Face 目录 /models/7B/ 下的权重转换成 f16 格式的 gguf
   ./run.sh --convert /models/7B/ --outtype f16

   # 2) 模型量化：把已经生成的 f16 模型量化为 q4_0
   ./run.sh -q /models/7B/ggml-model-f16.bin /models/7B/ggml-model-q4_0.bin 2

   # 3) 交互式推理：用一个 q4_0 模型生成 128 个 token
   ./run.sh -r -m /models/7B/ggml-model-q4_0.bin -p "你好，世界！" -n 128

   # 4) 基准测试：测量 /models/7B/ggml-model-f16.bin 的推理速度
   ./run.sh --bench -m /models/7B/ggml-model-f16.bin

   # 5) 困惑度测试：计算模型在给定文本文件上的 perplexity
   ./run.sh -p -m /models/7B/ggml-model-f16.bin -f test.txt

   # 6) 一键全流程：先转换再量化（假设/models/ 下有 7B/ggml-model-f16.bin）
   ./run.sh --all-in-one /models 7B

   # 7) 服务模式：在本地启动一个 RESTful API，监听 8080 端口
   ./run.sh -s -m /models/7B/ggml-model-q4_0.bin --port 8080
   ```

   这样，你就可以用一个统一的脚本管理模型转换、量化、推理、基准测试和部署了。
