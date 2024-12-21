#pragma region "包含头文件（减少重复）"
/*
Notes:杨小兵-2024-12-21
*/
// llama.h提供了C接口，可以在C++代码中使用
#include "llama.h"
// cstdio.h提供了C标准输入输出库
#include <cstdio>
// cstring.h提供了C字符串库
#include <cstring>
// string提供了C++字符串库
#include <string>
// vector提供了C++向量库
#include <vector>
#pragma endregion

/*
Notes:杨小兵-2024-12-21
一、print_usage功能解释
    1. 打印llama-simple使用方法
    2. 输入参数：
        1. argc：参数个数
        2. argv：参数列表
    3. 输出参数：无
二、static关键词功能解释说明
    1. static关键词修饰的函数只能在当前文件中使用
    2. static关键词修饰的全局变量只能在当前文件中使用
*/
static void print_usage(int, char ** argv) {
    printf("\nexample usage:\n");
    printf("\n    %s -m model.gguf [-n n_predict] [-ngl n_gpu_layers] [prompt]\n", argv[0]);
    printf("\n");
}

/*
Notes:杨小兵-2024-12-21
一、main功能解释
    1. 主函数
    2. 输入参数：
        1. argc：参数个数
        2. argv：参数列表
    3. 输出参数：返回值
二、main函数流程
    1. 初始化变量
    2. 加载动态后端
    3. 初始化模型
    4. 分词
    5. 初始化上下文
    6. 初始化采样器
    7. 打印提示
    8. 准备批次
    9. 主循环
    10. 打印
    11. 打印性能
    12. 释放资源
    13. 返回
*/
int main(int argc, char ** argv) {
    // path to the model gguf file
    std::string model_path;
    // prompt to generate text from
    std::string prompt = "Hello my name is";
    // number of layers to offload to the GPU
    int ngl = 99;
    // number of tokens to predict
    int n_predict = 32;

    // parse command line arguments

    {
        int i = 1;
        for (; i < argc; i++) {
            /*
            Notes:杨小兵-2024-12-21
            一、strcmp函数原型解释
                1. 函数原型：int strcmp(const char * str1, const char * str2)
                2. 功能：比较两个字符串
                3. 参数：
                    1. str1：字符串1
                    2. str2：字符串2
                4. 返回值：
                    1. 0：str1等于str2
                    2. 正数：str1大于str2
                    3. 负数：str1小于str2
            二、strcmp函数示例解释说明返回值的三种情况
                1. strcmp("abc", "abc")：返回0
                2. strcmp("abc", "def")：返回负数
                3. strcmp("def", "abc")：返回正数
            */
            if (strcmp(argv[i], "-m") == 0) {
                // check if the next argument exists
                if (i + 1 < argc) {
                    // set the model path
                    model_path = argv[++i];
                } else {
                    //  print usage and return 1
                    print_usage(argc, argv);
                    return 1;
                }
            } else if (strcmp(argv[i], "-n") == 0) {
                // check if the next argument exists
                if (i + 1 < argc) {
                    try {
                        // set the number of tokens to predict
                        n_predict = std::stoi(argv[++i]);
                    } catch (...) {
                        //  print usage and return 1
                        print_usage(argc, argv);
                        return 1;
                    }
                } else {
                    //  print usage and return 1
                    print_usage(argc, argv);
                    return 1;
                }
            } else if (strcmp(argv[i], "-ngl") == 0) {
                // check if the next argument exists
                if (i + 1 < argc) {
                    try {
                        // set the number of layers to offload to the GPU
                        ngl = std::stoi(argv[++i]);
                    } catch (...) {
                        //  print usage and return 1
                        print_usage(argc, argv);
                        return 1;
                    }
                } else {
                    //  print usage and return 1
                    print_usage(argc, argv);
                    return 1;
                }
            } else {
                // prompt starts here
                break;
            }
        }
        //  检查模型路径是否为空
        if (model_path.empty()) {
            //  如果模型路径为空，则打印使用方法并返回1
            print_usage(argc, argv);
            return 1;
        }
        //  检查是否还有参数
        if (i < argc) {
            //  如果还有参数，则将参数拼接到prompt中
            prompt = argv[i++];
            for (; i < argc; i++) {
                //  拼接空格和参数
                prompt += " ";
                prompt += argv[i];
            }
        }
    }

    // load dynamic backends(加载动态后端)
    ggml_backend_load_all();
    // initialize the model(初始化模型参数)
    llama_model_params model_params = llama_model_default_params();
    //  设置GPU层数
    model_params.n_gpu_layers = ngl;
    //  从文件加载模型
    llama_model * model = llama_load_model_from_file(model_path.c_str(), model_params);
    //  检查模型是否为空
    if (model == NULL) {
        //  如果模型为空，则打印错误信息并返回1
        fprintf(stderr , "%s: error: unable to load model\n" , __func__);
        return 1;
    }

    // tokenize the prompt

    // find the number of tokens in the prompt
    const int n_prompt = -llama_tokenize(model, prompt.c_str(), prompt.size(), NULL, 0, true, true);

    // allocate space for the tokens and tokenize the prompt(为tokens分配空间并对提示进行标记)
    std::vector<llama_token> prompt_tokens(n_prompt);
    //  对提示进行标记，返回值小于0表示错误
    if (llama_tokenize(model, prompt.c_str(), prompt.size(), prompt_tokens.data(), prompt_tokens.size(), true, true) < 0) {
        //  如果返回值小于0，则打印错误信息并返回1
        fprintf(stderr, "%s: error: failed to tokenize the prompt\n", __func__);
        return 1;
    }

    // initialize the context(初始化上下文)
    llama_context_params ctx_params = llama_context_default_params();
    // n_ctx is the context size(上下文大小)
    ctx_params.n_ctx = n_prompt + n_predict - 1;
    // n_batch is the maximum number of tokens that can be processed in a single call to llama_decode
    ctx_params.n_batch = n_prompt;
    // enable performance counters(启用性能计数器)
    ctx_params.no_perf = false;
    //  创建上下文
    llama_context * ctx = llama_new_context_with_model(model, ctx_params);
    //  检查上下文是否为空
    if (ctx == NULL) {
        //  如果上下文为空，则打印错误信息并返回1
        fprintf(stderr , "%s: error: failed to create the llama_context\n" , __func__);
        return 1;
    }

    // initialize the sampler(初始化采样器)
    auto sparams = llama_sampler_chain_default_params();
    // enable performance counters(启用性能计数器)
    sparams.no_perf = false;
    //  创建采样器
    llama_sampler * smpl = llama_sampler_chain_init(sparams);
    //  添加采样器
    llama_sampler_chain_add(smpl, llama_sampler_init_greedy());

    // print the prompt token-by-token(逐个打印提示)
    for (auto id : prompt_tokens) {
        // convert token to piece(将token转换为piece)
        char buf[128];
        int n = llama_token_to_piece(model, id, buf, sizeof(buf), 0, true);
        //  检查是否转换成功
        if (n < 0) {
            //  如果转换失败，则打印错误信息并返回1
            fprintf(stderr, "%s: error: failed to convert token to piece\n", __func__);
            return 1;
        }
        //  打印piece
        std::string s(buf, n);
        printf("%s", s.c_str());
    }

    // prepare a batch for the prompt(为提示准备一个批次)
    llama_batch batch = llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());

    // main loop(主循环)
    const auto t_main_start = ggml_time_us();
    //  初始化解码数量
    int n_decode = 0;
    //  初始化新的token_id
    llama_token new_token_id;
    //  循环直到生成的token数量达到预测数量
    for (int n_pos = 0; n_pos + batch.n_tokens < n_prompt + n_predict; ) {
        // evaluate the current batch with the transformer model(使用transformer模型评估当前批次)
        if (llama_decode(ctx, batch)) {
            //  如果评估失败，则打印错误信息并返回1
            fprintf(stderr, "%s : failed to eval, return code %d\n", __func__, 1);
            return 1;
        }
        //  更新token位置
        n_pos += batch.n_tokens;

        // sample the next token(采样下一个token)
        {
            // sample from the logits of the last token in the batch(从批次中的最后一个token的logits中采样)
            new_token_id = llama_sampler_sample(smpl, ctx, -1);

            // is it an end of generation?(是否生成结束)
            if (llama_token_is_eog(model, new_token_id)) {
                break;
            }
            //  接受token
            char buf[128];
            // convert token to piece(将token转换为piece)
            int n = llama_token_to_piece(model, new_token_id, buf, sizeof(buf), 0, true);
            if (n < 0) {
                //  如果转换失败，则打印错误信息并返回1
                fprintf(stderr, "%s: error: failed to convert token to piece\n", __func__);
                return 1;
            }
            //  打印piece
            std::string s(buf, n);
            printf("%s", s.c_str());
            fflush(stdout);

            // prepare the next batch with the sampled token(使用采样的token准备下一个批次)
            batch = llama_batch_get_one(&new_token_id, 1);
            //  更新解码数量
            n_decode += 1;
        }
    }

    printf("\n");
    //  等待所有计算完成
    const auto t_main_end = ggml_time_us();
    //  打印解码数量
    fprintf(stderr, "%s: decoded %d tokens in %.2f s, speed: %.2f t/s\n",
            __func__, n_decode, (t_main_end - t_main_start) / 1000000.0f, n_decode / ((t_main_end - t_main_start) / 1000000.0f));
    //  打印性能
    fprintf(stderr, "\n");
    llama_perf_sampler_print(smpl);
    llama_perf_context_print(ctx);
    fprintf(stderr, "\n");
    //  释放资源
    llama_sampler_free(smpl);
    llama_free(ctx);
    llama_free_model(model);

    return 0;
}
