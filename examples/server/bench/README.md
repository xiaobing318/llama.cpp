### 1 Server benchmark tools

Benchmark is using [k6](https://k6.io/).
```c
/*
Notes:杨小兵-2025-02-17

1、在对llama-server进行性能基准测试时，所使用的工具是 [k6](https://k6.io/)。也就是说，在本项目中，为了评估llama-server在不同负载或条件下的表现，会借助 k6 这个专门用于负载测试和性能测试的工具，从而对llama-server性能进行测量和比较。
2、可以借助这个实际的例子了解具体的benchmark具体指的是什么。
*/
```

#### 1.1 Install k6 and sse extension

SSE is not supported by default in k6, you have to build k6 with the [xk6-sse](https://github.com/phymbert/xk6-sse) extension.

Example (assuming golang >= 1.21 is installed):
```shell
go install go.k6.io/xk6/cmd/xk6@latest
$GOPATH/bin/xk6 build master \
--with github.com/phymbert/xk6-sse
```
```c
/*
Notes:杨小兵-2025-02-17

1、安装两部分内容
    1.1 k6
    1.2 sse extension
2、k6性能测试工具默认是不会支持SSE扩展的，你必须要使用xk6-sse扩展来构建k6。
3、上述给出一个使用golang编译器来构建k6。
    3.1 上述给出的脚本步骤是用来构建k6，目前同benchmark是没有关系的。
*/
```

#### 1.2 Download a dataset

This dataset was originally proposed in [vLLM benchmarks](https://github.com/vllm-project/vllm/blob/main/benchmarks/README.md).

```shell
wget https://huggingface.co/datasets/anon8231489123/ShareGPT_Vicuna_unfiltered/resolve/main/ShareGPT_V3_unfiltered_cleaned_split.json
```
```c
/*
Notes:杨小兵-2025-02-17

1、这部分内容介绍的是下载一个数据源。该数据源最初是在vLLM benchmarks中提议的。
2、使用wget工具用来下载对应的数据集。
*/
```

#### 1.3 Download a model
Example for PHI-2

```shell
../../../scripts/hf.sh --repo ggml-org/models --file phi-2/ggml-model-q4_0.gguf
```
```c
/*
Notes:杨小兵-2025-02-17

1、这部分内容介绍的是下载一个模型。
2、上述给出的示例是下载PHI-2模型示例。
*/
```

#### 1.4 Start the server
The server must answer OAI Chat completion requests on `http://localhost:8080/v1` or according to the environment variable `SERVER_BENCH_URL`.

Example:
```shell
llama-server --host localhost --port 8080 \
  --model ggml-model-q4_0.gguf \
  --cont-batching \
  --metrics \
  --parallel 8 \
  --batch-size 512 \
  --ctx-size 4096 \
  -ngl 33
```
```c
/*
Notes:杨小兵-2025-02-18

1、这部分内容解释的是如何使用llama-server可执行文件。
*/
```

#### 1.5 Run the benchmark

For 500 chat completions request with 8 concurrent users during maximum 10 minutes, run:
```shell
./k6 run script.js --duration 10m --iterations 500 --vus 8
```
```c
/*
Notes:杨小兵-2025-02-18

1、测试的示例为：
    1.1 500次对话补全的请求
    1.2 8个并行
    1.3 设置的总时间一共为8分钟
*/
```

The benchmark values can be overridden with:
- `SERVER_BENCH_URL` server url prefix for chat completions, default `http://localhost:8080/v1`
- `SERVER_BENCH_N_PROMPTS` total prompts to randomly select in the benchmark, default `480`
- `SERVER_BENCH_MODEL_ALIAS` model alias to pass in the completion request, default `my-model`
- `SERVER_BENCH_MAX_TOKENS` max tokens to predict, default: `512`
- `SERVER_BENCH_DATASET` path to the benchmark dataset file
- `SERVER_BENCH_MAX_PROMPT_TOKENS` maximum prompt tokens to filter out in the dataset: default `1024`
- `SERVER_BENCH_MAX_CONTEXT` maximum context size of the completions request to filter out in the dataset: prompt + predicted tokens, default `2048`
```c
/*
Notes:杨小兵-2025-02-18

1、可以通过上述提到的环境变量来设置benchmark的值。
*/
```

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
