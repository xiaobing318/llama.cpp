# llama.cpp/example/simple
```c
/*
notes:杨小兵-2024-12-20

1、这是llama.cpp项目中的一个实例程序
2、我想要通过这个实例程序来开始对llama.cpp项目的探索
*/
```

The purpose of this example is to demonstrate a minimal usage of llama.cpp for generating text with a given prompt.
```c
/*
notes:杨小兵-2024-12-20

1、此示例的目的是演示如何使用 llama.cpp 根据给定的提示生成文本
2、这是对llama.cpp项目使用的一个最小示例
*/
```

```bash
./llama-simple -m ./models/llama-7b-v2/ggml-model-f16.gguf "Hello my name is"

...

main: n_len = 32, n_ctx = 2048, n_parallel = 1, n_kv_req = 32

 Hello my name is Shawn and I'm a 20 year old male from the United States. I'm a 20 year old

main: decoded 27 tokens in 2.31 s, speed: 11.68 t/s

llama_print_timings:        load time =   579.15 ms
llama_print_timings:      sample time =     0.72 ms /    28 runs   (    0.03 ms per token, 38888.89 tokens per second)
llama_print_timings: prompt eval time =   655.63 ms /    10 tokens (   65.56 ms per token,    15.25 tokens per second)
llama_print_timings:        eval time =  2180.97 ms /    27 runs   (   80.78 ms per token,    12.38 tokens per second)
llama_print_timings:       total time =  2891.13 ms
```
```c
/*
notes:杨小兵-2024-12-20

1、上述给出了使用llama-simple示例程序的简单示例
*/
```
