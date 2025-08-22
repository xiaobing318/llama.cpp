#pragma once

#include "common.h"
#include "log.h"
#include "llama.h"
#include "arg.h" // common_remote_get_content
#include "base64.hpp"
#include "mtmd.h"
#include "mtmd-helper.h"
#include "chat.h"

// increase max payload length to allow use of larger context size
#define CPPHTTPLIB_FORM_URL_ENCODED_PAYLOAD_MAX_LENGTH 1048576
// increase backlog size to avoid connection resets for >> 1 slots
#define CPPHTTPLIB_LISTEN_BACKLOG 512
// disable Nagle's algorithm
#define CPPHTTPLIB_TCP_NODELAY true
#include <cpp-httplib/httplib.h>

#define JSON_ASSERT GGML_ASSERT
#include <nlohmann/json.hpp>

#include <random>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <cinttypes>

#define DEFAULT_OAICOMPAT_MODEL "gpt-3.5-turbo"

using json = nlohmann::ordered_json;

#define SLT_INF(slot, fmt, ...) LOG_INF("slot %12.*s: id %2d | task %d | " fmt, 12, __func__, (slot).id, (slot).id_task, __VA_ARGS__)
#define SLT_WRN(slot, fmt, ...) LOG_WRN("slot %12.*s: id %2d | task %d | " fmt, 12, __func__, (slot).id, (slot).id_task, __VA_ARGS__)
#define SLT_ERR(slot, fmt, ...) LOG_ERR("slot %12.*s: id %2d | task %d | " fmt, 12, __func__, (slot).id, (slot).id_task, __VA_ARGS__)
#define SLT_DBG(slot, fmt, ...) LOG_DBG("slot %12.*s: id %2d | task %d | " fmt, 12, __func__, (slot).id, (slot).id_task, __VA_ARGS__)

#define SRV_INF(fmt, ...) LOG_INF("srv  %12.*s: " fmt, 12, __func__, __VA_ARGS__)
#define SRV_WRN(fmt, ...) LOG_WRN("srv  %12.*s: " fmt, 12, __func__, __VA_ARGS__)
#define SRV_ERR(fmt, ...) LOG_ERR("srv  %12.*s: " fmt, 12, __func__, __VA_ARGS__)
#define SRV_DBG(fmt, ...) LOG_DBG("srv  %12.*s: " fmt, 12, __func__, __VA_ARGS__)

#define QUE_INF(fmt, ...) LOG_INF("que  %12.*s: " fmt, 12, __func__, __VA_ARGS__)
#define QUE_WRN(fmt, ...) LOG_WRN("que  %12.*s: " fmt, 12, __func__, __VA_ARGS__)
#define QUE_ERR(fmt, ...) LOG_ERR("que  %12.*s: " fmt, 12, __func__, __VA_ARGS__)
#define QUE_DBG(fmt, ...) LOG_DBG("que  %12.*s: " fmt, 12, __func__, __VA_ARGS__)

using raw_buffer = std::vector<uint8_t>;

template <typename T>
static T json_value(const json & body, const std::string & key, const T & default_value) {
    // Fallback null to default value
    if (body.contains(key) && !body.at(key).is_null()) {
        try {
            return body.at(key);
        } catch (NLOHMANN_JSON_NAMESPACE::detail::type_error const &) {
            LOG_WRN("Wrong type supplied for parameter '%s'. Expected '%s', using default value\n", key.c_str(), json(default_value).type_name());
            return default_value;
        }
    } else {
        return default_value;
    }
}

const static std::string build_info("b" + std::to_string(LLAMA_BUILD_NUMBER) + "-" + LLAMA_COMMIT);

// thin wrapper around common_grammar_trigger with (de)serialization functions
struct server_grammar_trigger {
    common_grammar_trigger value;

    server_grammar_trigger() = default;
    server_grammar_trigger(const common_grammar_trigger & value) : value(value) {}
    server_grammar_trigger(const json & in) {
        value.type = (common_grammar_trigger_type) in.at("type").get<int>();
        value.value = in.at("value").get<std::string>();
        if (value.type == COMMON_GRAMMAR_TRIGGER_TYPE_TOKEN) {
            value.token = (llama_token) in.at("token").get<int>();
        }
    }

    json to_json() const {
        json out {
            {"type", (int) value.type},
            {"value", value.value},
        };
        if (value.type == COMMON_GRAMMAR_TRIGGER_TYPE_TOKEN) {
            out["token"] = (int) value.token;
        }
        return out;
    }
};

//
// tokenizer and input processing utils
//

static bool json_is_array_of_numbers(const json & data) {
    if (data.is_array()) {
        for (const auto & e : data) {
            if (!e.is_number_integer()) {
                return false;
            }
        }
        return true;
    }
    return false;
}

// is array having BOTH numbers & strings?
static bool json_is_array_of_mixed_numbers_strings(const json & data) {
    bool seen_string = false;
    bool seen_number = false;
    if (data.is_array()) {
        for (const auto & e : data) {
            seen_string |= e.is_string();
            seen_number |= e.is_number_integer();
            if (seen_number && seen_string) {
                return true;
            }
        }
    }
    return false;
}

// get value by path(key1 / key2)
static json json_get_nested_values(const std::vector<std::string> & paths, const json & js) {
    json result = json::object();

    for (const std::string & path : paths) {
        json current = js;
        const auto keys = string_split<std::string>(path, /*separator*/ '/');
        bool valid_path = true;
        for (const std::string & k : keys) {
            if (valid_path && current.is_object() && current.contains(k)) {
                current = current[k];
            } else {
                valid_path = false;
            }
        }
        if (valid_path) {
            result[path] = current;
        }
    }
    return result;
}

/**
 * this handles 2 cases:
 * - only string, example: "string"
 * - mixed string and tokens, example: [12, 34, "string", 56, 78]
 */
static llama_tokens tokenize_mixed(const llama_vocab * vocab, const json & json_prompt, bool add_special, bool parse_special) {
    // If `add_bos` is true, we only add BOS, when json_prompt is a string,
    // or the first element of the json_prompt array is a string.
    llama_tokens prompt_tokens;

    if (json_prompt.is_array()) {
        bool first = true;
        for (const auto & p : json_prompt) {
            if (p.is_string()) {
                auto s = p.template get<std::string>();

                llama_tokens p;
                if (first) {
                    p = common_tokenize(vocab, s, add_special, parse_special);
                    first = false;
                } else {
                    p = common_tokenize(vocab, s, false, parse_special);
                }

                prompt_tokens.insert(prompt_tokens.end(), p.begin(), p.end());
            } else {
                if (first) {
                    first = false;
                }

                prompt_tokens.push_back(p.template get<llama_token>());
            }
        }
    } else {
        auto s = json_prompt.template get<std::string>();
        prompt_tokens = common_tokenize(vocab, s, add_special, parse_special);
    }

    return prompt_tokens;
}

/**
 * break the input "prompt" object into multiple prompt if needed, then tokenize them
 * this supports these cases:
 * - "prompt": "string"
 * - "prompt": [12, 34, 56]
 * - "prompt": [12, 34, "string", 56, 78]
 * and multiple prompts (multi-tasks):
 * - "prompt": ["string1", "string2"]
 * - "prompt": ["string1", [12, 34, 56]]
 * - "prompt": [[12, 34, 56], [78, 90, 12]]
 * - "prompt": [[12, 34, "string", 56, 78], [12, 34, 56]]
 */
