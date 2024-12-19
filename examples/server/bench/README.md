### 1 Server benchmark tools

Benchmark is using [k6](https://k6.io/).
```c
/*
notes:杨小兵-2024-12-19

1、对llama-server基准测试的tools
2、benchmark使用的是k6（目前对k6不熟悉）
*/
```

#### 1.1 Install k6 and sse extension

SSE is not supported by default in k6, you have to build k6 with the [xk6-sse](https://github.com/phymbert/xk6-sse) extension.
```c
/*
notes:杨小兵-2024-12-19

1、k6 默认不支持 SSE，你必须使用 [xk6-sse]扩展来构建 k6。
2、使用xk6-sse来构建k6从而获取SSE的支持
*/
```

Example:
```shell
go install go.k6.io/xk6/cmd/xk6@latest
xk6 build master \
--with github.com/phymbert/xk6-sse
```

#### 1.2 Download a dataset

This dataset was originally proposed in [vLLM benchmarks](https://github.com/vllm-project/vllm/blob/main/benchmarks/README.md).
```c
/*
notes:杨小兵-2024-12-19

1、下列命令行中的数据集最初是在[vLLM benchmarks]中被提出的
2、这个benchmark使用到了已有的数据集
*/
```

```shell
wget https://huggingface.co/datasets/anon8231489123/ShareGPT_Vicuna_unfiltered/resolve/main/ShareGPT_V3_unfiltered_cleaned_split.json
```

#### 1.3 Download a model
Example for PHI-2
```c
/*
notes:杨小兵-2024-12-19

1、这里下载PHI-2作为示例
2、下载模型文件格式是GGUF
*/
```

```shell
../../../scripts/hf.sh --repo ggml-org/models --file phi-2/ggml-model-q4_0.gguf
```

#### 1.4 Start the server
The server must answer OAI Chat completion requests on `http://localhost:8080/v1` or according to the environment variable `SERVER_BENCH_URL`.
```c
/*
notes:杨小兵-2024-12-19

1、llama-server服务器必须回答`http://localhost:8080/v1`上或根据环境变量`SERVER_BENCH_URL`的OAI聊天完成请求
2、上述这段话是被翻译过来的，理解不到位
*/
```

Example:
```shell
server --host localhost --port 8080 \
  --model ggml-model-q4_0.gguf \
  --cont-batching \
  --metrics \
  --parallel 8 \
  --batch-size 512 \
  --ctx-size 4096 \
  -ngl 33
```

#### 1.5 Run the benchmark
```c
/*
notes:杨小兵-2024-12-19

1、这部分内容介绍的内容就是运行benchmark
2、具体的内容目前先不用关心
*/
```
For 500 chat completions request with 8 concurrent users during maximum 10 minutes, run:
```shell
./k6 run script.js --duration 10m --iterations 500 --vus 8
```

The benchmark values can be overridden with:
- `SERVER_BENCH_URL` server url prefix for chat completions, default `http://localhost:8080/v1`
- `SERVER_BENCH_N_PROMPTS` total prompts to randomly select in the benchmark, default `480`
- `SERVER_BENCH_MODEL_ALIAS` model alias to pass in the completion request, default `my-model`
- `SERVER_BENCH_MAX_TOKENS` max tokens to predict, default: `512`
- `SERVER_BENCH_DATASET` path to the benchmark dataset file
- `SERVER_BENCH_MAX_PROMPT_TOKENS` maximum prompt tokens to filter out in the dataset: default `1024`
- `SERVER_BENCH_MAX_CONTEXT` maximum context size of the completions request to filter out in the dataset: prompt + predicted tokens, default `2048`

Note: the local tokenizer is just a string space split, real number of tokens will differ.

Or with [k6 options](https://k6.io/docs/using-k6/k6-options/reference/):

```shell
SERVER_BENCH_N_PROMPTS=500 k6 run script.js --duration 10m --iterations 500 --vus 8
```

To [debug http request](https://k6.io/docs/using-k6/http-debugging/) use `--http-debug="full"`.

#### 1.6 Metrics

Following metrics are available computed from the OAI chat completions response `usage`:
- `llamacpp_tokens_second` Trend of `usage.total_tokens / request duration`
- `llamacpp_prompt_tokens` Trend of `usage.prompt_tokens`
- `llamacpp_prompt_tokens_total_counter` Counter of `usage.prompt_tokens`
- `llamacpp_completion_tokens` Trend of `usage.completion_tokens`
- `llamacpp_completion_tokens_total_counter` Counter of `usage.completion_tokens`
- `llamacpp_completions_truncated_rate` Rate of completions truncated, i.e. if `finish_reason === 'length'`
- `llamacpp_completions_stop_rate` Rate of completions stopped by the model, i.e. if `finish_reason === 'stop'`

The script will fail if too many completions are truncated, see `llamacpp_completions_truncated_rate`.

K6 metrics might be compared against [server metrics](../README.md), with:

```shell
curl http://localhost:8080/metrics
```

### 2 Using the CI python script
The `bench.py` script does several steps:
- start the server
- define good variable for k6
- run k6 script
- extract metrics from prometheus

It aims to be used in the CI, but you can run it manually:

```shell
LLAMA_SERVER_BIN_PATH=../../../cmake-build-release/bin/llama-server python bench.py \
              --runner-label local \
              --name local \
              --branch `git rev-parse --abbrev-ref HEAD` \
              --commit `git rev-parse HEAD` \
              --scenario script.js \
              --duration 5m \
              --hf-repo ggml-org/models	 \
              --hf-file phi-2/ggml-model-q4_0.gguf \
              --model-path-prefix models \
              --parallel 4 \
              -ngl 33 \
              --batch-size 2048 \
              --ubatch-size	256 \
              --ctx-size 4096 \
              --n-prompts 200 \
              --max-prompt-tokens 256 \
              --max-tokens 256
```