static std::vector<llama_tokens> tokenize_input_prompts(const llama_vocab * vocab, const json & json_prompt, bool add_special, bool parse_special) {
    std::vector<llama_tokens> result;
    if (json_prompt.is_string() || json_is_array_of_mixed_numbers_strings(json_prompt)) {
        // string or mixed
        result.push_back(tokenize_mixed(vocab, json_prompt, add_special, parse_special));
    } else if (json_is_array_of_numbers(json_prompt)) {
        // array of tokens
        result.push_back(json_prompt.get<llama_tokens>());
    } else if (json_prompt.is_array()) {
        // array of prompts
        result.reserve(json_prompt.size());
        for (const auto & p : json_prompt) {
            if (p.is_string() || json_is_array_of_mixed_numbers_strings(p)) {
                result.push_back(tokenize_mixed(vocab, p, add_special, parse_special));
            } else if (json_is_array_of_numbers(p)) {
                // array of tokens
                result.push_back(p.get<llama_tokens>());
            } else {
                throw std::runtime_error("element of \"prompt\" must be a string, an list of tokens, or a list of mixed strings & tokens");
            }
        }
    } else {
        throw std::runtime_error("\"prompt\" must be a string, an list of tokens, a list of mixed strings & tokens, or a list of prompts");
    }
    if (result.empty()) {
        throw std::runtime_error("\"prompt\" must not be empty");
    }
    return result;
}

// return the last index of character that can form a valid string
// if the last character is potentially cut in half, return the index before the cut
// if validate_utf8(text) == text.size(), then the whole text is valid utf8
static size_t validate_utf8(const std::string& text) {
    size_t len = text.size();
    if (len == 0) return 0;

    // Check the last few bytes to see if a multi-byte character is cut off
    for (size_t i = 1; i <= 4 && i <= len; ++i) {
        unsigned char c = text[len - i];
        // Check for start of a multi-byte sequence from the end
        if ((c & 0xE0) == 0xC0) {
            // 2-byte character start: 110xxxxx
            // Needs at least 2 bytes
            if (i < 2) return len - i;
        } else if ((c & 0xF0) == 0xE0) {
            // 3-byte character start: 1110xxxx
            // Needs at least 3 bytes
            if (i < 3) return len - i;
        } else if ((c & 0xF8) == 0xF0) {
            // 4-byte character start: 11110xxx
            // Needs at least 4 bytes
            if (i < 4) return len - i;
        }
    }

    // If no cut-off multi-byte character is found, return full length
    return len;
}

//
// template utils
//

// format rerank task: [BOS]query[EOS][SEP]doc[EOS]
static llama_tokens format_rerank(const struct llama_vocab * vocab, const llama_tokens & query, const llama_tokens & doc) {
    llama_tokens result;

    // Get EOS token - use SEP token as fallback if EOS is not available
    llama_token eos_token = llama_vocab_eos(vocab);
    if (eos_token == LLAMA_TOKEN_NULL) {
        eos_token = llama_vocab_sep(vocab);
    }

    result.reserve(doc.size() + query.size() + 4);
    if (llama_vocab_get_add_bos(vocab)) {
        result.push_back(llama_vocab_bos(vocab));
    }
    result.insert(result.end(), query.begin(), query.end());
    if (llama_vocab_get_add_eos(vocab)) {
        result.push_back(eos_token);
    }
    if (llama_vocab_get_add_sep(vocab)) {
        result.push_back(llama_vocab_sep(vocab));
    }
    result.insert(result.end(), doc.begin(), doc.end());
    if (llama_vocab_get_add_eos(vocab)) {
        result.push_back(eos_token);
    }

    return result;
}

// format infill task
static llama_tokens format_infill(
        const llama_vocab * vocab,
        const json & input_prefix,
        const json & input_suffix,
        const json & input_extra,
        const int n_batch,
        const int n_predict,
        const int n_ctx,
        const bool spm_infill,
        const llama_tokens & tokens_prompt
    ) {
    // TODO: optimize this block by reducing memory allocations and movement

    // use FIM repo-level pattern:
    // ref: https://arxiv.org/pdf/2409.12186
    //
    // [FIM_REP]myproject
    // [FIM_SEP]filename0
    // extra chunk 0
    // [FIM_SEP]filename1
    // extra chunk 1
    // ...
    // [FIM_SEP]filename
    // [FIM_PRE]prefix[FIM_SUF]suffix[FIM_MID]prompt
    //
    llama_tokens extra_tokens;
    extra_tokens.reserve(n_ctx);

    auto tokens_prefix = tokenize_mixed(vocab, input_prefix, false, false);
    auto tokens_suffix = tokenize_mixed(vocab, input_suffix, false, false);

    if (llama_vocab_fim_rep(vocab) != LLAMA_TOKEN_NULL) {
        // TODO: make project name an input
        static const auto k_fim_repo = common_tokenize(vocab, "myproject\n", false, false);

        extra_tokens.push_back(llama_vocab_fim_rep(vocab));
        extra_tokens.insert(extra_tokens.end(), k_fim_repo.begin(), k_fim_repo.end());
    }
    for (const auto & chunk : input_extra) {
        // { "text": string, "filename": string }
        const std::string text     = json_value(chunk, "text",     std::string());
        const std::string filename = json_value(chunk, "filename", std::string("tmp"));

        if (llama_vocab_fim_sep(vocab) != LLAMA_TOKEN_NULL) {
            const auto k_fim_file = common_tokenize(vocab, filename + "\n", false, false);

            extra_tokens.insert(extra_tokens.end(), llama_vocab_fim_sep(vocab));
            extra_tokens.insert(extra_tokens.end(), k_fim_file.begin(), k_fim_file.end());
        } else {
            // chunk separator in binary form to avoid confusing the AI
            static const char k_chunk_prefix_str[] = {0x0a, 0x0a, 0x2d, 0x2d, 0x2d, 0x20, 0x73, 0x6e, 0x69, 0x70, 0x70, 0x65, 0x74, 0x20, 0x2d, 0x2d, 0x2d, 0x0a, 0x0a, 0x00};
            static const auto k_chunk_prefix_tokens = common_tokenize(vocab, k_chunk_prefix_str, false, false);

            extra_tokens.insert(extra_tokens.end(), k_chunk_prefix_tokens.begin(), k_chunk_prefix_tokens.end());
        }

        const auto chunk_tokens = common_tokenize(vocab, text, false, false);
        extra_tokens.insert(extra_tokens.end(), chunk_tokens.begin(), chunk_tokens.end());
    }

    if (llama_vocab_fim_sep(vocab) != LLAMA_TOKEN_NULL) {
        // TODO: current filename
        static const auto k_fim_file = common_tokenize(vocab, "filename\n", false, false);

        extra_tokens.insert(extra_tokens.end(), llama_vocab_fim_sep(vocab));
        extra_tokens.insert(extra_tokens.end(), k_fim_file.begin(), k_fim_file.end());
    }

    // for now pick FIM context to fit in a batch (ratio prefix:suffix = 3:1, TODO: configurable?)
    const int n_prefix_take = std::min<int>(tokens_prefix.size(),                3*(n_batch/4));
    const int n_suffix_take = std::min<int>(tokens_suffix.size(), std::max<int>(0, (n_batch/4) - (2 + tokens_prompt.size())));

    SRV_DBG("n_prefix_take = %d, n_suffix_take = %d, total = %d\n", n_prefix_take, n_suffix_take, (n_prefix_take + n_suffix_take));

    // fill the rest of the context with extra chunks
    const int n_extra_take = std::min<int>(std::max<int>(0, n_ctx - (n_batch) - 2*n_predict), extra_tokens.size());

    tokens_prefix.erase(tokens_prefix.begin(), tokens_prefix.begin() + tokens_prefix.size() - n_prefix_take);
    tokens_suffix.resize(n_suffix_take);

    tokens_prefix.insert(tokens_prefix.begin(), llama_vocab_fim_pre(vocab));
    tokens_prefix.insert(tokens_prefix.end(),   tokens_prompt.begin(), tokens_prompt.end());
    tokens_suffix.insert(tokens_suffix.begin(), llama_vocab_fim_suf(vocab));

    auto embd_inp = spm_infill ? tokens_suffix : tokens_prefix;
    auto embd_end = spm_infill ? tokens_prefix : tokens_suffix;

    if (llama_vocab_get_add_bos(vocab)) {
        embd_inp.insert(embd_inp.begin(), llama_vocab_bos(vocab));
    }

    SRV_DBG("extra: n_ctx = %d, n_extra_take = %d, n_extra = %d\n", n_ctx, n_extra_take, (int) extra_tokens.size());

    // put the extra context before the FIM prefix
    embd_inp.insert(embd_inp.begin(), extra_tokens.end() - n_extra_take, extra_tokens.end());

    embd_inp.insert(embd_inp.end(), embd_end.begin(), embd_end.end());
    embd_inp.push_back(llama_vocab_fim_mid(vocab));

    return embd_inp;
}

//
// base64 utils (TODO: move to common in the future)
//

static const std::string base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

static inline bool is_base64(uint8_t c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

static inline raw_buffer base64_decode(const std::string & encoded_string) {
    int i = 0;
    int j = 0;
    int in_ = 0;

    int in_len = encoded_string.size();

    uint8_t char_array_4[4];
    uint8_t char_array_3[3];

    raw_buffer ret;

    while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++) {
                char_array_4[i] = base64_chars.find(char_array_4[i]);
            }

            char_array_3[0] = ((char_array_4[0]      ) << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) +   char_array_4[3];

            for (i = 0; (i < 3); i++) {
                ret.push_back(char_array_3[i]);
            }

            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 4; j++) {
            char_array_4[j] = 0;
        }

        for (j = 0; j < 4; j++) {
            char_array_4[j] = base64_chars.find(char_array_4[j]);
        }

        char_array_3[0] = ((char_array_4[0]      ) << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) +   char_array_4[3];

        for (j = 0; j < i - 1; j++) {
            ret.push_back(char_array_3[j]);
        }
    }

    return ret;
}

//
// random string / id
//

static std::string random_string() {
    static const std::string str("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

    std::random_device rd;
    std::mt19937 generator(rd());

    std::string result(32, ' ');

    for (int i = 0; i < 32; ++i) {
        result[i] = str[generator() % str.size()];
    }

    return result;
}

static std::string gen_chatcmplid() {
    return "chatcmpl-" + random_string();
}

static std::string gen_tool_call_id() {
    return random_string();
}

//
// other common utils
//

// TODO: reuse llama_detokenize
template <class Iter>
static std::string tokens_to_str(llama_context * ctx, Iter begin, Iter end) {
    std::string ret;
    for (; begin != end; ++begin) {
        ret += common_token_to_piece(ctx, *begin);
    }

    return ret;
}

// format incomplete utf-8 multibyte character for output
static std::string tokens_to_output_formatted_string(const llama_context * ctx, const llama_token token) {
    std::string out = token == LLAMA_TOKEN_NULL ? "" : common_token_to_piece(ctx, token);

    // if the size is 1 and first bit is 1, meaning it's a partial character
    //   (size > 1 meaning it's already a known token)
    if (out.size() == 1 && (out[0] & 0x80) == 0x80) {
        std::stringstream ss;
        ss << std::hex << (out[0] & 0xff);
        std::string res(ss.str());
        out = "byte: \\x" + res;
    }

    return out;
}

static bool server_sent_event(httplib::DataSink & sink, const char * event, const json & data) {
    const std::string str =
        std::string(event) + ": " +
        data.dump(-1, ' ', false, json::error_handler_t::replace) +
        "\n\n"; // required by RFC 8895 - A message is terminated by a blank line (two line terminators in a row).

    LOG_DBG("data stream, to_send: %s", str.c_str());

    return sink.write(str.c_str(), str.size());
}

//
// OAI utils
//

// used by /completions endpoint
static json oaicompat_completion_params_parse(const json & body) {
    json llama_params;

    if (!body.contains("prompt")) {
        throw std::runtime_error("\"prompt\" is required");
    }

    // Handle "stop" field
    if (body.contains("stop") && body.at("stop").is_string()) {
        llama_params["stop"] = json::array({body.at("stop").get<std::string>()});
    } else {
        llama_params["stop"] = json_value(body, "stop", json::array());
    }

    // Handle "n" field
    int n_choices = json_value(body, "n", 1);
    if (n_choices != 1) {
        throw std::runtime_error("Only one completion choice is allowed");
    }

    // Handle "echo" field
    if (json_value(body, "echo", false)) {
        throw std::runtime_error("Only no echo is supported");
    }

    // Params supported by OAI but unsupported by llama.cpp
    static const std::vector<std::string> unsupported_params { "best_of", "suffix" };
    for (const auto & param : unsupported_params) {
        if (body.contains(param)) {
            throw std::runtime_error("Unsupported param: " + param);
        }
    }

    // Copy remaining properties to llama_params
    for (const auto & item : body.items()) {
        // Exception: if "n_predict" is present, we overwrite the value specified earlier by "max_tokens"
        if (!llama_params.contains(item.key()) || item.key() == "n_predict") {
            llama_params[item.key()] = item.value();
        }
    }

    return llama_params;
}

struct oaicompat_parser_options {
    bool use_jinja;
    bool prefill_assistant;
    common_reasoning_format reasoning_format;
    std::map<std::string,std::string> chat_template_kwargs;
    common_chat_templates * tmpls;
    bool allow_image;
    bool allow_audio;
    bool enable_thinking = true;
};

/*
 * ========================================================================
 * 📚 oaicompat_chat_params_parse - OpenAI ChatCompletion API兼容层核心函数
 * ========================================================================
 * 
 * 🎯 核心作用：
 * 这是llama.cpp服务器的"万能翻译器"，专门负责实现OpenAI ChatCompletion API的完全兼容。
 * 它将客户端发送的标准OpenAI格式聊天请求转换为llama.cpp内部能够处理的参数格式。
 * 
 * 💡 为什么需要这个函数？
 * 1. API兼容性：让现有使用ChatGPT API的应用无需修改即可切换到llama.cpp
 * 2. 格式差异：OpenAI使用messages数组格式，llama.cpp使用prompt字符串格式
 * 3. 参数映射：OpenAI的max_tokens对应llama.cpp的n_predict等参数名差异
 * 4. 特殊功能：处理多模态输入（图片/音频）、工具调用、聊天模板等高级功能
 * 
 * 🚨 如果没有这个函数会怎样？
 * 1. 无法兼容OpenAI API，现有应用无法直接迁移到llama.cpp
 * 2. 客户端需要学习llama.cpp专有的API格式，增加使用门槛
 * 3. 多模态数据无法正确解析，图片/音频功能失效
 * 4. 聊天模板无法应用，生成质量严重下降
 * 5. 工具调用功能完全无法使用
 */
static json oaicompat_chat_params_parse(
    json & body,                              /* 输入：OpenAI格式的聊天请求JSON */
    const oaicompat_parser_options & opt,     /* 输入：解析器配置选项 */
    std::vector<raw_buffer> & out_files)      /* 输出：解码后的多媒体文件数据 */
{
    /*
     * 🎯 第一步：初始化输出参数容器
     * 这个JSON对象将存储所有转换后的llama.cpp格式参数
     */
    json llama_params;

    /*
     * 📦 提取工具相关参数 - 支持函数调用功能
     * 
     * tools: 用户定义的可调用函数列表（类似ChatGPT的function calling）
     * has_tools: 布尔值，检查是否定义了工具函数
     * stream: 是否启用流式输出（实时返回AI回复，而不是等全部生成完）
     * tool_choice: 工具选择策略（"auto"=自动选择，"none"=不使用，或指定函数名）
     */
    auto tools = json_value(body, "tools", json());          // 提取工具函数定义数组
    auto has_tools = tools.is_array() && !tools.empty();    // 检查是否真的有工具函数
    auto stream = json_value(body, "stream", false);         // 是否使用流式输出
    auto tool_choice = json_value(body, "tool_choice", std::string("auto")); // 工具选择策略

    /*
     * 🔒 工具功能前置检查 - 确保有必要的支持
     * 
     * Jinja是一个模板引擎，工具调用需要它来格式化函数调用的prompt
     * 如果没有启用Jinja支持，就不能使用工具功能
     * 这是一个安全检查，避免用户配置错误导致功能异常
     */
    if (!opt.use_jinja) {  // 如果没有启用Jinja模板支持
        if (has_tools) {
            // 有工具但没有Jinja支持 → 抛出错误，提示用户需要启用--jinja参数
            throw std::runtime_error("tools param requires --jinja flag");
        }
        if (tool_choice != "auto") {
            // 指定了工具选择但没有Jinja支持 → 同样抛出错误
            throw std::runtime_error("tool_choice param requires --jinja flag");
        }
    }

    /*
     * 🛑 处理停止词参数 - 告诉AI什么时候该停止生成
     * 
     * "stop"参数定义了停止生成的触发词/短语
     * 当AI生成的文本中出现这些词时，会立即停止继续生成
     * 
     * 格式兼容性处理：
     * - OpenAI API支持字符串或字符串数组两种格式
     * - llama.cpp内部统一使用数组格式
     * - 这里做格式统一化：单个字符串转换成包含一个元素的数组
     */
    if (body.contains("stop") && body.at("stop").is_string()) {
        // 如果stop是单个字符串，转换为数组格式 ["stop_word"]
        llama_params["stop"] = json::array({body.at("stop").get<std::string>()});
    } else {
        // 如果stop本身就是数组或未提供，直接使用（默认为空数组）
        llama_params["stop"] = json_value(body, "stop", json::array());
    }

    /*
     * 📋 处理输出格式约束参数
     * 
     * json_schema: JSON格式规范，用于约束AI输出特定格式的JSON
     * grammar: 语法规则，用于约束AI输出符合特定语法的文本
     * 
     * 这两个参数不能同时使用，因为它们都是用来约束输出格式的
     */
    auto json_schema = json_value(body, "json_schema", json());
    auto grammar = json_value(body, "grammar", std::string());    // 提取语法约束规则
    if (!json_schema.is_null() && !grammar.empty()) {
        // 互斥检查：不能同时指定JSON格式和语法规则
        throw std::runtime_error("Cannot use both json_schema and grammar");
    }

    /*
     * 🎨 处理响应格式参数 - OpenAI标准的格式控制
     * 
     * response_format是OpenAI API的标准字段，用于指定AI回复的格式
     * 支持的格式类型：
     * - "text": 普通文本（默认）
     * - "json_object": 简单JSON对象
     * - "json_schema": 带有具体格式约束的JSON（更严格）
     * 
     * 这里需要把OpenAI的response_format转换成llama.cpp的json_schema
     */
    if (body.contains("response_format")) {
        json response_format      = json_value(body, "response_format", json::object());     // 提取响应格式配置
        std::string response_type = json_value(response_format, "type", std::string());  // 获取格式类型
        
        if (response_type == "json_object") {
            // 简单JSON对象模式：AI回复必须是有效的JSON格式
            json_schema = json_value(response_format, "schema", json::object());
        } else if (response_type == "json_schema") {
            // 严格JSON格式模式：AI回复必须符合指定的JSON Schema
            auto schema_wrapper = json_value(response_format, "json_schema", json::object());
            json_schema = json_value(schema_wrapper, "schema", json::object());
        } else if (!response_type.empty() && response_type != "text") {
            // 格式验证：只支持text、json_object、json_schema三种类型
            throw std::runtime_error("response_format type must be one of \"text\" or \"json_object\", but got: " + response_type);
        }
    }

    /*
     * 💬 第二步：处理对话消息 - 聊天系统的核心数据
     * 
     * messages是聊天请求的核心，包含整个对话历史
     * 格式：[{role: "system/user/assistant", content: "..."}, ...]
     * 
     * 必须进行严格验证，确保数据格式正确
     */
    if (!body.contains("messages")) {
        // messages是必需字段，没有就无法进行聊天
        throw std::runtime_error("'messages' is required");
    }
    json & messages = body.at("messages");        // 获取消息数组的引用
    if (!messages.is_array()) {
        // messages必须是数组格式，单个消息也不行
        throw std::runtime_error("Expected 'messages' to be an array");
    }
    /*
     * 🔍 遍历每条消息，进行格式验证和多媒体提取
     * 
     * 这个循环是整个函数最复杂的部分，需要：
     * 1. 验证每条消息的格式是否正确
     * 2. 提取并处理图片、音频等多媒体内容
     * 3. 把多媒体内容替换成占位符，供后续模板处理
     */
    for (auto & msg : messages) {
        /*
         * 🎭 验证消息角色和内容
         * 
         * 每条消息都有role字段，表示发送者身份：
         * - "system": 系统指令（告诉AI如何行为）
         * - "user": 用户消息（人类用户的输入）
         * - "assistant": AI回复（之前AI的回答）
         */
        std::string role = json_value(msg, "role", std::string());
        
        // 非assistant消息必须有content字段（系统指令和用户消息都需要内容）
        if (role != "assistant" && !msg.contains("content")) {
            throw std::runtime_error("All non-assistant messages must contain 'content'");
        }
        
        // assistant消息比较特殊，可能只有工具调用而没有文本内容
        if (role == "assistant") {
            if (!msg.contains("content") && !msg.contains("tool_calls")) {
                // assistant消息必须至少包含内容或工具调用之一
                throw std::runtime_error("Assistant message must contain either 'content' or 'tool_calls'!");
            }
            if (!msg.contains("content")) {
                // 如果只有工具调用没有内容，跳过多媒体处理
                continue; // avoid errors with no content
            }
        }
        /*
         * 📝 处理消息内容 - 支持文本和多媒体混合
         * 
         * OpenAI API支持两种content格式：
         * 1. 简单字符串：纯文本消息
         * 2. 复杂数组：包含文本、图片、音频等多种内容类型
         * 
         * 这里重点处理复杂格式，简单格式直接跳过
         */
        json & content = msg.at("content");
        if (content.is_string() || content.is_null()) {
            // 纯文本内容，无需特殊处理，直接跳过
            continue;
        }

        if (!content.is_array()) {
            // content必须是字符串或数组，其他格式都不支持
            throw std::runtime_error("Expected 'content' to be a string or an array");
        }

        /*
         * 🎨 处理多媒体内容数组
         * 
         * 数组格式的content包含多个部分，每个部分可能是：
         * - {type: "text", text: "文本内容"}
         * - {type: "image_url", image_url: {url: "图片地址或base64"}}
         * - {type: "input_audio", input_audio: {data: "base64音频", format: "wav/mp3"}}
         */
        for (auto & p : content) {
            std::string type = json_value(p, "type", std::string());    // 获取内容类型
            
            /*
             * 🖼️ 处理图片内容
             * 
             * 支持两种图片输入方式：
             * 1. HTTP/HTTPS网络图片链接（自动下载）
             * 2. Base64编码的图片数据（data:image/jpeg;base64,xxx格式）
             * 
             * 处理后会把图片替换成特殊占位符，实际图片数据存储在out_files中
             */
            if (type == "image_url") {
                // 前置检查：确保服务器支持图片处理
                if (!opt.allow_image) {
                    throw std::runtime_error("image input is not supported - hint: if this is unexpected, you may need to provide the mmproj");
                }

                json image_url  = json_value(p, "image_url", json::object());    // 提取图片URL配置
                std::string url = json_value(image_url, "url", std::string());   // 获取图片地址
                
                if (string_starts_with(url, "http")) {
                    /*
                     * 📥 处理网络图片：自动下载到内存
                     * 
                     * 对于http/https链接，需要：
                     * 1. 设置下载参数（用户代理、大小限制、超时时间）
                     * 2. 发起HTTP请求下载图片
                     * 3. 验证下载结果并存储到文件容器
                     */
                    // 配置下载参数
                    // TODO @ngxson : 这些参数未来可以做成可配置的
                    common_remote_params params;
                    params.headers.push_back("User-Agent: llama.cpp/" + build_info);  // 设置用户代理标识
                    params.max_size = 1024 * 1024 * 10; // 最大10MB，防止下载过大文件
                    params.timeout  = 10; // 10秒超时，避免长时间等待
                    
                    // 开始下载图片
                    SRV_INF("downloading image from '%s'\n", url.c_str());
                    auto res = common_remote_get_content(url, params);  // res.first=HTTP状态码, res.second=文件内容
                    
                    if (200 <= res.first && res.first < 300) {
                        // 下载成功（HTTP 2xx状态码）
                        SRV_INF("downloaded %ld bytes\n", res.second.size());
                        raw_buffer data;  // 创建文件数据容器
                        data.insert(data.end(), res.second.begin(), res.second.end());  // 复制下载的数据
                        out_files.push_back(data);  // 添加到输出文件列表
                    } else {
                        // 下载失败（HTTP错误状态码）
                        throw std::runtime_error("Failed to download image");
                    }

                } else {
                    /*
                     * 📊 处理Base64编码图片：解码内嵌图片数据
                     * 
                     * Base64格式：data:image/jpeg;base64,/9j/4AAQSkZJRgABAQAAAQ...
                     * 格式分析：
                     * - "data:image/jpeg;base64" : 数据类型声明部分
                     * - "/9j/4AAQSkZJRgABAQAAAQ..." : 实际的base64编码数据
                     * 
                     * 解码步骤：
                     * 1. 按逗号分割URL，获得类型声明和数据两部分
                     * 2. 验证格式是否正确（必须是data:image/xxx;base64格式）
                     * 3. 解码base64数据为二进制图片文件
                     */
                    std::vector<std::string> parts = string_split<std::string>(url, /*separator*/ ',');
                    if (parts.size() != 2) {
                        // 格式错误：应该只有两部分（类型声明,数据）
                        throw std::runtime_error("Invalid image_url.url value");
                    } else if (!string_starts_with(parts[0], "data:image/")) {
                        // 类型错误：必须是图片数据类型
                        throw std::runtime_error("Invalid image_url.url format: " + parts[0]);
                    } else if (!string_ends_with(parts[0], "base64")) {
                        // 编码错误：目前只支持base64编码
                        throw std::runtime_error("image_url.url must be base64 encoded");
                    } else {
                        // 解码成功：提取并解码base64数据
                        auto base64_data = parts[1];                      // 获取base64字符串
                        auto decoded_data = base64_decode(base64_data);   // 解码为二进制数据
                        out_files.push_back(decoded_data);               // 添加到输出文件列表
                    }
                }

                /*
                 * 🔄 图片内容替换：用占位符替代原始图片数据
                 * 
                 * 处理完图片后，需要把原来的图片内容替换成文本占位符：
                 * 1. 把type改为"text"（告诉后续处理这是文本内容）
                 * 2. 用特殊标记替代原始图片URL（模型能识别这个标记表示图片）
                 * 3. 删除原始image_url字段（清理不需要的数据）
                 * 
                 * 这样做的好处是：实际图片数据单独存储，文本部分保持简洁
                 */
                p["type"] = "text";                    // 改变内容类型为文本
                p["text"] = mtmd_default_marker();     // 插入图片占位符标记
                p.erase("image_url");                  // 删除原始图片URL字段

            } else if (type == "input_audio") {
                /*
                 * 🎵 处理音频内容
                 * 
                 * 类似图片处理，但音频只支持base64编码格式
                 * 支持的音频格式：WAV、MP3（与OpenAI API保持一致）
                 * 
                 * 处理流程：
                 * 1. 检查是否启用音频支持
                 * 2. 验证音频格式（只允许wav/mp3）
                 * 3. 解码base64音频数据
                 * 4. 用占位符替换原始音频内容
                 */
                // 前置检查：确保服务器支持音频处理
                if (!opt.allow_audio) {
                    throw std::runtime_error("audio input is not supported - hint: if this is unexpected, you may need to provide the mmproj");
                }

                // 提取音频配置信息
                json input_audio   = json_value(p, "input_audio", json::object());  // 获取音频配置对象
                std::string data   = json_value(input_audio, "data", std::string()); // 获取base64编码的音频数据
                std::string format = json_value(input_audio, "format", std::string()); // 获取音频格式
                
                // 格式验证：严格按照OpenAI API规范
                // 注意：虽然llama.cpp支持FLAC，但为了与OpenAI保持兼容，这里不允许
                if (format != "wav" && format != "mp3") {
                    throw std::runtime_error("input_audio.format must be either 'wav' or 'mp3'");
                }
                
                // 解码音频数据
                auto decoded_data = base64_decode(data); // 音频数据预期是base64编码格式
                out_files.push_back(decoded_data);       // 添加到输出文件列表

                /*
                 * 🔄 音频内容替换：与图片处理相同的占位符替换逻辑
                 * 
                 * 把原始音频内容替换成文本占位符，保持消息结构的一致性
                 */
                p["type"] = "text";                    // 改变内容类型为文本
                p["text"] = mtmd_default_marker();     // 插入音频占位符标记
                p.erase("input_audio");                // 删除原始音频字段

            } else if (type != "text") {
                // 内容类型验证：只支持text、image_url、input_audio三种类型
                throw std::runtime_error("unsupported content[].type");
            }
        }
    }

    /*
     * 🎯 第三步：准备聊天模板输入 - 格式化对话内容
     * 
     * 聊天模板是AI模型理解对话的关键，不同模型有不同的对话格式：
     * - Llama2: <s>[INST] {user_message} [/INST] {assistant_message} </s>
     * - ChatML: <|im_start|>user\n{message}<|im_end|>
     * - Alpaca: ### Instruction: {message} ### Response:
     * 
     * 这里准备所有必要的输入数据，供模板系统使用
     */
    common_chat_templates_inputs inputs;
    
    // 核心数据转换：把OpenAI格式转换成llama.cpp内部格式
    inputs.messages              = common_chat_msgs_parse_oaicompat(messages);      // 消息数组格式转换
    inputs.tools                 = common_chat_tools_parse_oaicompat(tools);        // 工具函数格式转换  
    inputs.tool_choice           = common_chat_tool_choice_parse_oaicompat(tool_choice); // 工具选择策略转换
    inputs.json_schema           = json_schema.is_null() ? "" : json_schema.dump(); // JSON格式约束转换
    inputs.grammar               = grammar;                                         // 语法规则（直接传递）
    
    // 模板系统配置
    inputs.use_jinja             = opt.use_jinja;                                   // 是否使用Jinja模板引擎
    inputs.parallel_tool_calls   = json_value(body, "parallel_tool_calls", false); // 是否支持并行工具调用
    inputs.add_generation_prompt = json_value(body, "add_generation_prompt", true); // 是否添加生成提示符
    
    // 高级功能配置
    inputs.reasoning_format      = opt.reasoning_format;                            // 推理格式（用于思维链等）
    inputs.enable_thinking       = opt.enable_thinking;                             // 是否启用思考模式
    /*
     * 🛠️ 工具调用相关配置
     * 
     * 当启用工具调用时，需要特殊处理：
     * 1. 工具调用与自定义语法规则互斥（避免冲突）
     * 2. 启用工具调用解析标志（让模型知道需要解析函数调用）
     */
    if (!inputs.tools.empty() && inputs.tool_choice != COMMON_CHAT_TOOL_CHOICE_NONE) {
        if (body.contains("grammar")) {
            // 冲突检查：不能同时使用工具调用和自定义语法
            throw std::runtime_error("Cannot use custom grammar constraints with tools.");
        }
        llama_params["parse_tool_calls"] = true;  // 告诉引擎需要解析工具调用
    }

    /*
     * 🔗 合并聊天模板参数
     * 
     * 模板参数来源有两个：
     * 1. 命令行参数（服务器启动时指定）
     * 2. 用户请求参数（chat_template_kwargs字段）
     * 
     * 用户请求的参数优先级更高，可以覆盖命令行参数
     */
    auto chat_template_kwargs_object = json_value(body, "chat_template_kwargs", json::object()); // 提取用户请求的模板参数
    inputs.chat_template_kwargs = opt.chat_template_kwargs;  // 先使用命令行参数作为基础
    for (const auto & item : chat_template_kwargs_object.items()) {
        // 用户请求的参数覆盖命令行参数（用户优先）
        inputs.chat_template_kwargs[item.key()] = item.value().dump();
    }

    /*
     * 🎭 Assistant消息预填充处理 - 高级推理功能
     * 
     * 这是一个高级功能，用于：
     * 1. 继续之前未完成的Assistant回复
     * 2. 引导模型按特定方式开始回答
     * 3. 修改推理模型的思维过程
     * 
     * 例如：如果最后一条消息是Assistant说了"我认为答案是"，
     * 那么AI会继续这句话，而不是重新开始一个完整回复
     */
    bool prefill_assistant_message = !inputs.messages.empty() && 
                                     inputs.messages.back().role == "assistant" && 
                                     opt.prefill_assistant;
    common_chat_msg last_message;  // 保存要预填充的消息
    
    if (prefill_assistant_message) {
        // 提取最后的Assistant消息作为预填充内容
        last_message = inputs.messages.back();
        inputs.messages.pop_back();  // 从消息列表中移除，稍后会特殊处理

        /* 安全检查：确保最多只有一个Assistant消息在末尾 */
        if (!inputs.messages.empty() && inputs.messages.back().role == "assistant"){
            throw std::runtime_error("Cannot have 2 or more assistant messages at the end of the list.");
        }

        /* 功能限制：预填充与推理格式不兼容 */
        /* TODO: 这个功能需要更充分的测试 */
        inputs.reasoning_format = COMMON_REASONING_FORMAT_NONE;

        /* 兼容性检查：预填充与思考模式不兼容 */
        if ( (!inputs.enable_thinking) || inputs.chat_template_kwargs.find("enable_thinking") != inputs.chat_template_kwargs.end()) {
            throw std::runtime_error("Assistant response prefill is incompatible with enable_thinking.");
        }

        inputs.add_generation_prompt = true;  // 确保添加生成提示符
    }

    /*
     * 🎨 第四步：应用聊天模板 - 生成最终prompt
     * 
     * 这是整个转换过程的核心步骤：
     * 1. 使用模型专用的聊天模板
     * 2. 把对话历史格式化成模型能理解的prompt
     * 3. 添加必要的特殊标记（开始符、结束符、角色标识等）
     * 
     * 输出的chat_params包含：
     * - prompt: 格式化后的完整对话文本
     * - grammar: 语法约束规则
     * - format: 聊天格式类型
     * - 其他模板相关参数
     */
    auto chat_params = common_chat_templates_apply(opt.tmpls, inputs);

    /*
     * 🔄 附加预填充内容
     * 
     * 如果启用了Assistant消息预填充，需要把预填充内容
     * 直接附加到生成的prompt末尾，这样AI就会从这里继续生成
     */
    if (prefill_assistant_message) {
        if (!last_message.content_parts.empty()) {
            // 多部分内容：遍历所有文本部分并拼接
            for (auto & p : last_message.content_parts) {
                chat_params.prompt += p.text;
            }
        } else {
            // 简单文本内容：直接附加到prompt
            chat_params.prompt += last_message.content;
        }
    }

    /*
     * 🏗️ 第五步：构建llama.cpp内部参数 - 最终格式转换
     * 
     * 把聊天模板的输出转换成llama.cpp推理引擎能直接使用的参数格式
     * 这些参数将直接传递给AI推理引擎进行文本生成
     */
    
    // 基础聊天参数
    llama_params["chat_format"]      = static_cast<int>(chat_params.format);  // 聊天格式类型（枚举转整数）
    llama_params["prompt"]           = chat_params.prompt;                    // 最终的完整prompt文本
    
    // 语法约束参数（可选）
    if (!chat_params.grammar.empty()) {
        llama_params["grammar"] = chat_params.grammar;  // 语法规则字符串
    }
    llama_params["grammar_lazy"]     = chat_params.grammar_lazy;             // 懒加载语法规则
    
    // 语法触发器配置（高级功能）
    auto grammar_triggers = json::array();
    for (const auto & trigger : chat_params.grammar_triggers) {
        server_grammar_trigger ct(trigger);  // 转换为服务器格式
        grammar_triggers.push_back(ct.to_json());
    }
    llama_params["grammar_triggers"] = grammar_triggers;                     // 语法触发器列表
    
    // 特殊token处理
    llama_params["preserved_tokens"] = chat_params.preserved_tokens;         // 保留的特殊token
    llama_params["thinking_forced_open"] = chat_params.thinking_forced_open; // 强制开启思考模式
    
    // 附加停止词（来自聊天模板）
    for (const auto & stop : chat_params.additional_stops) {
        llama_params["stop"].push_back(stop);  // 追加到现有停止词列表
    }

    /*
     * 🎯 处理生成选择数量参数
     * 
     * OpenAI API支持生成多个不同的回复选项（n参数）
     * 但llama.cpp目前只支持单个回复，所以这里做限制检查
     */
    int n_choices = json_value(body, "n", 1);  // 获取生成选择数量，默认1
    if (n_choices != 1) {
        // 目前只支持生成1个回复，多选择功能尚未实现
        throw std::runtime_error("Only one completion choice is allowed");
    }

    /*
     * 📊 处理日志概率参数 - 用于分析AI的"确信度"
     * 
     * logprobs功能可以显示AI对每个生成token的概率分布
     * 这对调试和理解AI的决策过程很有用
     * 
     * TODO: 当前的响应格式还不完全兼容OpenAI，但使用者较少，未来可能需要修复
     */
    if (json_value(body, "logprobs", false)) {  // 是否启用概率日志
        if (has_tools && stream) {
            // 功能冲突：工具调用+流式输出+概率日志三者不兼容
            throw std::runtime_error("logprobs is not supported with tools + stream");
        }
        llama_params["n_probs"] = json_value(body, "top_logprobs", 20);  // 设置显示概率的token数量
    } else if (body.contains("top_logprobs") && !body.at("top_logprobs").is_null()) {
        // 参数依赖检查：top_logprobs需要logprobs为true才有效
        throw std::runtime_error("top_logprobs requires logprobs to be set to true");
    }

    /*
     * 🔄 第六步：复制剩余参数 - 实现完全兼容
     * 
     * 这是整个函数的最后一步：把用户请求中所有剩余的参数
     * 直接复制到llama.cpp参数中。
     * 
     * 这个设计很巧妙：
     * 1. 优先处理特殊的OpenAI参数（上面已处理）
     * 2. 然后允许用户直接使用llama.cpp专有参数
     * 3. 实现了OpenAI API的完全兼容 + llama.cpp的扩展功能
     * 
     * 用户可以通过OpenAI接口使用llama.cpp专有功能，如：
     * - mirostat: 动态温度调节算法
     * - repeat_penalty: 重复惩罚系数
     * - tfs_z: 尾部自由采样参数
     * 等等...
     * 
     * 参考 "launch_slot_with_task()" 函数可以看到完整的支持参数列表
     */
    for (const auto & item : body.items()) {
        /*
         * 参数覆盖逻辑：
         * - 如果llama_params中没有这个参数，直接添加
         * - 特殊例外：n_predict参数始终以用户请求为准
         *   (n_predict是llama.cpp的原生参数，对应OpenAI的max_tokens)
         */
        if (!llama_params.contains(item.key()) || item.key() == "n_predict") {
            llama_params[item.key()] = item.value();
        }
    }

    /*
     * 🎉 转换完成：返回llama.cpp格式的完整参数对象
     * 
     * 这个返回的JSON包含了：
     * 1. 转换后的聊天参数（prompt、chat_format等）
     * 2. 处理后的生成参数（stop、temperature等）
     * 3. 用户指定的所有其他参数
     * 4. 多媒体文件数据（通过out_files参数输出）
     * 
     * 接下来这些参数会被传递给AI推理引擎进行文本生成
     */
    return llama_params;
}

static json format_embeddings_response_oaicompat(const json & request, const json & embeddings, bool use_base64 = false) {
    json data = json::array();
    int32_t n_tokens = 0;
    int i = 0;
    for (const auto & elem : embeddings) {
        json embedding_obj;

        if (use_base64) {
            const auto& vec = json_value(elem, "embedding", json::array()).get<std::vector<float>>();
            const char* data_ptr = reinterpret_cast<const char*>(vec.data());
            size_t data_size = vec.size() * sizeof(float);
            embedding_obj = {
                {"embedding", base64::encode(data_ptr, data_size)},
                {"index", i++},
                {"object", "embedding"},
                {"encoding_format", "base64"}
            };
        } else {
            embedding_obj = {
                {"embedding", json_value(elem, "embedding", json::array())},
                {"index", i++},
                {"object", "embedding"}
            };
        }
        data.push_back(embedding_obj);

        n_tokens += json_value(elem, "tokens_evaluated", 0);
    }

    json res = json {
        {"model", json_value(request, "model", std::string(DEFAULT_OAICOMPAT_MODEL))},
        {"object", "list"},
        {"usage", json {
            {"prompt_tokens", n_tokens},
            {"total_tokens", n_tokens}
        }},
        {"data", data}
    };

    return res;
}

static json format_response_rerank(
        const json & request,
        const json & ranks,
        bool is_tei_format,
        std::vector<std::string> & texts) {
    json res;
    if (is_tei_format) {
        // TEI response format
        res = json::array();
        bool return_text = json_value(request, "return_text", false);
        for (const auto & rank : ranks) {
            int index = json_value(rank, "index", 0);
            json elem = json{
                {"index", index},
                {"score", json_value(rank, "score", 0.0)},
            };
            if (return_text) {
                elem["text"] = std::move(texts[index]);
            }
            res.push_back(elem);
        }
    } else {
        // Jina response format
        json results = json::array();
        int32_t n_tokens = 0;
        for (const auto & rank : ranks) {
            results.push_back(json{
                {"index",           json_value(rank, "index", 0)},
                {"relevance_score", json_value(rank, "score", 0.0)},
            });

            n_tokens += json_value(rank, "tokens_evaluated", 0);
        }

        res = json{
            {"model", json_value(request, "model", std::string(DEFAULT_OAICOMPAT_MODEL))},
            {"object", "list"},
            {"usage", json{
                {"prompt_tokens", n_tokens},
                {"total_tokens", n_tokens}
            }},
            {"results", results}
        };
    }

    return res;
}

static bool is_valid_utf8(const std::string & str) {
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(str.data());
    const unsigned char* end = bytes + str.length();

    while (bytes < end) {
        if (*bytes <= 0x7F) {
            // 1-byte sequence (0xxxxxxx)
            bytes++;
        } else if ((*bytes & 0xE0) == 0xC0) {
            // 2-byte sequence (110xxxxx 10xxxxxx)
            if (end - bytes < 2 || (bytes[1] & 0xC0) != 0x80)
                return false;
            bytes += 2;
        } else if ((*bytes & 0xF0) == 0xE0) {
            // 3-byte sequence (1110xxxx 10xxxxxx 10xxxxxx)
            if (end - bytes < 3 || (bytes[1] & 0xC0) != 0x80 || (bytes[2] & 0xC0) != 0x80)
                return false;
            bytes += 3;
        } else if ((*bytes & 0xF8) == 0xF0) {
            // 4-byte sequence (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
            if (end - bytes < 4 || (bytes[1] & 0xC0) != 0x80 ||
                (bytes[2] & 0xC0) != 0x80 || (bytes[3] & 0xC0) != 0x80)
                return false;
            bytes += 4;
        } else {
            // Invalid UTF-8 lead byte
            return false;
        }
    }

    return true;
}

static json format_tokenizer_response(const json & tokens) {
    return json {
        {"tokens", tokens}
    };
}

static json format_detokenized_response(const std::string & content) {
    return json {
        {"content", content}
    };
}

static json format_logit_bias(const std::vector<llama_logit_bias> & logit_bias) {
    json data = json::array();
    for (const auto & lb : logit_bias) {
        data.push_back(json{
            {"bias", lb.bias},
            {"token", lb.token},
        });
    }
    return data;
}

static std::string safe_json_to_str(const json & data) {
    return data.dump(-1, ' ', false, json::error_handler_t::replace);
}

static std::vector<llama_token_data> get_token_probabilities(llama_context * ctx, int idx) {
    std::vector<llama_token_data> cur;
    const auto * logits = llama_get_logits_ith(ctx, idx);

    const llama_model * model = llama_get_model(ctx);
    const llama_vocab * vocab = llama_model_get_vocab(model);

    const int n_vocab = llama_vocab_n_tokens(vocab);

    cur.resize(n_vocab);
    for (llama_token token_id = 0; token_id < n_vocab; token_id++) {
        cur[token_id] = llama_token_data{token_id, logits[token_id], 0.0f};
    }

    // sort tokens by logits
    std::sort(cur.begin(), cur.end(), [](const llama_token_data & a, const llama_token_data & b) {
        return a.logit > b.logit;
    });

    // apply softmax
    float max_l = cur[0].logit;
    float cum_sum = 0.0f;
    for (size_t i = 0; i < cur.size(); ++i) {
        float p = expf(cur[i].logit - max_l);
        cur[i].p = p;
        cum_sum += p;
    }
    for (size_t i = 0; i < cur.size(); ++i) {
        cur[i].p /= cum_sum;
    }

    return cur;
}

static bool are_lora_equal(
        const std::vector<common_adapter_lora_info> & l1,
        const std::vector<common_adapter_lora_info> & l2) {
    if (l1.size() != l2.size()) {
        return false;
    }
    for (size_t i = 0; i < l1.size(); ++i) {
        // we don't check lora.path to reduce the time complexity
        if (l1[i].scale != l2[i].scale || l1[i].ptr != l2[i].ptr) {
            return false;
        }
    }
    return true;
}

// parse lora config from JSON request, returned a copy of lora_base with updated scale
static std::vector<common_adapter_lora_info> parse_lora_request(
        const std::vector<common_adapter_lora_info> & lora_base,
        const json & data) {
    std::vector<common_adapter_lora_info> lora(lora_base);
    int max_idx = lora.size();

    // clear existing value
    for (auto & entry : lora) {
        entry.scale = 0.0f;
    }

    // set value
    for (const auto & entry : data) {
        int id      = json_value(entry, "id", -1);
        float scale = json_value(entry, "scale", 0.0f);
        if (0 <= id && id < max_idx) {
            lora[id].scale = scale;
        } else {
            throw std::runtime_error("invalid adapter id");
        }
    }

    return lora;
}

//
// utils for interacting with libmtmd
// (may need to refactor in near future)
//

/**
 * server_tokens is a helper to manage the input tokens and image for the server.
 * it is made this way to simplify the logic of KV cache management.
 */
struct server_tokens {
    bool has_mtmd = false;

private: // disallow accessing these members directly, risking out-of-sync

    // map a **start** position in tokens to the image chunk
    std::unordered_map<llama_pos, mtmd::input_chunk_ptr> map_pos_to_media;

    // list of tokens
    // it can include LLAMA_TOKEN_NULL, which is used to indicate a token that is not a text token
    // a mtmd_input_chunk can occupy multiple tokens, one llama_token per **position**
    // important: for models using mrope, an image can contain multiple tokens but will use only one **position**
    llama_tokens tokens;

    // for ex. with input of 5 text tokens and 2 images:
    //      [0] [1] [2] [3] [4] [img0] [img0] [img0] [img1] [img1]
    // pos  0   1   2   3   4   5      6      7      8      9
    // map_pos_to_media will contain: {5, img0}, {8, img1}

public:
    server_tokens() = default;
    ~server_tokens() = default;

    // Prevent copying
    server_tokens(const server_tokens&) = delete;
    server_tokens& operator=(const server_tokens&) = delete;

    // Allow moving (usually implicitly generated if members are movable)
    server_tokens(server_tokens&&) = default;
    server_tokens& operator=(server_tokens&&) = default;

    // Allow accessing elements using [] operator
    llama_token operator[](size_t index) { return tokens[index]; }
    const llama_token& operator[](size_t index) const { return tokens[index]; }

    server_tokens(mtmd::input_chunks & mtmd_chunks, bool has_mtmd) : has_mtmd(has_mtmd) {
        for (size_t i = 0; i < mtmd_chunks.size(); ++i) {
            push_back(mtmd_chunks[i]);
        }
    }

    server_tokens(llama_tokens & tokens, bool has_mtmd) : has_mtmd(has_mtmd), tokens(tokens) {}

    // for debugging
    std::string str() const {
        std::ostringstream oss;
        oss << "tokens: ";
        for (const auto & t : tokens) {
            if (t == LLAMA_TOKEN_NULL) {
                oss << "<embd> ";
            } else {
                oss << t << " ";
            }
        }
        oss << "\n";
        oss << "image pos: ";
        for (const auto & it : map_pos_to_media) {
            oss << it.first << ", ";
        }
        return oss.str();
    }

    const mtmd::input_chunk_ptr & find_chunk(llama_pos pos) const {
        auto it = map_pos_to_media.find(pos);
        if (it != map_pos_to_media.end()) {
            return it->second;
        } else {
            throw std::runtime_error("Chunk not found");
        }
    }

    void push_back(llama_token tok) {
        if (tok == LLAMA_TOKEN_NULL) {
            throw std::runtime_error("Invalid token");
        }
        tokens.emplace_back(tok);
    }

    // will create a copy of the chunk if it contains non-text data
    void push_back(const mtmd_input_chunk * chunk) {
        auto type = mtmd_input_chunk_get_type(chunk);
        if (type == MTMD_INPUT_CHUNK_TYPE_IMAGE || type == MTMD_INPUT_CHUNK_TYPE_AUDIO) {
            GGML_ASSERT(has_mtmd);
            const int n_pos = mtmd_input_chunk_get_n_pos(chunk);
            llama_pos start_pos = tokens.size();
            for (int i = 0; i < n_pos; ++i) {
                tokens.emplace_back(LLAMA_TOKEN_NULL);
            }
            mtmd::input_chunk_ptr new_chunk(mtmd_input_chunk_copy(chunk));
            map_pos_to_media[start_pos] = std::move(new_chunk);
        } else if (type == MTMD_INPUT_CHUNK_TYPE_TEXT) {
            size_t n_tokens;
            auto text_tokens = mtmd_input_chunk_get_tokens_text(chunk, &n_tokens);
            for (size_t i = 0; i < n_tokens; ++i) {
                push_back(text_tokens[i]);
            }
        } else {
            GGML_ABORT("Invalid chunk type");
        }
    }

    // for compatibility with context shift and prompt truncation
    void insert(const llama_tokens & inp_tokens) {
        GGML_ASSERT(!has_mtmd); // only allow this if mtmd is disabled
        tokens.insert(tokens.end(), inp_tokens.begin(), inp_tokens.end());
    }

    // for compatibility with speculative decoding, ctx shift, slot save/load
    const llama_tokens & get_text_tokens() const {
        GGML_ASSERT(!has_mtmd); // only allow this if mtmd is disabled
        return tokens;
    }

    // for compatibility with speculative decoding
    void set_token(llama_pos pos, llama_token id) {
        GGML_ASSERT(!has_mtmd); // only allow this if mtmd is disabled
        tokens[pos] = id;
    }

    size_t size() const {
        return tokens.size();
    }

    bool empty() const {
        return tokens.empty();
    }

    void clear() {
        tokens.clear();
    }

    void keep_first(size_t n) {
        GGML_ASSERT(n <= tokens.size());
        if (has_mtmd) {
            if (n == tokens.size()) {
                return; // nothing to do
            }
            // we throw an error if we try to remove a token in the middle of an image
            // for ex. with input of 5 text tokens and 2 images:
            //    [0] [1] [2] [3] [4] [img0] [img0] [img0] [img1] [img1]
            // n  1   2   3   4   5   6      7      8      9      10
            // allowed to resize      ^                    ^
            // disallowed to resize          ^      ^             ^
            if (n > 0) {
                llama_token last_token = tokens[n - 1];
                // make sure we never remove tokens in the middle of an image
                if (last_token == LLAMA_TOKEN_NULL) {
                    find_chunk(n - 1); // will throw an error if the token is not begin-of-chunk
                }
            }
            // remove all image chunks that are not used anymore
            for (auto it = map_pos_to_media.begin(); it != map_pos_to_media.end(); ) {
                llama_pos pos = it->first;
                if (pos >= (llama_pos)n) {
                    it = map_pos_to_media.erase(it);
                } else {
                    ++it;
                }
            }
        }
        tokens.resize(n);
    }

    std::string detokenize(const llama_context * ctx, bool special) const {
        llama_tokens text_tokens;
        text_tokens.reserve(tokens.size());
        for (const auto & t : tokens) {
            if (t != LLAMA_TOKEN_NULL) {
                text_tokens.push_back(t);
            }
        }
        return common_detokenize(ctx, text_tokens, special);
    }

    size_t get_common_prefix(const server_tokens & b) const {
        size_t max_idx = std::min(tokens.size(), b.tokens.size());
        for (size_t i = 0; i < max_idx; ++i) {
            auto & ai =   tokens[i];
            auto & bi = b.tokens[i];

            if (ai == LLAMA_TOKEN_NULL && bi == LLAMA_TOKEN_NULL) {
                GGML_ASSERT(has_mtmd);
                const auto & a_chunk =   find_chunk(i);
                const auto & b_chunk = b.find_chunk(i);
                GGML_ASSERT(a_chunk && b_chunk);
                std::string ai_id  = mtmd_input_chunk_get_id(a_chunk.get());
                std::string bi_id  = mtmd_input_chunk_get_id(b_chunk.get());
                size_t a_pos       = mtmd_input_chunk_get_n_pos(a_chunk.get());
                size_t b_pos       = mtmd_input_chunk_get_n_pos(b_chunk.get());
                if (ai_id == bi_id && a_pos == b_pos) {
                    GGML_ASSERT(a_pos > 0 && "Invalid media chunk"); // should never happen
                    i += a_pos - 1; // will be +1 by the for loop
                    continue;
                } else {
                    return i;
                }
            } else if (ai == bi) {
                continue;
            } else {
                return i;
            }
        }
        return max_idx; // all tokens are equal
    }

    // make sure all text tokens are within the vocab range
    bool validate(const struct llama_context * ctx) const {
        const llama_model * model = llama_get_model(ctx);
        const llama_vocab * vocab = llama_model_get_vocab(model);
        const int32_t n_vocab = llama_vocab_n_tokens(vocab);

        for (size_t i = 0; i < tokens.size(); ++i) {
            auto & t = tokens[i];
            if (t == LLAMA_TOKEN_NULL) {
                try {
                    const auto & chunk = find_chunk(i);
                    size_t n_pos = mtmd_input_chunk_get_n_pos(chunk.get());
                    i += n_pos - 1; // will be +1 by the for loop
                } catch (const std::exception & e) {
                    return false;
                }
            } else if (t < 0 || t >= n_vocab) {
                return false;
            }
        }
        return true;
    }

    // encode and decode the image chunk
    int32_t process_chunk(
                llama_context * ctx,
                mtmd_context * mctx,
                llama_pos n_past,
                int32_t seq_id,
                llama_pos & n_pos_out) {
        auto & chunk = find_chunk(n_past);
        const char * name = mtmd_input_chunk_get_type(chunk.get()) == MTMD_INPUT_CHUNK_TYPE_IMAGE
                            ? "image" : "audio";
        SRV_INF("processing %s...\n", name);
        int32_t n_batch = llama_n_batch(ctx);
        int64_t t0 = ggml_time_ms();
        llama_pos new_n_past = n_past;
        int32_t result = mtmd_helper_eval_chunk_single(mctx, ctx,
            chunk.get(),
            n_past,
            seq_id,
            n_batch,
            true, // logits last
            &new_n_past);
        SRV_INF("%s processed in %" PRId64 " ms\n", name, ggml_time_ms() - t0);
        if (result != 0) {
            LOG_ERR("mtmd_helper_eval failed with status %d", result);
            n_pos_out = n_past;
            return result;
        }
        n_pos_out = new_n_past;
        return 0;
    }
};

// Computes FNV-1a hash of the data
static std::string fnv_hash(const uint8_t * data, size_t len) {
    const uint64_t fnv_prime = 0x100000001b3ULL;
    uint64_t hash = 0xcbf29ce484222325ULL;

    for (size_t i = 0; i < len; ++i) {
        hash ^= data[i];
        hash *= fnv_prime;
    }
    return std::to_string(hash);
}
