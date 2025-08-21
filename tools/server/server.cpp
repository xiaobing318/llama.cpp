#include "chat.h"
#include "utils.hpp"

#include "arg.h"
#include "common.h"
#include "json-schema-to-grammar.h"
#include "llama.h"
#include "log.h"
#include "sampling.h"
#include "speculative.h"
#include "mtmd.h"
#include "mtmd-helper.h"

// mime type for sending response
#define MIMETYPE_JSON "application/json; charset=utf-8"

// auto generated files (see README.md for details)
#include "index.html.gz.hpp"
#include "loading.html.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cinttypes>
#include <deque>
#include <memory>
#include <mutex>
#include <signal.h>
#include <thread>
#include <unordered_map>
#include <unordered_set>

using json = nlohmann::ordered_json;

constexpr int HTTP_POLLING_SECONDS = 1;

enum stop_type {
    STOP_TYPE_NONE,
    STOP_TYPE_EOS,
    STOP_TYPE_WORD,
    STOP_TYPE_LIMIT,
};

// state diagram: https://github.com/ggml-org/llama.cpp/pull/9283
enum slot_state {
    SLOT_STATE_IDLE,
    SLOT_STATE_STARTED, // TODO: this state is only used for setting up the initial prompt processing; maybe merge it with launch_slot_with_task in the future
    SLOT_STATE_PROCESSING_PROMPT,
    SLOT_STATE_DONE_PROMPT,
    SLOT_STATE_GENERATING,
};

enum server_state {
    SERVER_STATE_LOADING_MODEL,  // Server is starting up, model not fully loaded yet
    SERVER_STATE_READY,          // Server is ready and model is loaded
};

enum server_task_type {
    SERVER_TASK_TYPE_COMPLETION,
    SERVER_TASK_TYPE_EMBEDDING,
    SERVER_TASK_TYPE_RERANK,
    SERVER_TASK_TYPE_INFILL,
    SERVER_TASK_TYPE_CANCEL,
    SERVER_TASK_TYPE_NEXT_RESPONSE,
    SERVER_TASK_TYPE_METRICS,
    SERVER_TASK_TYPE_SLOT_SAVE,
    SERVER_TASK_TYPE_SLOT_RESTORE,
    SERVER_TASK_TYPE_SLOT_ERASE,
    SERVER_TASK_TYPE_SET_LORA,
};

enum oaicompat_type {
    OAICOMPAT_TYPE_NONE,
    OAICOMPAT_TYPE_CHAT,
    OAICOMPAT_TYPE_COMPLETION,
    OAICOMPAT_TYPE_EMBEDDING,
};

// https://community.openai.com/t/openai-chat-list-of-error-codes-and-types/357791/11
enum error_type {
    ERROR_TYPE_INVALID_REQUEST,
    ERROR_TYPE_AUTHENTICATION,
    ERROR_TYPE_SERVER,
    ERROR_TYPE_NOT_FOUND,
    ERROR_TYPE_PERMISSION,
    ERROR_TYPE_UNAVAILABLE, // custom error
    ERROR_TYPE_NOT_SUPPORTED, // custom error
};

static bool server_task_type_need_embd(server_task_type task_type) {
    switch (task_type) {
        case SERVER_TASK_TYPE_EMBEDDING:
        case SERVER_TASK_TYPE_RERANK:
            return true;
        default:
            return false;
    }
}

static bool server_task_type_need_logits(server_task_type task_type) {
    switch (task_type) {
        case SERVER_TASK_TYPE_COMPLETION:
        case SERVER_TASK_TYPE_INFILL:
            return true;
        default:
            return false;
    }
}

struct slot_params {
    bool stream        = true;
    bool cache_prompt  = true; // remember the prompt to avoid reprocessing all prompt
    bool return_tokens = false;

    int32_t n_keep    =  0; // number of tokens to keep from initial prompt
    int32_t n_discard =  0; // number of tokens after n_keep that may be discarded when shifting context, 0 defaults to half
    int32_t n_predict = -1; // new tokens to predict
    int32_t n_indent  =  0; // mininum line indentation for the generated text in number of whitespace characters

    int64_t t_max_prompt_ms  = -1; // TODO: implement
    int64_t t_max_predict_ms = -1; // if positive, limit the generation phase to this time limit

    std::vector<common_adapter_lora_info> lora;

    std::vector<std::string> antiprompt;
    std::vector<std::string> response_fields;
    bool timings_per_token = false;
    bool post_sampling_probs = false;

    struct common_params_sampling sampling;
    struct common_params_speculative speculative;

    // OAI-compat fields
    bool                         verbose                   = false;
    oaicompat_type               oaicompat                 = OAICOMPAT_TYPE_NONE;
    std::string                  oaicompat_model;
    std::string                  oaicompat_cmpl_id;
    common_chat_syntax           oaicompat_chat_syntax;

    // Embeddings
    int32_t embd_normalize = 2; // (-1=none, 0=max absolute int16, 1=taxicab, 2=Euclidean/L2, >2=p-norm)

    json to_json() const {
        std::vector<std::string> samplers;
        samplers.reserve(sampling.samplers.size());
        for (const auto & sampler : sampling.samplers) {
            samplers.emplace_back(common_sampler_type_to_str(sampler));
        }

        json lora = json::array();
        for (size_t i = 0; i < this->lora.size(); ++i) {
            lora.push_back({{"id", i}, {"scale", this->lora[i].scale}});
        }

        auto grammar_triggers = json::array();
        for (const auto & trigger : sampling.grammar_triggers) {
            server_grammar_trigger ct(std::move(trigger));
            grammar_triggers.push_back(ct.to_json());
        }

        return json {
            {"n_predict",                 n_predict},     // Server configured n_predict
            {"seed",                      sampling.seed},
            {"temperature",               sampling.temp},
            {"dynatemp_range",            sampling.dynatemp_range},
            {"dynatemp_exponent",         sampling.dynatemp_exponent},
            {"top_k",                     sampling.top_k},
            {"top_p",                     sampling.top_p},
            {"min_p",                     sampling.min_p},
            {"top_n_sigma",               sampling.top_n_sigma},
            {"xtc_probability",           sampling.xtc_probability},
            {"xtc_threshold",             sampling.xtc_threshold},
            {"typical_p",                 sampling.typ_p},
            {"repeat_last_n",             sampling.penalty_last_n},
            {"repeat_penalty",            sampling.penalty_repeat},
            {"presence_penalty",          sampling.penalty_present},
            {"frequency_penalty",         sampling.penalty_freq},
            {"dry_multiplier",            sampling.dry_multiplier},
            {"dry_base",                  sampling.dry_base},
            {"dry_allowed_length",        sampling.dry_allowed_length},
            {"dry_penalty_last_n",        sampling.dry_penalty_last_n},
            {"dry_sequence_breakers",     sampling.dry_sequence_breakers},
            {"mirostat",                  sampling.mirostat},
            {"mirostat_tau",              sampling.mirostat_tau},
            {"mirostat_eta",              sampling.mirostat_eta},
            {"stop",                      antiprompt},
            {"max_tokens",                n_predict}, // User configured n_predict
            {"n_keep",                    n_keep},
            {"n_discard",                 n_discard},
            {"ignore_eos",                sampling.ignore_eos},
            {"stream",                    stream},
            {"logit_bias",                format_logit_bias(sampling.logit_bias)},
            {"n_probs",                   sampling.n_probs},
            {"min_keep",                  sampling.min_keep},
            {"grammar",                   sampling.grammar},
            {"grammar_lazy",              sampling.grammar_lazy},
            {"grammar_triggers",          grammar_triggers},
            {"preserved_tokens",          sampling.preserved_tokens},
            {"chat_format",               common_chat_format_name(oaicompat_chat_syntax.format)},
            {"reasoning_format",          common_reasoning_format_name(oaicompat_chat_syntax.reasoning_format)},
            {"reasoning_in_content",      oaicompat_chat_syntax.reasoning_in_content},
            {"thinking_forced_open",      oaicompat_chat_syntax.thinking_forced_open},
            {"samplers",                  samplers},
            {"speculative.n_max",         speculative.n_max},
            {"speculative.n_min",         speculative.n_min},
            {"speculative.p_min",         speculative.p_min},
            {"timings_per_token",         timings_per_token},
            {"post_sampling_probs",       post_sampling_probs},
            {"lora",                      lora},
        };
    }
};

struct server_task {
    int id    = -1; // to be filled by server_queue
    int index = -1; // used when there are multiple prompts (batch request)

    server_task_type type;

    // used by SERVER_TASK_TYPE_CANCEL
    int id_target = -1;

    // used by SERVER_TASK_TYPE_INFERENCE
    slot_params   params;
    server_tokens prompt_tokens;
    int id_selected_slot = -1;

    // used by SERVER_TASK_TYPE_SLOT_SAVE, SERVER_TASK_TYPE_SLOT_RESTORE, SERVER_TASK_TYPE_SLOT_ERASE
    struct slot_action {
        int slot_id;
        std::string filename;
        std::string filepath;
    };
    slot_action slot_action;

    // used by SERVER_TASK_TYPE_METRICS
    bool metrics_reset_bucket = false;

    // used by SERVER_TASK_TYPE_SET_LORA
    std::vector<common_adapter_lora_info> set_lora;

    server_task(server_task_type type) : type(type) {}

    static slot_params params_from_json_cmpl(
            const llama_context * ctx,
            const common_params & params_base,
            const json & data) {
        const llama_model * model = llama_get_model(ctx);
        const llama_vocab * vocab = llama_model_get_vocab(model);

        slot_params params;

        // Sampling parameter defaults are loaded from the global server context (but individual requests can still override them)
        slot_params defaults;
        defaults.sampling    = params_base.sampling;
        defaults.speculative = params_base.speculative;
        defaults.n_keep      = params_base.n_keep;
        defaults.antiprompt  = params_base.antiprompt;

        // enabling this will output extra debug information in the HTTP responses from the server
        params.verbose           = params_base.verbosity > 9;
        params.timings_per_token = json_value(data, "timings_per_token", false);

        params.stream           = json_value(data, "stream",             false);
        params.cache_prompt     = json_value(data, "cache_prompt",       true);
        params.return_tokens    = json_value(data, "return_tokens",      false);
        params.n_predict        = json_value(data, "n_predict",          json_value(data, "max_tokens", defaults.n_predict));
        params.n_indent         = json_value(data, "n_indent",           defaults.n_indent);
        params.n_keep           = json_value(data, "n_keep",             defaults.n_keep);
        params.n_discard        = json_value(data, "n_discard",          defaults.n_discard);
      //params.t_max_prompt_ms  = json_value(data, "t_max_prompt_ms",    defaults.t_max_prompt_ms); // TODO: implement
        params.t_max_predict_ms = json_value(data, "t_max_predict_ms",   defaults.t_max_predict_ms);
        params.response_fields  = json_value(data, "response_fields",   std::vector<std::string>());

        params.sampling.top_k              = json_value(data, "top_k",              defaults.sampling.top_k);
        params.sampling.top_p              = json_value(data, "top_p",              defaults.sampling.top_p);
        params.sampling.min_p              = json_value(data, "min_p",              defaults.sampling.min_p);
        params.sampling.top_n_sigma        = json_value(data, "top_n_sigma",        defaults.sampling.top_n_sigma);
        params.sampling.xtc_probability    = json_value(data, "xtc_probability",    defaults.sampling.xtc_probability);
        params.sampling.xtc_threshold      = json_value(data, "xtc_threshold",      defaults.sampling.xtc_threshold);
        params.sampling.typ_p              = json_value(data, "typical_p",          defaults.sampling.typ_p);
        params.sampling.temp               = json_value(data, "temperature",        defaults.sampling.temp);
        params.sampling.dynatemp_range     = json_value(data, "dynatemp_range",     defaults.sampling.dynatemp_range);
        params.sampling.dynatemp_exponent  = json_value(data, "dynatemp_exponent",  defaults.sampling.dynatemp_exponent);
        params.sampling.penalty_last_n     = json_value(data, "repeat_last_n",      defaults.sampling.penalty_last_n);
        params.sampling.penalty_repeat     = json_value(data, "repeat_penalty",     defaults.sampling.penalty_repeat);
        params.sampling.penalty_freq       = json_value(data, "frequency_penalty",  defaults.sampling.penalty_freq);
        params.sampling.penalty_present    = json_value(data, "presence_penalty",   defaults.sampling.penalty_present);
        params.sampling.dry_multiplier     = json_value(data, "dry_multiplier",     defaults.sampling.dry_multiplier);
        params.sampling.dry_base           = json_value(data, "dry_base",           defaults.sampling.dry_base);
        params.sampling.dry_allowed_length = json_value(data, "dry_allowed_length", defaults.sampling.dry_allowed_length);
        params.sampling.dry_penalty_last_n = json_value(data, "dry_penalty_last_n", defaults.sampling.dry_penalty_last_n);
        params.sampling.mirostat           = json_value(data, "mirostat",           defaults.sampling.mirostat);
        params.sampling.mirostat_tau       = json_value(data, "mirostat_tau",       defaults.sampling.mirostat_tau);
        params.sampling.mirostat_eta       = json_value(data, "mirostat_eta",       defaults.sampling.mirostat_eta);
        params.sampling.seed               = json_value(data, "seed",               defaults.sampling.seed);
        params.sampling.n_probs            = json_value(data, "n_probs",            defaults.sampling.n_probs);
        params.sampling.min_keep           = json_value(data, "min_keep",           defaults.sampling.min_keep);
        params.post_sampling_probs         = json_value(data, "post_sampling_probs", defaults.post_sampling_probs);

        params.speculative.n_min = json_value(data, "speculative.n_min", defaults.speculative.n_min);
        params.speculative.n_max = json_value(data, "speculative.n_max", defaults.speculative.n_max);
        params.speculative.p_min = json_value(data, "speculative.p_min", defaults.speculative.p_min);

        params.speculative.n_min = std::min(params.speculative.n_max, params.speculative.n_min);
        params.speculative.n_min = std::max(params.speculative.n_min, 0);
        params.speculative.n_max = std::max(params.speculative.n_max, 0);

        // Use OpenAI API logprobs only if n_probs wasn't provided
        if (data.contains("logprobs") && params.sampling.n_probs == defaults.sampling.n_probs){
            params.sampling.n_probs = json_value(data, "logprobs", defaults.sampling.n_probs);
        }

        if (data.contains("lora")) {
            if (data.at("lora").is_array()) {
                params.lora = parse_lora_request(params_base.lora_adapters, data.at("lora"));
            } else {
                throw std::runtime_error("Error: 'lora' must be an array of objects with 'id' and 'scale' fields");
            }
        } else {
            params.lora = params_base.lora_adapters;
        }

        // TODO: add more sanity checks for the input parameters

        if (params.sampling.penalty_last_n < -1) {
            throw std::runtime_error("Error: repeat_last_n must be >= -1");
        }

        if (params.sampling.dry_penalty_last_n < -1) {
            throw std::runtime_error("Error: dry_penalty_last_n must be >= -1");
        }

        if (params.sampling.penalty_last_n == -1) {
            // note: should be the slot's context and not the full context, but it's ok
            params.sampling.penalty_last_n = llama_n_ctx(ctx);
        }

        if (params.sampling.dry_penalty_last_n == -1) {
            params.sampling.dry_penalty_last_n = llama_n_ctx(ctx);
        }

        if (params.sampling.dry_base < 1.0f) {
            params.sampling.dry_base = defaults.sampling.dry_base;
        }

        // sequence breakers for DRY
        {
            // Currently, this is not compatible with TextGen WebUI, Koboldcpp and SillyTavern format
            // Ref: https://github.com/oobabooga/text-generation-webui/blob/d1af7a41ade7bd3c3a463bfa640725edb818ebaf/extensions/openai/typing.py#L39

            if (data.contains("dry_sequence_breakers")) {
                params.sampling.dry_sequence_breakers = json_value(data, "dry_sequence_breakers", std::vector<std::string>());
                if (params.sampling.dry_sequence_breakers.empty()) {
                    throw std::runtime_error("Error: dry_sequence_breakers must be a non-empty array of strings");
                }
            }
        }

        // process "json_schema" and "grammar"
        if (data.contains("json_schema") && !data.contains("grammar")) {
            try {
                auto schema                  = json_value(data, "json_schema", json::object());
                SRV_DBG("JSON schema: %s\n", schema.dump(2).c_str());
                params.sampling.grammar      = json_schema_to_grammar(schema);
                SRV_DBG("Converted grammar: %s\n", params.sampling.grammar.c_str());
            } catch (const std::exception & e) {
                throw std::runtime_error(std::string("\"json_schema\": ") + e.what());
            }
        } else {
            params.sampling.grammar      = json_value(data, "grammar", defaults.sampling.grammar);
            SRV_DBG("Grammar: %s\n", params.sampling.grammar.c_str());
            params.sampling.grammar_lazy = json_value(data, "grammar_lazy", defaults.sampling.grammar_lazy);
            SRV_DBG("Grammar lazy: %s\n", params.sampling.grammar_lazy ? "true" : "false");
        }

        {
            auto it = data.find("chat_format");
            if (it != data.end()) {
                params.oaicompat_chat_syntax.format = static_cast<common_chat_format>(it->get<int>());
                SRV_INF("Chat format: %s\n", common_chat_format_name(params.oaicompat_chat_syntax.format));
            } else {
                params.oaicompat_chat_syntax.format = defaults.oaicompat_chat_syntax.format;
            }
            common_reasoning_format reasoning_format = params_base.reasoning_format;
            if (data.contains("reasoning_format")) {
                reasoning_format = common_reasoning_format_from_name(data.at("reasoning_format").get<std::string>());
            }
            params.oaicompat_chat_syntax.reasoning_format = reasoning_format;
            params.oaicompat_chat_syntax.reasoning_in_content = params.stream && (reasoning_format == COMMON_REASONING_FORMAT_DEEPSEEK_LEGACY);
            params.oaicompat_chat_syntax.thinking_forced_open = json_value(data, "thinking_forced_open", false);
            params.oaicompat_chat_syntax.parse_tool_calls = json_value(data, "parse_tool_calls", false);
        }

        {
            const auto preserved_tokens = data.find("preserved_tokens");
            if (preserved_tokens != data.end()) {
                for (const auto & t : *preserved_tokens) {
                    auto ids = common_tokenize(vocab, t.get<std::string>(), /* add_special= */ false, /* parse_special= */ true);
                    if (ids.size() == 1) {
                        SRV_DBG("Preserved token: %d\n", ids[0]);
                        params.sampling.preserved_tokens.insert(ids[0]);
                    } else {
                        // This may happen when using a tool call style meant for a model with special tokens to preserve on a model without said tokens.
                        SRV_DBG("Not preserved because more than 1 token: %s\n", t.get<std::string>().c_str());
                    }
                }
            }
            const auto grammar_triggers = data.find("grammar_triggers");
            if (grammar_triggers != data.end()) {
                for (const auto & t : *grammar_triggers) {
                    server_grammar_trigger ct(t);
                    if (ct.value.type == COMMON_GRAMMAR_TRIGGER_TYPE_WORD) {
                        const auto & word = ct.value.value;
                        auto ids = common_tokenize(vocab, word, /* add_special= */ false, /* parse_special= */ true);
                        if (ids.size() == 1) {
                            auto token = ids[0];
                            if (std::find(params.sampling.preserved_tokens.begin(), params.sampling.preserved_tokens.end(), (llama_token) token) == params.sampling.preserved_tokens.end()) {
                                throw std::runtime_error("Grammar trigger word should be marked as preserved token: " + word);
                            }
                            SRV_DBG("Grammar trigger token: %d (`%s`)\n", token, word.c_str());
                            common_grammar_trigger trigger;
                            trigger.type = COMMON_GRAMMAR_TRIGGER_TYPE_TOKEN;
                            trigger.value = word;
                            trigger.token = token;
                            params.sampling.grammar_triggers.push_back(std::move(trigger));
                        } else {
                            SRV_DBG("Grammar trigger word: `%s`\n", word.c_str());
                            params.sampling.grammar_triggers.push_back({COMMON_GRAMMAR_TRIGGER_TYPE_WORD, word});
                        }
                    } else {
                        if (ct.value.type == COMMON_GRAMMAR_TRIGGER_TYPE_PATTERN) {
                            SRV_DBG("Grammar trigger pattern: `%s`\n", ct.value.value.c_str());
                        } else if (ct.value.type == COMMON_GRAMMAR_TRIGGER_TYPE_PATTERN_FULL) {
                            SRV_DBG("Grammar trigger pattern full: `%s`\n", ct.value.value.c_str());
                        } else {
                            throw std::runtime_error("Unknown grammar trigger type");
                        }
                        params.sampling.grammar_triggers.emplace_back(std::move(ct.value));
                    }
                }
            }
            if (params.sampling.grammar_lazy && params.sampling.grammar_triggers.empty()) {
                throw std::runtime_error("Error: no triggers set for lazy grammar!");
            }
        }

        {
            params.sampling.logit_bias.clear();

            const auto & logit_bias = data.find("logit_bias");
            if (logit_bias != data.end() && logit_bias->is_array()) {
                const int n_vocab = llama_vocab_n_tokens(vocab);
                for (const auto & el : *logit_bias) {
                    // TODO: we may want to throw errors here, in case "el" is incorrect
                    if (el.is_array() && el.size() == 2) {
                        float bias;
                        if (el[1].is_number()) {
                            bias = el[1].get<float>();
                        } else if (el[1].is_boolean() && !el[1].get<bool>()) {
                            bias = -INFINITY;
                        } else {
                            continue;
                        }

                        if (el[0].is_number_integer()) {
                            llama_token tok = el[0].get<llama_token>();
                            if (tok >= 0 && tok < n_vocab) {
                                params.sampling.logit_bias.push_back({tok, bias});
                            }
                        } else if (el[0].is_string()) {
                            auto toks = common_tokenize(vocab, el[0].get<std::string>(), false);
                            for (auto tok : toks) {
                                params.sampling.logit_bias.push_back({tok, bias});
                            }
                        }
                    }
                }
           } else if (logit_bias != data.end() && logit_bias->is_object()) {
                const int n_vocab = llama_vocab_n_tokens(vocab);
                for (const auto & el : logit_bias->items()) {
                    float bias;
                    const auto & key = el.key();
                    const auto & value = el.value();
                    if (value.is_number()) {
                        bias = value.get<float>();
                    } else if (value.is_boolean() && !value.get<bool>()) {
                        bias = -INFINITY;
                    } else {
                        continue;
                    }

                    char *end;
                    llama_token tok = strtol(key.c_str(), &end, 10);
                    if (*end == 0) {
                        if (tok >= 0 && tok < n_vocab) {
                            params.sampling.logit_bias.push_back({tok, bias});
                        }
                    } else {
                        auto toks = common_tokenize(vocab, key, false);
                        for (auto tok : toks) {
                            params.sampling.logit_bias.push_back({tok, bias});
                        }
                    }
                }
            }

            params.sampling.ignore_eos = json_value(data, "ignore_eos", params_base.sampling.ignore_eos);
            if (params.sampling.ignore_eos) {
                params.sampling.logit_bias.insert(
                        params.sampling.logit_bias.end(),
                        defaults.sampling.logit_bias_eog.begin(), defaults.sampling.logit_bias_eog.end());
            }
        }

        {
            params.antiprompt.clear();

            const auto & stop = data.find("stop");
            if (stop != data.end() && stop->is_array()) {
                for (const auto & word : *stop) {
                    if (!word.empty()) {
                        params.antiprompt.push_back(word);
                    }
                }
            }
            // set reverse prompt from cli args if not set in the request
            if (params.antiprompt.empty()) {
                params.antiprompt = defaults.antiprompt;
            }
        }

        {
            const auto samplers = data.find("samplers");
            if (samplers != data.end()) {
                if (samplers->is_array()) {
                    params.sampling.samplers = common_sampler_types_from_names(*samplers, false);
                } else if (samplers->is_string()){
                    params.sampling.samplers = common_sampler_types_from_chars(samplers->get<std::string>());
                }
            } else {
                params.sampling.samplers = defaults.sampling.samplers;
            }
        }

        std::string model_name = params_base.model_alias.empty() ? DEFAULT_OAICOMPAT_MODEL : params_base.model_alias;
        params.oaicompat_model = json_value(data, "model", model_name);

        return params;
    }

    // utility function
    static std::unordered_set<int> get_list_id(const std::vector<server_task> & tasks) {
        std::unordered_set<int> ids(tasks.size());
        for (size_t i = 0; i < tasks.size(); i++) {
            ids.insert(tasks[i].id);
        }
        return ids;
    }
};

struct result_timings {
    int32_t prompt_n = -1;
    double prompt_ms;
    double prompt_per_token_ms;
    double prompt_per_second;

    int32_t predicted_n = -1;
    double predicted_ms;
    double predicted_per_token_ms;
    double predicted_per_second;

    // Optional speculative metrics - only included when > 0
    int32_t draft_n = 0;
    int32_t draft_n_accepted = 0;

    json to_json() const {
        json base = {
            {"prompt_n",               prompt_n},
            {"prompt_ms",              prompt_ms},
            {"prompt_per_token_ms",    prompt_per_token_ms},
            {"prompt_per_second",      prompt_per_second},

            {"predicted_n",            predicted_n},
            {"predicted_ms",           predicted_ms},
            {"predicted_per_token_ms", predicted_per_token_ms},
            {"predicted_per_second",   predicted_per_second},
        };

        if (draft_n > 0) {
            base["draft_n"] = draft_n;
            base["draft_n_accepted"] = draft_n_accepted;
        }

        return base;
    }
};

struct server_task_result {
    int id           = -1;
    int id_slot      = -1;
    virtual bool is_error() {
        // only used by server_task_result_error
        return false;
    }
    virtual bool is_stop() {
        // only used by server_task_result_cmpl_*
        return false;
    }
    virtual int get_index() {
        return -1;
    }
    virtual json to_json() = 0;
    virtual ~server_task_result() = default;
};

// using shared_ptr for polymorphism of server_task_result
using server_task_result_ptr = std::unique_ptr<server_task_result>;

inline std::string stop_type_to_str(stop_type type) {
    switch (type) {
        case STOP_TYPE_EOS:   return "eos";
        case STOP_TYPE_WORD:  return "word";
        case STOP_TYPE_LIMIT: return "limit";
        default:              return "none";
    }
}

struct completion_token_output {
    llama_token tok;
    float prob;
    std::string text_to_send;
    struct prob_info {
        llama_token tok;
        std::string txt;
        float prob;
    };
    std::vector<prob_info> probs;

    json to_json(bool post_sampling_probs) const {
        json probs_for_token = json::array();
        for (const auto & p : probs) {
            std::string txt(p.txt);
            txt.resize(validate_utf8(txt));
            probs_for_token.push_back(json {
                {"id",      p.tok},
                {"token",   txt},
                {"bytes",   str_to_bytes(p.txt)},
                {
                    post_sampling_probs ? "prob" : "logprob",
                    post_sampling_probs ? p.prob : logarithm(p.prob)
                },
            });
        }
        return probs_for_token;
    }

    static json probs_vector_to_json(const std::vector<completion_token_output> & probs, bool post_sampling_probs) {
        json out = json::array();
        for (const auto & p : probs) {
            std::string txt(p.text_to_send);
            txt.resize(validate_utf8(txt));
            out.push_back(json {
                {"id",           p.tok},
                {"token",        txt},
                {"bytes",        str_to_bytes(p.text_to_send)},
                {
                    post_sampling_probs ? "prob" : "logprob",
                    post_sampling_probs ? p.prob : logarithm(p.prob)
                },
                {
                    post_sampling_probs ? "top_probs" : "top_logprobs",
                    p.to_json(post_sampling_probs)
                },
            });
        }
        return out;
    }

    static float logarithm(float x) {
        // nlohmann::json converts -inf to null, so we need to prevent that
        return x == 0.0f ? std::numeric_limits<float>::lowest() : std::log(x);
    }

    static std::vector<unsigned char> str_to_bytes(const std::string & str) {
        std::vector<unsigned char> bytes;
        for (unsigned char c : str) {
            bytes.push_back(c);
        }
        return bytes;
    }
};

struct swa_checkpoint {
    llama_pos pos_min;
    llama_pos pos_max;

    std::vector<uint8_t> data;
};

struct server_task_result_cmpl_final : server_task_result {
    int index = 0;

    std::string content;
    llama_tokens tokens;

    bool stream;
    result_timings timings;
    std::string prompt;

    bool truncated;
    int32_t n_decoded;
    int32_t n_prompt_tokens;
    int32_t n_tokens_cached;
    bool has_new_line;
    std::string stopping_word;
    stop_type stop = STOP_TYPE_NONE;

    bool post_sampling_probs;
    std::vector<completion_token_output> probs_output;
    std::vector<std::string>  response_fields;

    slot_params generation_params;

    // OAI-compat fields
    bool               verbose                  = false;
    oaicompat_type     oaicompat                = OAICOMPAT_TYPE_NONE;
    std::string        oaicompat_model;
    std::string        oaicompat_cmpl_id;
    common_chat_msg    oaicompat_msg;
    std::vector<common_chat_msg_diff> oaicompat_msg_diffs;

    virtual int get_index() override {
        return index;
    }

    virtual bool is_stop() override {
        return true; // in stream mode, final responses are considered stop
    }

    virtual json to_json() override {
        switch (oaicompat) {
            case OAICOMPAT_TYPE_NONE:
                return to_json_non_oaicompat();
            case OAICOMPAT_TYPE_COMPLETION:
                return to_json_oaicompat();
            case OAICOMPAT_TYPE_CHAT:
                return stream ? to_json_oaicompat_chat_stream() : to_json_oaicompat_chat();
            default:
                GGML_ASSERT(false && "Invalid oaicompat_type");
        }
    }

    json to_json_non_oaicompat() {
        json res = json {
            {"index",               index},
            {"content",             stream ? "" : content}, // in stream mode, content is already in last partial chunk
            {"tokens",              stream ? llama_tokens {} : tokens},
            {"id_slot",             id_slot},
            {"stop",                true},
            {"model",               oaicompat_model},
            {"tokens_predicted",    n_decoded},
            {"tokens_evaluated",    n_prompt_tokens},
            {"generation_settings", generation_params.to_json()},
            {"prompt",              prompt},
            {"has_new_line",        has_new_line},
            {"truncated",           truncated},
            {"stop_type",           stop_type_to_str(stop)},
            {"stopping_word",       stopping_word},
            {"tokens_cached",       n_tokens_cached},
            {"timings",             timings.to_json()},
        };
        if (!stream && !probs_output.empty()) {
            res["completion_probabilities"] = completion_token_output::probs_vector_to_json(probs_output, post_sampling_probs);
        }
        return response_fields.empty() ? res : json_get_nested_values(response_fields, res);
    }

    json to_json_oaicompat() {
        std::time_t t = std::time(0);
        json logprobs = json(nullptr); // OAI default to null
        if (!stream && probs_output.size() > 0) {
            logprobs = json{
                {"content", completion_token_output::probs_vector_to_json(probs_output, post_sampling_probs)},
            };
        }
        json finish_reason = "length";
        if (stop == STOP_TYPE_WORD || stop == STOP_TYPE_EOS) {
            finish_reason = "stop";
        }
        json res = json {
            {"choices",            json::array({
                json{
                    {"text",          stream ? "" : content}, // in stream mode, content is already in last partial chunk
                    {"index",         index},
                    {"logprobs",      logprobs},
                    {"finish_reason", finish_reason},
                }
            })},
            {"created",            t},
            {"model",              oaicompat_model},
            {"system_fingerprint", build_info},
            {"object",             "text_completion"},
            {"usage", json {
                {"completion_tokens", n_decoded},
                {"prompt_tokens",     n_prompt_tokens},
                {"total_tokens",      n_decoded + n_prompt_tokens}
            }},
            {"id", oaicompat_cmpl_id}
        };

        // extra fields for debugging purposes
        if (verbose) {
            res["__verbose"] = to_json_non_oaicompat();
        }
        if (timings.prompt_n >= 0) {
            res.push_back({"timings", timings.to_json()});
        }

        return res;
    }

    json to_json_oaicompat_chat() {
        std::string finish_reason = "length";
        common_chat_msg msg;
        if (!oaicompat_msg.empty()) {
            msg = oaicompat_msg;
        } else {
            msg.role = "assistant";
            msg.content = content;
        }
        if (stop == STOP_TYPE_WORD || stop == STOP_TYPE_EOS) {
            finish_reason = msg.tool_calls.empty() ? "stop" : "tool_calls";
        }

        json choice {
            {"finish_reason", finish_reason},
            {"index", 0},
            {"message", msg.to_json_oaicompat<json>()},
        };

        if (!stream && probs_output.size() > 0) {
            choice["logprobs"] = json{
                {"content", completion_token_output::probs_vector_to_json(probs_output, post_sampling_probs)},
            };
        }

        std::time_t t = std::time(0);

        json res = json {
            {"choices",            json::array({choice})},
            {"created",            t},
            {"model",              oaicompat_model},
            {"system_fingerprint", build_info},
            {"object",             "chat.completion"},
            {"usage", json {
                {"completion_tokens", n_decoded},
                {"prompt_tokens",     n_prompt_tokens},
                {"total_tokens",      n_decoded + n_prompt_tokens}
            }},
            {"id", oaicompat_cmpl_id}
        };

        // extra fields for debugging purposes
        if (verbose) {
            res["__verbose"] = to_json_non_oaicompat();
        }
        if (timings.prompt_n >= 0) {
            res.push_back({"timings", timings.to_json()});
        }

        return res;
    }

    json to_json_oaicompat_chat_stream() {
        std::time_t t = std::time(0);
        std::string finish_reason = "length";
        if (stop == STOP_TYPE_WORD || stop == STOP_TYPE_EOS) {
            finish_reason = oaicompat_msg.tool_calls.empty() ? "stop" : "tool_calls";
        }

        json deltas = json::array();
        for (const auto & diff : oaicompat_msg_diffs) {
            deltas.push_back({
                {"choices", json::array({
                    json {
                        {"finish_reason", nullptr},
                        {"index", 0},
                        {"delta", common_chat_msg_diff_to_json_oaicompat<json>(diff)},
                    },
                })},
                {"created", t},
                {"id", oaicompat_cmpl_id},
                {"model", oaicompat_model},
                {"system_fingerprint", build_info},
                {"object", "chat.completion.chunk"},
            });
        }

        deltas.push_back({
            {"choices", json::array({
                json {
                    {"finish_reason", finish_reason},
                    {"index", 0},
                    {"delta", json::object()},
                },
            })},
            {"created",            t},
            {"id",                 oaicompat_cmpl_id},
            {"model",              oaicompat_model},
            {"system_fingerprint", build_info},
            {"object",             "chat.completion.chunk"},
            {"usage", json {
                {"completion_tokens", n_decoded},
                {"prompt_tokens",     n_prompt_tokens},
                {"total_tokens",      n_decoded + n_prompt_tokens},
            }},
        });

        if (timings.prompt_n >= 0) {
            deltas.back().push_back({"timings", timings.to_json()});
        }

        // extra fields for debugging purposes
        if (verbose && !deltas.empty()) {
            deltas.front()["__verbose"] = to_json_non_oaicompat();
        }

        return deltas;
    }
};

struct server_task_result_cmpl_partial : server_task_result {
    int index = 0;

    std::string  content;
    llama_tokens tokens;

    int32_t n_decoded;
    int32_t n_prompt_tokens;

    bool post_sampling_probs;
    completion_token_output prob_output;
    result_timings timings;

    // OAI-compat fields
    bool            verbose   = false;
    oaicompat_type  oaicompat = OAICOMPAT_TYPE_NONE;
    std::string     oaicompat_model;
    std::string     oaicompat_cmpl_id;
    std::vector<common_chat_msg_diff> oaicompat_msg_diffs;

    virtual int get_index() override {
        return index;
    }

    virtual bool is_stop() override {
        return false; // in stream mode, partial responses are not considered stop
    }

    virtual json to_json() override {
        switch (oaicompat) {
            case OAICOMPAT_TYPE_NONE:
                return to_json_non_oaicompat();
            case OAICOMPAT_TYPE_COMPLETION:
                return to_json_oaicompat();
            case OAICOMPAT_TYPE_CHAT:
                return to_json_oaicompat_chat();
            default:
                GGML_ASSERT(false && "Invalid oaicompat_type");
        }
    }

    json to_json_non_oaicompat() {
        // non-OAI-compat JSON
        json res = json {
            {"index",            index},
            {"content",          content},
            {"tokens",           tokens},
            {"stop",             false},
            {"id_slot",          id_slot},
            {"tokens_predicted", n_decoded},
            {"tokens_evaluated", n_prompt_tokens},
        };
        // populate the timings object when needed (usually for the last response or with timings_per_token enabled)
        if (timings.prompt_n > 0) {
            res.push_back({"timings", timings.to_json()});
        }
        if (!prob_output.probs.empty()) {
            res["completion_probabilities"] = completion_token_output::probs_vector_to_json({prob_output}, post_sampling_probs);
        }
        return res;
    }

    json to_json_oaicompat() {
        std::time_t t = std::time(0);
        json logprobs = json(nullptr); // OAI default to null
        if (prob_output.probs.size() > 0) {
            logprobs = json{
                {"content", completion_token_output::probs_vector_to_json({prob_output}, post_sampling_probs)},
            };
        }
        json res = json {
            {"choices",            json::array({
                json{
                    {"text",          content},
                    {"index",         index},
                    {"logprobs",      logprobs},
                    {"finish_reason", nullptr},
                }
            })},
            {"created",            t},
            {"model",              oaicompat_model},
            {"system_fingerprint", build_info},
            {"object",             "text_completion"},
            {"id",                 oaicompat_cmpl_id}
        };

        // extra fields for debugging purposes
        if (verbose) {
            res["__verbose"] = to_json_non_oaicompat();
        }
        if (timings.prompt_n >= 0) {
            res.push_back({"timings", timings.to_json()});
        }

        return res;
    }

    json to_json_oaicompat_chat() {
        bool first = n_decoded == 1;
        std::time_t t = std::time(0);
        json choices;

        std::vector<json> deltas;
        auto add_delta = [&](const json & delta) {
            deltas.push_back({
                {"choices", json::array({
                    json {
                        {"finish_reason", nullptr},
                        {"index", 0},
                        {"delta", delta},
                    },
                })},
                {"created", t},
                {"id", oaicompat_cmpl_id},
                {"model", oaicompat_model},
                {"system_fingerprint", build_info},
                {"object", "chat.completion.chunk"},
            });
        };
        // We have to send an initial update to conform to openai behavior
        if (first) {
            add_delta({
                {"role", "assistant"},
                {"content", nullptr},
            });
        }

        for (const auto & diff : oaicompat_msg_diffs) {
            add_delta(common_chat_msg_diff_to_json_oaicompat<json>(diff));
        }

        if (!deltas.empty()) {
            GGML_ASSERT(deltas[deltas.size() - 1].at("choices").size() >= 1);

            if (prob_output.probs.size() > 0) {
                deltas[deltas.size() - 1].at("choices").at(0)["logprobs"] = json {
                    {"content", completion_token_output::probs_vector_to_json({prob_output}, post_sampling_probs)},
                };
            }

            if (timings.prompt_n >= 0) {
                deltas[deltas.size() - 1].push_back({"timings", timings.to_json()});
            }
        }

        return deltas;
    }
};

struct server_task_result_embd : server_task_result {
    int index = 0;
    std::vector<std::vector<float>> embedding;

    int32_t n_tokens;

    // OAI-compat fields
    oaicompat_type oaicompat = OAICOMPAT_TYPE_NONE;

    virtual int get_index() override {
        return index;
    }

    virtual json to_json() override {
        return oaicompat == OAICOMPAT_TYPE_EMBEDDING
            ? to_json_oaicompat()
            : to_json_non_oaicompat();
    }

    json to_json_non_oaicompat() {
        return json {
            {"index",     index},
            {"embedding", embedding},
        };
    }

    json to_json_oaicompat() {
        return json {
            {"index",            index},
            {"embedding",        embedding[0]},
            {"tokens_evaluated", n_tokens},
        };
    }
};

struct server_task_result_rerank : server_task_result {
    int index = 0;
    float score = -1e6;

    int32_t n_tokens;

    virtual int get_index() override {
        return index;
    }

    virtual json to_json() override {
        return json {
            {"index",            index},
            {"score",            score},
            {"tokens_evaluated", n_tokens},
        };
    }
};

// this function maybe used outside of server_task_result_error
static json format_error_response(const std::string & message, const enum error_type type) {
    std::string type_str;
    int code = 500;
    switch (type) {
        case ERROR_TYPE_INVALID_REQUEST:
            type_str = "invalid_request_error";
            code = 400;
            break;
        case ERROR_TYPE_AUTHENTICATION:
            type_str = "authentication_error";
            code = 401;
            break;
        case ERROR_TYPE_NOT_FOUND:
            type_str = "not_found_error";
            code = 404;
            break;
        case ERROR_TYPE_SERVER:
            type_str = "server_error";
            code = 500;
            break;
        case ERROR_TYPE_PERMISSION:
            type_str = "permission_error";
            code = 403;
            break;
        case ERROR_TYPE_NOT_SUPPORTED:
            type_str = "not_supported_error";
            code = 501;
            break;
        case ERROR_TYPE_UNAVAILABLE:
            type_str = "unavailable_error";
            code = 503;
            break;
    }
    return json {
        {"code", code},
        {"message", message},
        {"type", type_str},
    };
}

struct server_task_result_error : server_task_result {
    int index = 0;
    error_type err_type = ERROR_TYPE_SERVER;
    std::string err_msg;

    virtual bool is_error() override {
        return true;
    }

    virtual json to_json() override {
        return format_error_response(err_msg, err_type);
    }
};

struct server_task_result_metrics : server_task_result {
    int n_idle_slots;
    int n_processing_slots;
    int n_tasks_deferred;
    int64_t t_start;

    // TODO: somehow reuse server_metrics in the future, instead of duplicating the fields
    uint64_t n_prompt_tokens_processed_total = 0;
    uint64_t t_prompt_processing_total       = 0;
    uint64_t n_tokens_predicted_total        = 0;
    uint64_t t_tokens_generation_total       = 0;

    uint64_t n_past_max = 0;

    uint64_t n_prompt_tokens_processed = 0;
    uint64_t t_prompt_processing       = 0;

    uint64_t n_tokens_predicted  = 0;
    uint64_t t_tokens_generation = 0;

    uint64_t n_decode_total     = 0;
    uint64_t n_busy_slots_total = 0;

    // while we can also use std::vector<server_slot> this requires copying the slot object which can be quite messy
    // therefore, we use json to temporarily store the slot.to_json() result
    json slots_data = json::array();

    virtual json to_json() override {
        return json {
            { "idle",                            n_idle_slots },
            { "processing",                      n_processing_slots },
            { "deferred",                        n_tasks_deferred },
            { "t_start",                         t_start },

            { "n_prompt_tokens_processed_total", n_prompt_tokens_processed_total },
            { "t_tokens_generation_total",       t_tokens_generation_total },
            { "n_tokens_predicted_total",        n_tokens_predicted_total },
            { "t_prompt_processing_total",       t_prompt_processing_total },

            { "n_past_max",                      n_past_max },

            { "n_prompt_tokens_processed",       n_prompt_tokens_processed },
            { "t_prompt_processing",             t_prompt_processing },
            { "n_tokens_predicted",              n_tokens_predicted },
            { "t_tokens_generation",             t_tokens_generation },

            { "n_decode_total",                  n_decode_total },
            { "n_busy_slots_total",              n_busy_slots_total },

            { "slots",                           slots_data },
        };
    }
};

struct server_task_result_slot_save_load : server_task_result {
    std::string filename;
    bool is_save; // true = save, false = load

    size_t n_tokens;
    size_t n_bytes;
    double t_ms;

    virtual json to_json() override {
        if (is_save) {
            return json {
                { "id_slot",   id_slot },
                { "filename",  filename },
                { "n_saved",   n_tokens },
                { "n_written", n_bytes },
                { "timings", {
                    { "save_ms", t_ms }
                }},
            };
        } else {
            return json {
                { "id_slot",    id_slot },
                { "filename",   filename },
                { "n_restored", n_tokens },
                { "n_read",     n_bytes },
                { "timings", {
                    { "restore_ms", t_ms }
                }},
            };
        }
    }
};

struct server_task_result_slot_erase : server_task_result {
    size_t n_erased;

    virtual json to_json() override {
        return json {
            { "id_slot",  id_slot },
            { "n_erased", n_erased },
        };
    }
};

struct server_task_result_apply_lora : server_task_result {
    virtual json to_json() override {
        return json {{ "success", true }};
    }
};

struct server_slot {
    int id;
    int id_task = -1;

    // only used for completion/embedding/infill/rerank
    server_task_type task_type = SERVER_TASK_TYPE_COMPLETION;

    llama_batch batch_spec = {};

    llama_context * ctx = nullptr;
    llama_context * ctx_dft = nullptr;

    // multimodal
    mtmd_context * mctx = nullptr;

    common_speculative * spec = nullptr;

    std::vector<common_adapter_lora_info> lora;

    // the index relative to completion multi-task request
    size_t index = 0;

    struct slot_params params;

    slot_state state = SLOT_STATE_IDLE;

    // used to determine the slot that has been used the longest
    int64_t t_last_used = -1;

    // generation props
    int32_t n_ctx       = 0;  // context size per slot
    int32_t n_past      = 0;
    int32_t n_decoded   = 0;
    int32_t n_remaining = -1;
    int32_t i_batch     = -1;
    int32_t n_predict   = -1; // TODO: disambiguate from params.n_predict

    // n_prompt_tokens may not be equal to prompt_tokens.size(), because prompt maybe truncated
    int32_t n_prompt_tokens           = 0;
    int32_t n_prompt_tokens_processed = 0;

    // input prompt tokens
    server_tokens prompt_tokens;

    size_t last_nl_pos = 0;

    std::string  generated_text;
    llama_tokens generated_tokens;
    common_chat_msg chat_msg;

    server_tokens cache_tokens;

    std::vector<completion_token_output> generated_token_probs;

    std::vector<swa_checkpoint> swa_checkpoints;

    bool has_next_token = true;
    bool has_new_line   = false;
    bool truncated      = false;
    stop_type stop;

    std::string stopping_word;

    // sampling
    json json_schema;

    struct common_sampler * smpl = nullptr;

    llama_token sampled;

    common_chat_format chat_format = COMMON_CHAT_FORMAT_CONTENT_ONLY;
    std::vector<std::string> generated_tool_call_ids;

    // stats
    size_t n_sent_text        = 0; // number of sent text character

    int64_t t_start_process_prompt;
    int64_t t_start_generation;

    double t_prompt_processing; // ms
    double t_token_generation;  // ms

    std::function<void(int)> callback_on_release;

    // Speculative decoding stats
    int32_t n_draft_total = 0;      // Total draft tokens generated
    int32_t n_draft_accepted = 0;   // Draft tokens actually accepted

    void reset() {
        SLT_DBG(*this, "%s", "\n");

        n_prompt_tokens    = 0;
        last_nl_pos        = 0;
        generated_text     = "";
        has_new_line       = false;
        truncated          = false;
        stop               = STOP_TYPE_NONE;
        stopping_word      = "";
        n_past             = 0;
        n_sent_text        = 0;
        task_type          = SERVER_TASK_TYPE_COMPLETION;
        chat_format        = COMMON_CHAT_FORMAT_CONTENT_ONLY;

        generated_tokens.clear();
        generated_token_probs.clear();
        chat_msg = {};
        json_schema = json();
        generated_tool_call_ids.clear();

        // clear speculative decoding stats
        n_draft_total = 0;
        n_draft_accepted = 0;
    }

    bool need_embd() const {
        return server_task_type_need_embd(task_type);
    }

    bool need_logits() const {
        return server_task_type_need_logits(task_type);
    }

    // if the context does not have a memory module then all embeddings have to be computed within a single ubatch
    // also we cannot split if the pooling would require any past tokens
    bool can_split() const {
        return
            !need_embd() ||
            (llama_get_memory(ctx) && llama_pooling_type(ctx) == LLAMA_POOLING_TYPE_LAST);
    }

    bool can_batch_with(server_slot & other_slot) const {
        return task_type == other_slot.task_type && are_lora_equal(lora, other_slot.lora);
    }

    bool has_budget(const common_params & global_params) {
        if (params.n_predict == -1 && global_params.n_predict == -1) {
            return true; // limitless
        }

        n_remaining = -1;

        if (params.n_predict != -1) {
            n_remaining = params.n_predict - n_decoded;
        } else if (global_params.n_predict != -1) {
            n_remaining = global_params.n_predict - n_decoded;
        }

        return n_remaining > 0; // no budget
    }

    bool is_processing() const {
        return state != SLOT_STATE_IDLE;
    }

    bool can_speculate() const {
        return ctx_dft && params.speculative.n_max > 0 && params.cache_prompt;
    }

    void add_token(const completion_token_output & token) {
        if (!is_processing()) {
            SLT_WRN(*this, "%s", "slot is not processing\n");
            return;
        }
        generated_token_probs.push_back(token);
    }

    void release() {
        if (is_processing()) {
            SLT_INF(*this, "stop processing: n_past = %d, truncated = %d\n", n_past, truncated);

            t_last_used = ggml_time_us();
            t_token_generation = (ggml_time_us() - t_start_generation) / 1e3;
            state = SLOT_STATE_IDLE;
            callback_on_release(id);
        }
    }

    result_timings get_timings() const {
        result_timings timings;
        timings.prompt_n = n_prompt_tokens_processed;
        timings.prompt_ms = t_prompt_processing;
        timings.prompt_per_token_ms = t_prompt_processing / n_prompt_tokens_processed;
        timings.prompt_per_second = 1e3 / t_prompt_processing * n_prompt_tokens_processed;

        timings.predicted_n = n_decoded;
        timings.predicted_ms = t_token_generation;
        timings.predicted_per_token_ms = t_token_generation / n_decoded;
        timings.predicted_per_second = 1e3 / t_token_generation * n_decoded;

        // Add speculative metrics
        if (n_draft_total > 0) {
            timings.draft_n = n_draft_total;
            timings.draft_n_accepted = n_draft_accepted;
        }

        return timings;
    }

    const common_chat_msg & update_chat_msg(std::vector<common_chat_msg_diff> & diffs) {
        auto previous_msg = chat_msg;
        SRV_DBG("Parsing chat message: %s\n", generated_text.c_str());
        auto new_msg = common_chat_parse(
            generated_text,
            /* is_partial= */ stop != STOP_TYPE_EOS,
            params.oaicompat_chat_syntax);
        if (!new_msg.empty()) {
            new_msg.ensure_tool_call_ids_set(generated_tool_call_ids, gen_tool_call_id);
            chat_msg = new_msg;
            diffs = common_chat_msg_diff::compute_diffs(previous_msg, new_msg.empty() ? previous_msg : new_msg);
        }
        return chat_msg;
    }

    size_t find_stopping_strings(const std::string & text, const size_t last_token_size, bool is_full_stop) {
        size_t stop_pos = std::string::npos;

        for (const std::string & word : params.antiprompt) {
            size_t pos;

            if (is_full_stop) {
                const size_t tmp      = word.size() + last_token_size;
                const size_t from_pos = text.size() > tmp ? text.size() - tmp : 0;

                pos = text.find(word, from_pos);
            } else {
                // otherwise, partial stop
                pos = string_find_partial_stop(text, word);
            }

            if (pos != std::string::npos && (stop_pos == std::string::npos || pos < stop_pos)) {
                if (is_full_stop) {
                    stop           = STOP_TYPE_WORD;
                    stopping_word  = word;
                    has_next_token = false;
                }
                stop_pos = pos;
            }
        }

        return stop_pos;
    }

    void print_timings() const {
        const double t_prompt        =       t_prompt_processing / n_prompt_tokens_processed;
        const double n_prompt_second = 1e3 / t_prompt_processing * n_prompt_tokens_processed;

        const double t_gen        =       t_token_generation / n_decoded;
        const double n_gen_second = 1e3 / t_token_generation * n_decoded;

        SLT_INF(*this,
                "\n"
                "prompt eval time = %10.2f ms / %5d tokens (%8.2f ms per token, %8.2f tokens per second)\n"
                "       eval time = %10.2f ms / %5d tokens (%8.2f ms per token, %8.2f tokens per second)\n"
                "      total time = %10.2f ms / %5d tokens\n",
                t_prompt_processing, n_prompt_tokens_processed, t_prompt, n_prompt_second,
                t_token_generation, n_decoded, t_gen, n_gen_second,
                t_prompt_processing + t_token_generation, n_prompt_tokens_processed + n_decoded);

        if (n_draft_total > 0) {
            const float draft_ratio = (float) n_draft_accepted / n_draft_total;
            SLT_INF(*this,
                    "\n"
                    "draft acceptance rate = %0.5f (%5d accepted / %5d generated)\n",
                    draft_ratio, n_draft_accepted, n_draft_total
            );
        }
    }

    json to_json() const {
        return json {
            {"id",            id},
            {"id_task",       id_task},
            {"n_ctx",         n_ctx},
            {"speculative",   can_speculate()},
            {"is_processing", is_processing()},
            {"params",        params.to_json()},
            {"prompt",        prompt_tokens.detokenize(ctx, true)},
            {"next_token",
                {
                    {"has_next_token", has_next_token},
                    {"has_new_line",   has_new_line},
                    {"n_remain",       n_remaining},
                    {"n_decoded",      n_decoded},
                    {"stopping_word",  stopping_word},
                }
            },
        };
    }
};

struct server_metrics {
    int64_t t_start = 0;

    uint64_t n_prompt_tokens_processed_total = 0;
    uint64_t t_prompt_processing_total       = 0;
    uint64_t n_tokens_predicted_total        = 0;
    uint64_t t_tokens_generation_total       = 0;

    uint64_t n_past_max = 0;

    uint64_t n_prompt_tokens_processed = 0;
    uint64_t t_prompt_processing       = 0;

    uint64_t n_tokens_predicted  = 0;
    uint64_t t_tokens_generation = 0;

    uint64_t n_decode_total     = 0;
    uint64_t n_busy_slots_total = 0;

    void init() {
        t_start = ggml_time_us();
    }

    void on_prompt_eval(const server_slot & slot) {
        n_prompt_tokens_processed_total += slot.n_prompt_tokens_processed;
        n_prompt_tokens_processed       += slot.n_prompt_tokens_processed;
        t_prompt_processing             += slot.t_prompt_processing;
        t_prompt_processing_total       += slot.t_prompt_processing;

        if (slot.n_past > 0) {
            n_past_max = std::max(n_past_max, (uint64_t) slot.n_past);
        }
    }

    void on_prediction(const server_slot & slot) {
        n_tokens_predicted_total   += slot.n_decoded;
        n_tokens_predicted         += slot.n_decoded;
        t_tokens_generation        += slot.t_token_generation;
        t_tokens_generation_total  += slot.t_token_generation;
    }

    void on_decoded(const std::vector<server_slot> & slots) {
        n_decode_total++;
        for (const auto & slot : slots) {
            if (slot.is_processing()) {
                n_busy_slots_total++;
            }
            if (slot.n_past > 0) {
                n_past_max = std::max(n_past_max, (uint64_t) slot.n_past);
            }
        }
    }

    void reset_bucket() {
        n_prompt_tokens_processed = 0;
        t_prompt_processing       = 0;
        n_tokens_predicted        = 0;
        t_tokens_generation       = 0;
    }
};

struct server_queue {
    int id = 0;
    bool running;

    // queues
    std::deque<server_task> queue_tasks;
    std::deque<server_task> queue_tasks_deferred;

    std::mutex mutex_tasks;
    std::condition_variable condition_tasks;

    // callback functions
    std::function<void(server_task &&)> callback_new_task;
    std::function<void(void)>           callback_update_slots;

    // Add a new task to the end of the queue
    int post(server_task && task, bool front = false) {
        std::unique_lock<std::mutex> lock(mutex_tasks);
        GGML_ASSERT(task.id != -1);
        // if this is cancel task make sure to clean up pending tasks
        if (task.type == SERVER_TASK_TYPE_CANCEL) {
            cleanup_pending_task(task.id_target);
        }
        const int task_id = task.id;
        QUE_DBG("new task, id = %d, front = %d\n", task_id, front);
        if (front) {
            queue_tasks.push_front(std::move(task));
        } else {
            queue_tasks.push_back(std::move(task));
        }
        condition_tasks.notify_one();
        return task_id;
    }

    // multi-task version of post()
    int post(std::vector<server_task> && tasks, bool front = false) {
        std::unique_lock<std::mutex> lock(mutex_tasks);
        for (auto & task : tasks) {
            if (task.id == -1) {
                task.id = id++;
            }
            // if this is cancel task make sure to clean up pending tasks
            if (task.type == SERVER_TASK_TYPE_CANCEL) {
                cleanup_pending_task(task.id_target);
            }
            QUE_DBG("new task, id = %d/%d, front = %d\n", task.id, (int) tasks.size(), front);
            if (front) {
                queue_tasks.push_front(std::move(task));
            } else {
                queue_tasks.push_back(std::move(task));
            }
        }
        condition_tasks.notify_one();
        return 0;
    }

    // Add a new task, but defer until one slot is available
    void defer(server_task && task) {
        std::unique_lock<std::mutex> lock(mutex_tasks);
        QUE_DBG("defer task, id = %d\n", task.id);
        queue_tasks_deferred.push_back(std::move(task));
        condition_tasks.notify_one();
    }

    // Get the next id for creating a new task
    int get_new_id() {
        std::unique_lock<std::mutex> lock(mutex_tasks);
        int new_id = id++;
        return new_id;
    }

    // Register function to process a new task
    void on_new_task(std::function<void(server_task &&)> callback) {
        callback_new_task = std::move(callback);
    }

    // Register the function to be called when all slots data is ready to be processed
    void on_update_slots(std::function<void(void)> callback) {
        callback_update_slots = std::move(callback);
    }

    // Call when the state of one slot is changed, it will move one task from deferred to main queue
    void pop_deferred_task() {
        std::unique_lock<std::mutex> lock(mutex_tasks);
        if (!queue_tasks_deferred.empty()) {
            queue_tasks.emplace_front(std::move(queue_tasks_deferred.front()));
            queue_tasks_deferred.pop_front();
        }
        condition_tasks.notify_one();
    }

    // end the start_loop routine
    void terminate() {
        std::unique_lock<std::mutex> lock(mutex_tasks);
        running = false;
        condition_tasks.notify_all();
    }

    /**
     * Main loop consists of these steps:
     * - Wait until a new task arrives
     * - Process the task (i.e. maybe copy data into slot)
     * - Check if multitask is finished
     * - Update all slots
     */
    void start_loop() {
        running = true;

        while (true) {
            QUE_DBG("%s", "processing new tasks\n");

            while (true) {
                std::unique_lock<std::mutex> lock(mutex_tasks);
                if (!running) {
                    QUE_DBG("%s", "terminate\n");
                    return;
                }
                if (queue_tasks.empty()) {
                    lock.unlock();
                    break;
                }
                server_task task = std::move(queue_tasks.front());
                queue_tasks.pop_front();
                lock.unlock();

                QUE_DBG("processing task, id = %d\n", task.id);
                callback_new_task(std::move(task));
            }

            // all tasks in the current loop is processed, slots data is now ready
            QUE_DBG("%s", "update slots\n");

            callback_update_slots();

            QUE_DBG("%s", "waiting for new tasks\n");
            {
                std::unique_lock<std::mutex> lock(mutex_tasks);
                if (!running) {
                    QUE_DBG("%s", "terminate\n");
                    return;
                }
                if (queue_tasks.empty()) {
                    condition_tasks.wait(lock, [&]{
                        return (!queue_tasks.empty() || !running);
                    });
                }
            }
        }
    }

private:
    void cleanup_pending_task(int id_target) {
        // no need lock because this is called exclusively by post()
        auto rm_func = [id_target](const server_task & task) {
            return task.id_target == id_target;
        };
        queue_tasks.erase(
            std::remove_if(queue_tasks.begin(),          queue_tasks.end(),          rm_func),
            queue_tasks.end());
        queue_tasks_deferred.erase(
            std::remove_if(queue_tasks_deferred.begin(), queue_tasks_deferred.end(), rm_func),
            queue_tasks_deferred.end());
    }
};

struct server_response {
    bool running = true;

    // for keeping track of all tasks waiting for the result
    std::unordered_set<int> waiting_task_ids;

    // the main result queue (using ptr for polymorphism)
    std::vector<server_task_result_ptr> queue_results;

    std::mutex mutex_results;
    std::condition_variable condition_results;

    // add the id_task to the list of tasks waiting for response
    void add_waiting_task_id(int id_task) {
        SRV_DBG("add task %d to waiting list. current waiting = %d (before add)\n", id_task, (int) waiting_task_ids.size());

        std::unique_lock<std::mutex> lock(mutex_results);
        waiting_task_ids.insert(id_task);
    }

    void add_waiting_tasks(const std::vector<server_task> & tasks) {
        std::unique_lock<std::mutex> lock(mutex_results);

        for (const auto & task : tasks) {
            SRV_DBG("add task %d to waiting list. current waiting = %d (before add)\n", task.id, (int) waiting_task_ids.size());
            waiting_task_ids.insert(task.id);
        }
    }

    // when the request is finished, we can remove task associated with it
    void remove_waiting_task_id(int id_task) {
        SRV_DBG("remove task %d from waiting list. current waiting = %d (before remove)\n", id_task, (int) waiting_task_ids.size());

        std::unique_lock<std::mutex> lock(mutex_results);
        waiting_task_ids.erase(id_task);
        // make sure to clean up all pending results
        queue_results.erase(
            std::remove_if(queue_results.begin(), queue_results.end(), [id_task](const server_task_result_ptr & res) {
                return res->id == id_task;
            }),
            queue_results.end());
    }

    void remove_waiting_task_ids(const std::unordered_set<int> & id_tasks) {
        std::unique_lock<std::mutex> lock(mutex_results);

        for (const auto & id_task : id_tasks) {
            SRV_DBG("remove task %d from waiting list. current waiting = %d (before remove)\n", id_task, (int) waiting_task_ids.size());
            waiting_task_ids.erase(id_task);
        }
    }

    // This function blocks the thread until there is a response for one of the id_tasks
    server_task_result_ptr recv(const std::unordered_set<int> & id_tasks) {
        while (true) {
            std::unique_lock<std::mutex> lock(mutex_results);
            condition_results.wait(lock, [&]{
                if (!running) {
                    SRV_DBG("%s : queue result stop\n", __func__);
                    std::terminate(); // we cannot return here since the caller is HTTP code
                }
                return !queue_results.empty();
            });

            for (size_t i = 0; i < queue_results.size(); i++) {
                if (id_tasks.find(queue_results[i]->id) != id_tasks.end()) {
                    server_task_result_ptr res = std::move(queue_results[i]);
                    queue_results.erase(queue_results.begin() + i);
                    return res;
                }
            }
        }

        // should never reach here
    }

    // same as recv(), but have timeout in seconds
    // if timeout is reached, nullptr is returned
    server_task_result_ptr recv_with_timeout(const std::unordered_set<int> & id_tasks, int timeout) {
        while (true) {
            std::unique_lock<std::mutex> lock(mutex_results);

            for (int i = 0; i < (int) queue_results.size(); i++) {
                if (id_tasks.find(queue_results[i]->id) != id_tasks.end()) {
                    server_task_result_ptr res = std::move(queue_results[i]);
                    queue_results.erase(queue_results.begin() + i);
                    return res;
                }
            }

            std::cv_status cr_res = condition_results.wait_for(lock, std::chrono::seconds(timeout));
            if (!running) {
                SRV_DBG("%s : queue result stop\n", __func__);
                std::terminate(); // we cannot return here since the caller is HTTP code
            }
            if (cr_res == std::cv_status::timeout) {
                return nullptr;
            }
        }

        // should never reach here
    }

    // single-task version of recv()
    server_task_result_ptr recv(int id_task) {
        std::unordered_set<int> id_tasks = {id_task};
        return recv(id_tasks);
    }

    // Send a new result to a waiting id_task
    void send(server_task_result_ptr && result) {
        SRV_DBG("sending result for task id = %d\n", result->id);

        std::unique_lock<std::mutex> lock(mutex_results);
        for (const auto & id_task : waiting_task_ids) {
            if (result->id == id_task) {
                SRV_DBG("task id = %d pushed to result queue\n", result->id);

                queue_results.emplace_back(std::move(result));
                condition_results.notify_all();
                return;
            }
        }
    }

    // terminate the waiting loop
    void terminate() {
        running = false;
        condition_results.notify_all();
    }
};

struct server_context {
    common_params params_base;

    // note: keep these alive - they determine the lifetime of the model, context, etc.
    common_init_result llama_init;
    common_init_result llama_init_dft;

    llama_model * model = nullptr;
    llama_context * ctx = nullptr;

    // multimodal
    mtmd_context * mctx = nullptr;

    const llama_vocab * vocab = nullptr;
    bool vocab_dft_compatible = true;

    llama_model * model_dft = nullptr;

    llama_context_params cparams_dft;

    llama_batch batch {};

    bool clean_kv_cache = true;
    bool add_bos_token  = true;

    int32_t n_ctx; // total context for all clients / slots

    // slots / clients
    std::vector<server_slot> slots;
    json default_generation_settings_for_props;

    server_queue    queue_tasks;
    server_response queue_results;

    server_metrics metrics;

    // Necessary similarity of prompt for slot selection
    float slot_prompt_similarity = 0.0f;

    common_chat_templates_ptr chat_templates;
    oaicompat_parser_options  oai_parser_opt;

    ~server_context() {
        mtmd_free(mctx);

        // Clear any sampling context
        for (server_slot & slot : slots) {
            common_sampler_free(slot.smpl);
            slot.smpl = nullptr;

            llama_free(slot.ctx_dft);
            slot.ctx_dft = nullptr;

            common_speculative_free(slot.spec);
            slot.spec = nullptr;

            llama_batch_free(slot.batch_spec);
        }

        llama_batch_free(batch);
    }

    bool load_model(const common_params & params) {
        SRV_INF("loading model '%s'\n", params.model.path.c_str());

        params_base = params;

        llama_init = common_init_from_params(params_base);

        model = llama_init.model.get();
        ctx   = llama_init.context.get();

        if (model == nullptr) {
            SRV_ERR("failed to load model, '%s'\n", params_base.model.path.c_str());
            return false;
        }

        vocab = llama_model_get_vocab(model);

        n_ctx = llama_n_ctx(ctx);

        add_bos_token = llama_vocab_get_add_bos(vocab);

        if (!params_base.speculative.model.path.empty() || !params_base.speculative.model.hf_repo.empty()) {
            SRV_INF("loading draft model '%s'\n", params_base.speculative.model.path.c_str());

            auto params_dft = params_base;

            params_dft.devices      = params_base.speculative.devices;
            params_dft.model        = params_base.speculative.model;
            params_dft.n_ctx        = params_base.speculative.n_ctx == 0 ? params_base.n_ctx / params_base.n_parallel : params_base.speculative.n_ctx;
            params_dft.n_gpu_layers = params_base.speculative.n_gpu_layers;
            params_dft.n_parallel   = 1;
            params_dft.cache_type_k = params_base.speculative.cache_type_k;
            params_dft.cache_type_v = params_base.speculative.cache_type_v;

            params_dft.cpuparams.n_threads = params_base.speculative.cpuparams.n_threads;
            params_dft.cpuparams_batch.n_threads = params_base.speculative.cpuparams_batch.n_threads;
            params_dft.tensor_buft_overrides = params_base.speculative.tensor_buft_overrides;

            llama_init_dft = common_init_from_params(params_dft);

            model_dft = llama_init_dft.model.get();

            if (model_dft == nullptr) {
                SRV_ERR("failed to load draft model, '%s'\n", params_base.speculative.model.path.c_str());
                return false;
            }

            vocab_dft_compatible = common_speculative_are_compatible(ctx, llama_init_dft.context.get());
            if (!vocab_dft_compatible) {
                SRV_INF("the draft model '%s' is not compatible with the target model '%s'. tokens will be translated between the draft and target models.\n", params_base.speculative.model.path.c_str(), params_base.model.path.c_str());
            }

            const int n_ctx_dft = llama_n_ctx(llama_init_dft.context.get());

            cparams_dft = common_context_params_to_llama(params_dft);
            cparams_dft.n_batch = n_ctx_dft;

            // the context is not needed - we will create one for each slot
            llama_init_dft.context.reset();
        }

        chat_templates = common_chat_templates_init(model, params_base.chat_template);
        try {
            common_chat_format_example(chat_templates.get(), params.use_jinja, params.default_template_kwargs);
        } catch (const std::exception & e) {
            SRV_WRN("%s: Chat template parsing error: %s\n", __func__, e.what());
            SRV_WRN("%s: The chat template that comes with this model is not yet supported, falling back to chatml. This may cause the model to output suboptimal responses\n", __func__);
            chat_templates = common_chat_templates_init(model, "chatml");
        }

        std::string & mmproj_path = params_base.mmproj.path;
        if (!mmproj_path.empty()) {
            mtmd_context_params mparams = mtmd_context_params_default();
            mparams.use_gpu       = params_base.mmproj_use_gpu;
            mparams.print_timings = false;
            mparams.n_threads     = params_base.cpuparams.n_threads;
            mparams.verbosity     = params_base.verbosity > 0 ? GGML_LOG_LEVEL_DEBUG : GGML_LOG_LEVEL_INFO;
            mctx = mtmd_init_from_file(mmproj_path.c_str(), model, mparams);
            if (mctx == nullptr) {
                SRV_ERR("failed to load multimodal model, '%s'\n", mmproj_path.c_str());
                return false;
            }
            SRV_INF("loaded multimodal model, '%s'\n", mmproj_path.c_str());

            if (params_base.ctx_shift) {
                params_base.ctx_shift = false;
                SRV_WRN("%s\n", "ctx_shift is not supported by multimodal, it will be disabled");
            }

            if (params_base.n_cache_reuse) {
                params_base.n_cache_reuse = 0;
                SRV_WRN("%s\n", "cache_reuse is not supported by multimodal, it will be disabled");
            }

            if (!params_base.speculative.model.path.empty()) {
                SRV_ERR("%s\n", "err: speculative decode is not supported by multimodal");
                return false;
            }
        }

        if (!llama_memory_can_shift(llama_get_memory(ctx))) {
            if (params_base.ctx_shift) {
                params_base.ctx_shift = false;
                SRV_WRN("%s\n", "ctx_shift is not supported by this context, it will be disabled");
            }

            if (params_base.n_cache_reuse) {
                params_base.n_cache_reuse = 0;
                SRV_WRN("%s\n", "cache_reuse is not supported by this context, it will be disabled");
            }
        }

        return true;
    }

    void init() {
        const int32_t n_ctx_slot = n_ctx / params_base.n_parallel;

        SRV_INF("initializing slots, n_slots = %d\n", params_base.n_parallel);

        for (int i = 0; i < params_base.n_parallel; i++) {
            server_slot slot;

            slot.id = i;
            slot.ctx = ctx;
            slot.n_ctx = n_ctx_slot;
            slot.n_predict = params_base.n_predict;
            slot.mctx = mctx;
            slot.cache_tokens.has_mtmd = mctx != nullptr;

            if (model_dft) {
                slot.batch_spec = llama_batch_init(params_base.speculative.n_max + 1, 0, 1);

                slot.ctx_dft = llama_init_from_model(model_dft, cparams_dft);
                if (slot.ctx_dft == nullptr) {
                    SRV_ERR("%s", "failed to create draft context\n");
                    return;
                }

                slot.spec = common_speculative_init(slot.ctx, slot.ctx_dft);
                if (slot.spec == nullptr) {
                    SRV_ERR("%s", "failed to create speculator\n");
                    return;
                }
                for (auto &pair : params_base.speculative.replacements) {
                    common_speculative_add_replacement_tgt_dft(slot.spec, pair.first.c_str(), pair.second.c_str());
                }
            }

            SLT_INF(slot, "new slot n_ctx_slot = %d\n", slot.n_ctx);

            slot.params.sampling = params_base.sampling;
            slot.params.n_keep = params_base.n_keep;

            slot.callback_on_release = [this](int) {
                queue_tasks.pop_deferred_task();
            };

            slot.reset();

            slots.push_back(std::move(slot));
        }

        default_generation_settings_for_props = slots[0].to_json();

        // the update_slots() logic will always submit a maximum of n_batch or n_parallel tokens
        // note that n_batch can be > n_ctx (e.g. for non-causal attention models such as BERT where the KV cache is not used)
        {
            const int32_t n_batch = llama_n_batch(ctx);
            batch = llama_batch_init(std::max(n_batch, params_base.n_parallel), 0, 1);
        }

        metrics.init();

        oai_parser_opt = {
            /* use_jinja             */ params_base.use_jinja,
            /* prefill_assistant     */ params_base.prefill_assistant,
            /* reasoning_format      */ params_base.reasoning_format,
            /* chat_template_kwargs  */ params_base.default_template_kwargs,
            /* common_chat_templates */ chat_templates.get(),
            /* allow_image           */ mctx ? mtmd_support_vision(mctx) : false,
            /* allow_audio           */ mctx ? mtmd_support_audio (mctx) : false,
            /* enable_thinking       */ params_base.reasoning_budget != 0,
        };
    }

    server_slot * get_slot_by_id(int id) {
        for (server_slot & slot : slots) {
            if (slot.id == id) {
                return &slot;
            }
        }

        return nullptr;
    }

    server_slot * get_available_slot(const server_task & task) {
        server_slot * ret = nullptr;

        // find the slot that has at least n% prompt similarity
        if (ret == nullptr && slot_prompt_similarity != 0.0f) {
            int lcs_len = 0;
            float similarity = 0;

            for (server_slot & slot : slots) {
                // skip the slot if it is not available
                if (slot.is_processing()) {
                    continue;
                }

                // skip the slot if it does not contains cached tokens
                if (slot.cache_tokens.empty()) {
                    continue;
                }

                // length of the Longest Common Subsequence between the current slot's prompt and the input prompt
                int cur_lcs_len = slot.cache_tokens.get_common_prefix(task.prompt_tokens);

                // fraction of the common subsequence length compared to the current slot's prompt length
                float cur_similarity = static_cast<float>(cur_lcs_len) / static_cast<int>(slot.cache_tokens.size());

                // select the current slot if the criteria match
                if (cur_lcs_len > lcs_len && cur_similarity > slot_prompt_similarity) {
                    lcs_len = cur_lcs_len;
                    similarity = cur_similarity;
                    ret = &slot;
                }
            }

            if (ret != nullptr) {
                SLT_DBG(*ret, "selected slot by lcs similarity, lcs_len = %d, similarity = %f\n", lcs_len, similarity);
            }
        }

        // find the slot that has been least recently used
        if (ret == nullptr) {
            int64_t t_last = -1;

            for (server_slot & slot : slots) {
                // skip the slot if it is not available
                if (slot.is_processing()) {
                    continue;
                }

                // select the current slot if the criteria match
                if (!ret || slot.t_last_used <= t_last) {
                    t_last = slot.t_last_used;
                    ret = &slot;
                }
            }

            if (ret != nullptr) {
                SLT_DBG(*ret, "selected slot by lru, t_last = %" PRId64 "\n", t_last);
            }
        }

        return ret;
    }

    bool launch_slot_with_task(server_slot & slot, server_task && task) {
        slot.reset();
        slot.id_task       = task.id;
        slot.index         = task.index;
        slot.task_type     = task.type;
        slot.params        = std::move(task.params);
        slot.prompt_tokens = std::move(task.prompt_tokens);

        if (!are_lora_equal(slot.params.lora, slot.lora)) {
            // if lora is changed, we cannot reuse cached tokens
            slot.cache_tokens.clear();
            slot.lora = slot.params.lora;
        }

        if (!slot.prompt_tokens.validate(ctx)) {
            send_error(task, "Prompt contains invalid tokens", ERROR_TYPE_INVALID_REQUEST);
            return false;
        }
        SLT_DBG(slot, "launching slot : %s\n", safe_json_to_str(slot.to_json()).c_str());

        if (slot.n_predict > 0 && slot.params.n_predict > slot.n_predict) {
            // Might be better to reject the request with a 400 ?
            SLT_WRN(slot, "n_predict = %d exceeds server configuration, setting to %d\n", slot.params.n_predict, slot.n_predict);
            slot.params.n_predict = slot.n_predict;
        }

        {
            if (slot.smpl != nullptr) {
                common_sampler_free(slot.smpl);
            }

            slot.smpl = common_sampler_init(model, slot.params.sampling);
            if (slot.smpl == nullptr) {
                // for now, the only error that may happen here is invalid grammar
                send_error(task, "Failed to parse grammar", ERROR_TYPE_INVALID_REQUEST);
                return false;
            }
        }

        if (slot.ctx_dft) {
            llama_batch_free(slot.batch_spec);

            slot.batch_spec = llama_batch_init(slot.params.speculative.n_max + 1, 0, 1);
        }

        slot.state = SLOT_STATE_STARTED;

        SLT_INF(slot, "%s", "processing task\n");

        return true;
    }

    void kv_cache_clear() {
        SRV_DBG("%s", "clearing KV cache\n");

        // clear the entire KV cache
        llama_memory_clear(llama_get_memory(ctx), true);
        clean_kv_cache = false;
    }

    bool process_token(completion_token_output & result, server_slot & slot) {
        // remember which tokens were sampled - used for repetition penalties during sampling
        const std::string token_str = result.text_to_send;
        slot.sampled = result.tok;

        slot.generated_text += token_str;
        if (slot.params.return_tokens) {
            slot.generated_tokens.push_back(result.tok);
        }
        slot.has_next_token = true;

        // check if there is incomplete UTF-8 character at the end
        bool incomplete = validate_utf8(slot.generated_text) < slot.generated_text.size();

        // search stop word and delete it
        if (!incomplete) {
            size_t pos = std::min(slot.n_sent_text, slot.generated_text.size());

            const std::string str_test = slot.generated_text.substr(pos);
            bool send_text = true;

            size_t stop_pos = slot.find_stopping_strings(str_test, token_str.size(), true);
            if (stop_pos != std::string::npos) {
                slot.generated_text.erase(
                    slot.generated_text.begin() + pos + stop_pos,
                    slot.generated_text.end());
                pos = std::min(slot.n_sent_text, slot.generated_text.size());
            } else if (slot.has_next_token) {
                stop_pos = slot.find_stopping_strings(str_test, token_str.size(), false);
                send_text = stop_pos == std::string::npos;
            }

            // check if there is any token to predict
            if (send_text) {
                // no send the stop word in the response
                result.text_to_send = slot.generated_text.substr(pos, std::string::npos);
                slot.n_sent_text += result.text_to_send.size();
                // add the token to slot queue and cache
            } else {
                result.text_to_send = "";
            }

            slot.add_token(result);
            if (slot.params.stream) {
                send_partial_response(slot, result);
            }
        }

        if (incomplete) {
            slot.has_next_token = true;
        }

        // if context shifting is disabled, make sure that we don't run out of context
        if (!params_base.ctx_shift && slot.n_past + 1 >= slot.n_ctx) {
            slot.stop           = STOP_TYPE_LIMIT;
            slot.has_next_token = false;

            SLT_DBG(slot, "stopped due to running out of context, n_past = %d, n_ctx = %d\n", slot.n_past, slot.n_ctx);
        }

        // check the limits
        if (slot.n_decoded > 0 && slot.has_next_token && !slot.has_budget(params_base)) {
            slot.stop           = STOP_TYPE_LIMIT;
            slot.has_next_token = false;

            SLT_DBG(slot, "stopped by limit, n_decoded = %d, n_predict = %d\n", slot.n_decoded, slot.params.n_predict);
        }

        if (slot.has_new_line) {
            // require that each new line has a whitespace prefix (i.e. indentation) of at least slot.params.n_indent
            if (slot.params.n_indent > 0) {
                // check the current indentation
                // TODO: improve by not doing it more than once for each new line
                if (slot.last_nl_pos > 0) {
                    size_t pos = slot.last_nl_pos;

                    int n_indent = 0;
                    while (pos < slot.generated_text.size() && (slot.generated_text[pos] == ' ' || slot.generated_text[pos] == '\t')) {
                        n_indent++;
                        pos++;
                    }

                    if (pos < slot.generated_text.size() && n_indent < slot.params.n_indent) {
                        slot.stop           = STOP_TYPE_LIMIT;
                        slot.has_next_token = false;

                        // cut the last line
                        slot.generated_text.erase(pos, std::string::npos);

                        SLT_DBG(slot, "stopped by indentation limit, n_decoded = %d, n_indent = %d\n", slot.n_decoded, n_indent);
                    }
                }

                // find the next new line
                {
                    const size_t pos = slot.generated_text.find('\n', slot.last_nl_pos);

                    if (pos != std::string::npos) {
                        slot.last_nl_pos = pos + 1;
                    }
                }
            }
        }

        // check if there is a new line in the generated text
        if (result.text_to_send.find('\n') != std::string::npos) {
            slot.has_new_line = true;

            // if we have seen a new line, we stop after a certain time limit, but only upon another new line
            if (slot.params.t_max_predict_ms > 0 && (ggml_time_us() - slot.t_start_generation > 1000.0f*slot.params.t_max_predict_ms)) {
                slot.stop           = STOP_TYPE_LIMIT;
                slot.has_next_token = false;

                SLT_DBG(slot, "stopped by time limit, n_decoded = %d, t_max_predict_ms = %d ms\n", slot.n_decoded, (int) slot.params.t_max_predict_ms);
            }
        }

        // if context shift is disabled, we stop when it reaches the context limit
        if (slot.n_past >= slot.n_ctx) {
            slot.truncated      = true;
            slot.stop           = STOP_TYPE_LIMIT;
            slot.has_next_token = false;

            SLT_DBG(slot, "stopped due to running out of context capacity, n_past = %d, n_prompt_tokens = %d, n_decoded = %d, n_ctx = %d\n",
                    slot.n_decoded, slot.n_prompt_tokens, slot.n_past, slot.n_ctx);
        }

        if (llama_vocab_is_eog(vocab, result.tok)) {
            slot.stop           = STOP_TYPE_EOS;
            slot.has_next_token = false;

            SLT_DBG(slot, "%s", "stopped by EOS\n");
        }

        const auto n_ctx_train = llama_model_n_ctx_train(model);

        if (slot.params.n_predict < 1 && slot.n_predict < 1 && slot.n_prompt_tokens + slot.n_decoded >= n_ctx_train) {
            slot.truncated      = true;
            slot.stop           = STOP_TYPE_LIMIT;
            slot.has_next_token = false; // stop prediction

            SLT_WRN(slot,
                    "n_predict (%d) is set for infinite generation. "
                    "Limiting generated tokens to n_ctx_train (%d) to avoid EOS-less generation infinite loop\n",
                    slot.params.n_predict, n_ctx_train);
        }

        SLT_DBG(slot, "n_decoded = %d, n_remaining = %d, next token: %5d '%s'\n", slot.n_decoded, slot.n_remaining, result.tok, token_str.c_str());

        return slot.has_next_token; // continue
    }

    void populate_token_probs(const server_slot & slot, completion_token_output & result, bool post_sampling, bool special, int idx) {
        size_t n_probs = slot.params.sampling.n_probs;
        size_t n_vocab = llama_vocab_n_tokens(vocab);
        if (post_sampling) {
            const auto * cur_p = common_sampler_get_candidates(slot.smpl);
            const size_t max_probs = cur_p->size;

            // set probability for sampled token
            for (size_t i = 0; i < max_probs; i++) {
                if (cur_p->data[i].id == result.tok) {
                    result.prob = cur_p->data[i].p;
                    break;
                }
            }

            // set probability for top n_probs tokens
            result.probs.reserve(max_probs);
            for (size_t i = 0; i < std::min(max_probs, n_probs); i++) {
                result.probs.push_back({
                    cur_p->data[i].id,
                    common_token_to_piece(ctx, cur_p->data[i].id, special),
                    cur_p->data[i].p
                });
            }
        } else {
            // TODO: optimize this with min-p optimization
            std::vector<llama_token_data> cur = get_token_probabilities(ctx, idx);

            // set probability for sampled token
            for (size_t i = 0; i < n_vocab; i++) {
                // set probability for sampled token
                if (cur[i].id == result.tok) {
                    result.prob = cur[i].p;
                    break;
                }
            }

            // set probability for top n_probs tokens
            result.probs.reserve(n_probs);
            for (size_t i = 0; i < std::min(n_vocab, n_probs); i++) {
                result.probs.push_back({
                    cur[i].id,
                    common_token_to_piece(ctx, cur[i].id, special),
                    cur[i].p
                });
            }
        }
    }

    void send_error(const server_task & task, const std::string & error, const enum error_type type = ERROR_TYPE_SERVER) {
        send_error(task.id, error, type);
    }

    void send_error(const server_slot & slot, const std::string & error, const enum error_type type = ERROR_TYPE_SERVER) {
        send_error(slot.id_task, error, type);
    }

    void send_error(const int id_task, const std::string & error, const enum error_type type = ERROR_TYPE_SERVER) {
        SRV_ERR("task id = %d, error: %s\n", id_task, error.c_str());

        auto res = std::make_unique<server_task_result_error>();
        res->id       = id_task;
        res->err_type = type;
        res->err_msg  = error;

        queue_results.send(std::move(res));
    }

    // if multimodal is enabled, send an error and return false
    bool ensure_no_mtmd(const int id_task) {
        if (mctx) {
            send_error(id_task, "This feature is not supported by multimodal", ERROR_TYPE_NOT_SUPPORTED);
            return false;
        }
        return true;
    }

    void send_partial_response(server_slot & slot, const completion_token_output & tkn) {
        auto res = std::make_unique<server_task_result_cmpl_partial>();

        res->id      = slot.id_task;
        res->index   = slot.index;
        res->content = tkn.text_to_send;
        res->tokens  = { tkn.tok };

        res->n_decoded           = slot.n_decoded;
        res->n_prompt_tokens     = slot.n_prompt_tokens;
        res->post_sampling_probs = slot.params.post_sampling_probs;

        res->verbose               = slot.params.verbose;
        res->oaicompat             = slot.params.oaicompat;
        res->oaicompat_model       = slot.params.oaicompat_model;
        res->oaicompat_cmpl_id     = slot.params.oaicompat_cmpl_id;

        slot.update_chat_msg(res->oaicompat_msg_diffs);

        // populate res.probs_output
        if (slot.params.sampling.n_probs > 0) {
            res->prob_output = tkn; // copy the token probs
        }

        // populate timings if this is final response or timings_per_token is enabled
        if (slot.stop != STOP_TYPE_NONE || slot.params.timings_per_token) {
            res->timings = slot.get_timings();
        }

        queue_results.send(std::move(res));
    }

    void send_final_response(server_slot & slot) {
        auto res = std::make_unique<server_task_result_cmpl_final>();
        res->id              = slot.id_task;
        res->id_slot         = slot.id;

        res->index           = slot.index;
        res->content         = slot.generated_text;
        res->tokens          = std::move(slot.generated_tokens);
        res->timings         = slot.get_timings();
        res->prompt          = slot.prompt_tokens.detokenize(ctx, true);
        res->response_fields = std::move(slot.params.response_fields);

        res->truncated           = slot.truncated;
        res->n_decoded           = slot.n_decoded;
        res->n_prompt_tokens     = slot.n_prompt_tokens;
        res->n_tokens_cached     = slot.n_past;
        res->has_new_line        = slot.has_new_line;
        res->stopping_word       = slot.stopping_word;
        res->stop                = slot.stop;
        res->post_sampling_probs = slot.params.post_sampling_probs;

        res->verbose               = slot.params.verbose;
        res->stream                = slot.params.stream;
        res->oaicompat             = slot.params.oaicompat;
        res->oaicompat_model       = slot.params.oaicompat_model;
        res->oaicompat_cmpl_id     = slot.params.oaicompat_cmpl_id;
        res->oaicompat_msg         = slot.update_chat_msg(res->oaicompat_msg_diffs);

        // populate res.probs_output
        if (slot.params.sampling.n_probs > 0) {
            if (!slot.params.stream && slot.stop == STOP_TYPE_WORD) {
                const llama_tokens stop_word_toks = common_tokenize(ctx, slot.stopping_word, false);

                size_t safe_offset = std::min(slot.generated_token_probs.size(), stop_word_toks.size());
                res->probs_output = std::vector<completion_token_output>(
                        slot.generated_token_probs.begin(),
                        slot.generated_token_probs.end() - safe_offset);
            } else {
                res->probs_output = std::vector<completion_token_output>(
                        slot.generated_token_probs.begin(),
                        slot.generated_token_probs.end());
            }
        }

        res->generation_params = slot.params; // copy the parameters

        queue_results.send(std::move(res));
    }

    void send_embedding(const server_slot & slot, const llama_batch & batch) {
        auto res = std::make_unique<server_task_result_embd>();
        res->id        = slot.id_task;
        res->index     = slot.index;
        res->n_tokens  = slot.n_prompt_tokens;
        res->oaicompat = slot.params.oaicompat;

        const int n_embd = llama_model_n_embd(model);

        std::vector<float> embd_res(n_embd, 0.0f);

        for (int i = 0; i < batch.n_tokens; ++i) {
            if (!batch.logits[i] || batch.seq_id[i][0] != slot.id) {
                continue;
            }

            const float * embd = nullptr;
            if (llama_pooling_type(slot.ctx) == LLAMA_POOLING_TYPE_NONE) {
                embd = llama_get_embeddings_ith(ctx, i);
            } else {
                embd = llama_get_embeddings_seq(ctx, batch.seq_id[i][0]);
            }

            if (embd == nullptr) {
                SLT_ERR(slot, "failed to get embeddings, token = %d, seq_id = %d\n", batch.token[i], batch.seq_id[i][0]);

                res->embedding.push_back(std::vector<float>(n_embd, 0.0f));
                continue;
            }

            // normalize only when there is pooling
            if (llama_pooling_type(slot.ctx) != LLAMA_POOLING_TYPE_NONE) {
                common_embd_normalize(embd, embd_res.data(), n_embd, slot.params.embd_normalize);
                res->embedding.push_back(embd_res);
                break;
            } else {
                res->embedding.emplace_back(embd, embd + n_embd);
            }
        }

        SLT_DBG(slot, "%s", "sending embeddings\n");

        queue_results.send(std::move(res));
    }

    void send_rerank(const server_slot & slot, const llama_batch & batch) {
        auto res = std::make_unique<server_task_result_rerank>();
        res->id    = slot.id_task;
        res->index = slot.index;
        res->n_tokens = slot.n_prompt_tokens;

        for (int i = 0; i < batch.n_tokens; ++i) {
            if (!batch.logits[i] || batch.seq_id[i][0] != slot.id) {
                continue;
            }

            const float * embd = llama_get_embeddings_seq(ctx, batch.seq_id[i][0]);
            if (embd == NULL) {
                embd = llama_get_embeddings_ith(ctx, i);
            }

            if (embd == NULL) {
                SLT_ERR(slot, "failed to get embeddings, token = %d, seq_id = %d\n", batch.token[i], batch.seq_id[i][0]);

                res->score = -1e6;
                continue;
            }

            res->score = embd[0];
        }

        SLT_DBG(slot, "sending rerank result, res.score = %f\n", res->score);

        queue_results.send(std::move(res));
    }

    //
    // Functions to create new task(s) and receive result(s)
    //

    void cancel_tasks(const std::unordered_set<int> & id_tasks) {
        std::vector<server_task> cancel_tasks;
        cancel_tasks.reserve(id_tasks.size());
        for (const auto & id_task : id_tasks) {
            SRV_WRN("cancel task, id_task = %d\n", id_task);

            server_task task(SERVER_TASK_TYPE_CANCEL);
            task.id_target = id_task;
            queue_results.remove_waiting_task_id(id_task);
            cancel_tasks.push_back(std::move(task));
        }
        // push to beginning of the queue, so it has highest priority
        queue_tasks.post(std::move(cancel_tasks), true);
    }

    // receive the results from task(s)
    void receive_multi_results(
            const std::unordered_set<int> & id_tasks,
            const std::function<void(std::vector<server_task_result_ptr>&)> & result_handler,
            const std::function<void(json)> & error_handler,
            const std::function<bool()> & is_connection_closed) {
        std::vector<server_task_result_ptr> results(id_tasks.size());
        for (int i = 0; i < (int)id_tasks.size(); i++) {
            server_task_result_ptr result = queue_results.recv_with_timeout(id_tasks, HTTP_POLLING_SECONDS);

            if (is_connection_closed()) {
                cancel_tasks(id_tasks);
                return;
            }

            if (result == nullptr) {
                i--; // retry
                continue;
            }

            if (result->is_error()) {
                error_handler(result->to_json());
                cancel_tasks(id_tasks);
                return;
            }

            GGML_ASSERT(
                dynamic_cast<server_task_result_cmpl_final*>(result.get()) != nullptr
                || dynamic_cast<server_task_result_embd*>(result.get()) != nullptr
                || dynamic_cast<server_task_result_rerank*>(result.get()) != nullptr
            );
            const size_t idx = result->get_index();
            GGML_ASSERT(idx < results.size() && "index out of range");
            results[idx] = std::move(result);
        }
        result_handler(results);
    }

    // receive the results from task(s), in stream mode
    void receive_cmpl_results_stream(
            const std::unordered_set<int> & id_tasks,
            const std::function<bool(server_task_result_ptr&)> & result_handler,
            const std::function<void(json)> & error_handler,
            const std::function<bool()> & is_connection_closed) {
        size_t n_finished = 0;
        while (true) {
            server_task_result_ptr result = queue_results.recv_with_timeout(id_tasks, HTTP_POLLING_SECONDS);

            if (is_connection_closed()) {
                cancel_tasks(id_tasks);
                return;
            }

            if (result == nullptr) {
                continue; // retry
            }

            if (result->is_error()) {
                error_handler(result->to_json());
                cancel_tasks(id_tasks);
                return;
            }

            GGML_ASSERT(
                dynamic_cast<server_task_result_cmpl_partial*>(result.get()) != nullptr
                || dynamic_cast<server_task_result_cmpl_final*>(result.get()) != nullptr
            );
            if (!result_handler(result)) {
                cancel_tasks(id_tasks);
                break;
            }

            if (result->is_stop()) {
                if (++n_finished == id_tasks.size()) {
                    break;
                }
            }
        }
    }

    //
    // Functions to process the task
    //

    void process_single_task(server_task && task) {
        switch (task.type) {
            case SERVER_TASK_TYPE_COMPLETION:
            case SERVER_TASK_TYPE_INFILL:
            case SERVER_TASK_TYPE_EMBEDDING:
            case SERVER_TASK_TYPE_RERANK:
                {
                    const int id_slot = task.id_selected_slot;

                    server_slot * slot = id_slot != -1 ? get_slot_by_id(id_slot) : get_available_slot(task);

                    if (slot == nullptr) {
                        // if no slot is available, we defer this task for processing later
                        SRV_DBG("no slot is available, defer task, id_task = %d\n", task.id);
                        queue_tasks.defer(std::move(task));
                        break;
                    }

                    if (slot->is_processing()) {
                        // if requested slot is unavailable, we defer this task for processing later
                        SRV_DBG("requested slot is unavailable, defer task, id_task = %d\n", task.id);
                        queue_tasks.defer(std::move(task));
                        break;
                    }

                    if (!launch_slot_with_task(*slot, std::move(task))) {
                        SRV_ERR("failed to launch slot with task, id_task = %d\n", task.id);
                        break;
                    }
                } break;
            case SERVER_TASK_TYPE_CANCEL:
                {
                    // release slot linked with the task id
                    for (auto & slot : slots) {
                        if (slot.id_task == task.id_target) {
                            slot.release();
                            break;
                        }
                    }
                } break;
            case SERVER_TASK_TYPE_NEXT_RESPONSE:
                {
                    // do nothing
                } break;
            case SERVER_TASK_TYPE_METRICS:
                {
                    json slots_data = json::array();

                    int n_idle_slots       = 0;
                    int n_processing_slots = 0;

                    for (server_slot & slot : slots) {
                        json slot_data = slot.to_json();

                        if (slot.is_processing()) {
                            n_processing_slots++;
                        } else {
                            n_idle_slots++;
                        }

                        slots_data.push_back(slot_data);
                    }
                    SRV_DBG("n_idle_slots = %d, n_processing_slots = %d\n", n_idle_slots, n_processing_slots);

                    auto res = std::make_unique<server_task_result_metrics>();
                    res->id                  = task.id;
                    res->slots_data          = std::move(slots_data);
                    res->n_idle_slots        = n_idle_slots;
                    res->n_processing_slots  = n_processing_slots;
                    res->n_tasks_deferred    = queue_tasks.queue_tasks_deferred.size();
                    res->t_start             = metrics.t_start;

                    res->n_prompt_tokens_processed_total = metrics.n_prompt_tokens_processed_total;
                    res->t_prompt_processing_total       = metrics.t_prompt_processing_total;
                    res->n_tokens_predicted_total        = metrics.n_tokens_predicted_total;
                    res->t_tokens_generation_total       = metrics.t_tokens_generation_total;

                    res->n_past_max = metrics.n_past_max;

                    res->n_prompt_tokens_processed = metrics.n_prompt_tokens_processed;
                    res->t_prompt_processing       = metrics.t_prompt_processing;
                    res->n_tokens_predicted        = metrics.n_tokens_predicted;
                    res->t_tokens_generation       = metrics.t_tokens_generation;

                    res->n_decode_total          = metrics.n_decode_total;
                    res->n_busy_slots_total      = metrics.n_busy_slots_total;

                    if (task.metrics_reset_bucket) {
                        metrics.reset_bucket();
                    }
                    queue_results.send(std::move(res));
                } break;
            case SERVER_TASK_TYPE_SLOT_SAVE:
                {
                    if (!ensure_no_mtmd(task.id)) {
                        break;
                    }

                    int id_slot = task.slot_action.slot_id;
                    server_slot * slot = get_slot_by_id(id_slot);
                    if (slot == nullptr) {
                        send_error(task, "Invalid slot ID", ERROR_TYPE_INVALID_REQUEST);
                        break;
                    }
                    if (slot->is_processing()) {
                        // if requested slot is unavailable, we defer this task for processing later
                        SRV_DBG("requested slot is unavailable, defer task, id_task = %d\n", task.id);
                        queue_tasks.defer(std::move(task));
                        break;
                    }

                    const size_t token_count = slot->cache_tokens.size();
                    const int64_t t_start = ggml_time_us();

                    std::string filename = task.slot_action.filename;
                    std::string filepath = task.slot_action.filepath;

                    const llama_tokens & tokens = slot->cache_tokens.get_text_tokens();
                    const size_t nwrite = llama_state_seq_save_file(ctx, filepath.c_str(), slot->id, tokens.data(), token_count);

                    const int64_t t_end = ggml_time_us();
                    const double t_save_ms = (t_end - t_start) / 1000.0;

                    auto res = std::make_unique<server_task_result_slot_save_load>();
                    res->id       = task.id;
                    res->id_slot  = id_slot;
                    res->filename = filename;
                    res->is_save  = true;
                    res->n_tokens = token_count;
                    res->n_bytes  = nwrite;
                    res->t_ms     = t_save_ms;
                    queue_results.send(std::move(res));
                } break;
            case SERVER_TASK_TYPE_SLOT_RESTORE:
                {
                    if (!ensure_no_mtmd(task.id)) break;
                    int id_slot = task.slot_action.slot_id;
                    server_slot * slot = get_slot_by_id(id_slot);
                    if (slot == nullptr) {
                        send_error(task, "Invalid slot ID", ERROR_TYPE_INVALID_REQUEST);
                        break;
                    }
                    if (slot->is_processing()) {
                        // if requested slot is unavailable, we defer this task for processing later
                        SRV_DBG("requested slot is unavailable, defer task, id_task = %d\n", task.id);
                        queue_tasks.defer(std::move(task));
                        break;
                    }

                    const int64_t t_start = ggml_time_us();

                    std::string filename = task.slot_action.filename;
                    std::string filepath = task.slot_action.filepath;

                    llama_tokens tokens;
                    tokens.resize(slot->n_ctx);
                    size_t token_count = 0;
                    size_t nread = llama_state_seq_load_file(ctx, filepath.c_str(), slot->id, tokens.data(), tokens.size(), &token_count);
                    if (nread == 0) {
                        slot->cache_tokens.clear(); // KV may already been invalidated?
                        send_error(task, "Unable to restore slot, no available space in KV cache or invalid slot save file", ERROR_TYPE_INVALID_REQUEST);
                        break;
                    }
                    tokens.resize(token_count);
                    slot->cache_tokens.clear();
                    slot->cache_tokens.insert(tokens);

                    const int64_t t_end = ggml_time_us();
                    const double t_restore_ms = (t_end - t_start) / 1000.0;

                    auto res = std::make_unique<server_task_result_slot_save_load>();
                    res->id       = task.id;
                    res->id_slot  = id_slot;
                    res->filename = filename;
                    res->is_save  = false;
                    res->n_tokens = token_count;
                    res->n_bytes  = nread;
                    res->t_ms     = t_restore_ms;
                    queue_results.send(std::move(res));
                } break;
            case SERVER_TASK_TYPE_SLOT_ERASE:
                {
                    if (!ensure_no_mtmd(task.id)) break;
                    int id_slot = task.slot_action.slot_id;
                    server_slot * slot = get_slot_by_id(id_slot);
                    if (slot == nullptr) {
                        send_error(task, "Invalid slot ID", ERROR_TYPE_INVALID_REQUEST);
                        break;
                    }
                    if (slot->is_processing()) {
                        // if requested slot is unavailable, we defer this task for processing later
                        SRV_DBG("requested slot is unavailable, defer task, id_task = %d\n", task.id);
                        queue_tasks.defer(std::move(task));
                        break;
                    }

                    // Erase token cache
                    const size_t n_erased = slot->cache_tokens.size();
                    llama_memory_seq_rm(llama_get_memory(ctx), slot->id, -1, -1);
                    slot->cache_tokens.clear();

                    auto res = std::make_unique<server_task_result_slot_erase>();
                    res->id       = task.id;
                    res->id_slot  = id_slot;
                    res->n_erased = n_erased;
                    queue_results.send(std::move(res));
                } break;
            case SERVER_TASK_TYPE_SET_LORA:
                {
                    params_base.lora_adapters = std::move(task.set_lora);
                    auto res = std::make_unique<server_task_result_apply_lora>();
                    res->id = task.id;
                    queue_results.send(std::move(res));
                } break;

        }
    }

    void update_slots() {
        // check if all slots are idle
        {
            bool all_idle = true;

            for (auto & slot : slots) {
                if (slot.is_processing()) {
                    all_idle = false;
                    break;
                }
            }

            if (all_idle) {
                SRV_INF("%s", "all slots are idle\n");
                if (clean_kv_cache) {
                    kv_cache_clear();
                }

                return;
            }
        }

        {
            SRV_DBG("%s", "posting NEXT_RESPONSE\n");

            server_task task(SERVER_TASK_TYPE_NEXT_RESPONSE);
            task.id = queue_tasks.get_new_id();
            queue_tasks.post(std::move(task));
        }

        // apply context-shift if needed
        // TODO: simplify and improve
        for (server_slot & slot : slots) {
            if (slot.is_processing() && slot.n_past + 1 >= slot.n_ctx) {
                if (!params_base.ctx_shift) {
                    // this check is redundant (for good)
                    // we should never get here, because generation should already stopped in process_token()
                    slot.release();
                    send_error(slot, "context shift is disabled", ERROR_TYPE_SERVER);
                    continue;
                }

                if (mctx) {
                    // we should never reach this because params_base.ctx_shift is automatically disabled if mmproj is loaded
                    // we don't support ctx_shift because an image chunk may contains multiple tokens
                    GGML_ABORT("not supported by multimodal");
                }

                // Shift context
                const int n_keep    = slot.params.n_keep + add_bos_token;
                const int n_left    = slot.n_past - n_keep;
                const int n_discard = slot.params.n_discard ? slot.params.n_discard : (n_left / 2);

                SLT_WRN(slot, "slot context shift, n_keep = %d, n_left = %d, n_discard = %d\n", n_keep, n_left, n_discard);

                llama_memory_seq_rm (llama_get_memory(ctx), slot.id, n_keep            , n_keep + n_discard);
                llama_memory_seq_add(llama_get_memory(ctx), slot.id, n_keep + n_discard, slot.n_past,        -n_discard);

                // add generated tokens to cache
                {
                    llama_tokens new_tokens = slot.cache_tokens.get_text_tokens(); // copy
                    for (size_t i = n_keep + n_discard; i < new_tokens.size(); i++) {
                        new_tokens[i - n_discard] = new_tokens[i];
                    }

                    new_tokens.resize(slot.cache_tokens.size() - n_discard);
                    slot.cache_tokens.clear();
                    slot.cache_tokens.insert(new_tokens);
                }

                slot.n_past -= n_discard;

                slot.truncated = true;
            }
        }

        // start populating the batch for this iteration
        common_batch_clear(batch);

        // track if given slot can be batched with slots already in the batch
        server_slot * slot_batched = nullptr;

        auto accept_special_token = [&](server_slot & slot, llama_token token) {
            return params_base.special || slot.params.sampling.preserved_tokens.find(token) != slot.params.sampling.preserved_tokens.end();
        };

        // frist, add sampled tokens from any ongoing sequences
        for (auto & slot : slots) {
            if (slot.state != SLOT_STATE_GENERATING) {
                continue;
            }

            // check if we can batch this slot with the previous one
            if (!slot_batched) {
                slot_batched = &slot;
            } else if (!slot_batched->can_batch_with(slot)) {
                continue;
            }

            slot.i_batch = batch.n_tokens;

            common_batch_add(batch, slot.sampled, slot.n_past, { slot.id }, true);

            slot.n_past += 1;
            slot.cache_tokens.push_back(slot.sampled);

            SLT_DBG(slot, "slot decode token, n_ctx = %d, n_past = %d, n_cache_tokens = %d, truncated = %d\n",
                    slot.n_ctx, slot.n_past, (int) slot.cache_tokens.size(), slot.truncated);
        }

        // process in chunks of params.n_batch
        int32_t n_batch  = llama_n_batch(ctx);
        int32_t n_ubatch = llama_n_ubatch(ctx);

        // next, batch any pending prompts without exceeding n_batch
        if (params_base.cont_batching || batch.n_tokens == 0) {
            for (auto & slot : slots) {
                // check if we can batch this slot with the previous one
                if (slot.is_processing()) {
                    if (!slot_batched) {
                        slot_batched = &slot;
                    } else if (!slot_batched->can_batch_with(slot)) {
                        continue;
                    }
                }

                // this slot still has a prompt to be processed
                if (slot.state == SLOT_STATE_PROCESSING_PROMPT || slot.state == SLOT_STATE_STARTED) {
                    auto & prompt_tokens = slot.prompt_tokens;

                    // TODO: maybe move branch to outside of this loop in the future
                    if (slot.state == SLOT_STATE_STARTED) {
                        slot.t_start_process_prompt = ggml_time_us();
                        slot.t_start_generation = 0;

                        slot.n_past = 0;
                        slot.n_prompt_tokens = prompt_tokens.size();
                        slot.state = SLOT_STATE_PROCESSING_PROMPT;

                        SLT_INF(slot, "new prompt, n_ctx_slot = %d, n_keep = %d, n_prompt_tokens = %d\n", slot.n_ctx, slot.params.n_keep, slot.n_prompt_tokens);

                        // print prompt tokens (for debugging)
                        /*if (1) {
                            // first 16 tokens (avoid flooding logs)
                            for (int i = 0; i < std::min<int>(16, prompt_tokens.size()); i++) {
                                SLT_DBG(slot, "prompt token %3d: %6d '%s'\n", i, prompt_tokens[i], common_token_to_piece(ctx, prompt_tokens[i]).c_str());
                            }
                        } else {
                            // all
                            for (int i = 0; i < (int) prompt_tokens.size(); i++) {
                                SLT_DBG(slot, "prompt token %3d: %6d '%s'\n", i, prompt_tokens[i], common_token_to_piece(ctx, prompt_tokens[i]).c_str());
                            }
                        }*/

                        // empty prompt passed -> release the slot and send empty response
                        if (prompt_tokens.empty()) {
                            SLT_WRN(slot, "%s", "empty prompt - releasing slot\n");

                            slot.release();
                            slot.print_timings();
                            send_final_response(slot);
                            continue;
                        }

                        // TODO: support memory-less logits computation
                        if (slot.need_logits() && !llama_get_memory(ctx)) {
                            slot.release();
                            send_error(slot, "the current context does not logits computation. skipping", ERROR_TYPE_SERVER);
                            continue;
                        }

                        if (!slot.can_split()) {
                            if (slot.n_prompt_tokens > n_ubatch) {
                                slot.release();
                                send_error(slot, "input is too large to process. increase the physical batch size", ERROR_TYPE_SERVER);
                                continue;
                            }

                            if (slot.n_prompt_tokens > slot.n_ctx) {
                                slot.release();
                                send_error(slot, "input is larger than the max context size. skipping", ERROR_TYPE_SERVER);
                                continue;
                            }
                        } else {
                            if (!params_base.ctx_shift) {
                                // if context shift is disabled, we make sure prompt size is smaller than KV size
                                // TODO: there should be a separate parameter that control prompt truncation
                                //       context shift should be applied only during the generation phase
                                if (slot.n_prompt_tokens >= slot.n_ctx) {
                                    slot.release();
                                    send_error(slot, "the request exceeds the available context size. try increasing the context size or enable context shift", ERROR_TYPE_INVALID_REQUEST);
                                    continue;
                                }
                            }
                            if (slot.params.n_keep < 0) {
                                slot.params.n_keep = slot.n_prompt_tokens;
                            }
                            slot.params.n_keep = std::min(slot.n_ctx - 4, slot.params.n_keep);

                            // if input prompt is too big, truncate it
                            if (slot.n_prompt_tokens >= slot.n_ctx) {
                                if (mctx) {
                                    // we should never reach this
                                    GGML_ABORT("not supported by multimodal");
                                }
                                const int n_left = slot.n_ctx - slot.params.n_keep;

                                const int n_block_size = n_left / 2;
                                const int erased_blocks = (slot.n_prompt_tokens - slot.params.n_keep - n_block_size) / n_block_size;

                                const llama_tokens & curr_tokens = slot.prompt_tokens.get_text_tokens();
                                llama_tokens new_tokens(
                                        curr_tokens.begin(),
                                        curr_tokens.begin() + slot.params.n_keep);

                                new_tokens.insert(
                                        new_tokens.end(),
                                        curr_tokens.begin() + slot.params.n_keep + erased_blocks * n_block_size,
                                        curr_tokens.end());

                                prompt_tokens.clear();
                                prompt_tokens.insert(new_tokens);

                                slot.truncated = true;
                                slot.n_prompt_tokens = prompt_tokens.size();

                                SLT_WRN(slot, "input truncated, n_ctx = %d, n_keep = %d, n_left = %d, n_prompt_tokens = %d\n", slot.n_ctx, slot.params.n_keep, n_left, slot.n_prompt_tokens);

                                GGML_ASSERT(slot.n_prompt_tokens < slot.n_ctx);
                            }

                            if (slot.params.cache_prompt) {
                                // reuse any previously computed tokens that are common with the new prompt
                                slot.n_past = slot.cache_tokens.get_common_prefix(prompt_tokens);

                                // reuse chunks from the cached prompt by shifting their KV cache in the new position
                                if (params_base.n_cache_reuse > 0) {
                                    size_t head_c = slot.n_past; // cache
                                    size_t head_p = slot.n_past; // current prompt

                                    if (mctx) {
                                        // we should never reach this
                                        GGML_ABORT("not supported by multimodal");
                                    }

                                    SLT_DBG(slot, "trying to reuse chunks with size > %d, slot.n_past = %d\n", params_base.n_cache_reuse, slot.n_past);

                                    while (head_c < slot.cache_tokens.size() &&
                                           head_p < prompt_tokens.size()) {

                                        size_t n_match = 0;
                                        while (head_c + n_match < slot.cache_tokens.size() &&
                                               head_p + n_match < prompt_tokens.size()     &&
                                               slot.cache_tokens[head_c + n_match] == prompt_tokens[head_p + n_match]) {

                                            n_match++;
                                        }

                                        if (n_match >= (size_t) params_base.n_cache_reuse) {
                                            SLT_INF(slot, "reusing chunk with size %zu, shifting KV cache [%zu, %zu) -> [%zu, %zu)\n", n_match, head_c, head_c + n_match, head_p, head_p + n_match);
                                            //for (size_t i = head_p; i < head_p + n_match; i++) {
                                            //    SLT_DBG(slot, "cache token %3zu: %6d '%s'\n", i, prompt_tokens[i], common_token_to_piece(ctx, prompt_tokens[i]).c_str());
                                            //}

                                            const int64_t kv_shift = (int64_t) head_p - (int64_t) head_c;

                                            llama_memory_seq_rm (llama_get_memory(ctx), slot.id, head_p, head_c);
                                            llama_memory_seq_add(llama_get_memory(ctx), slot.id, head_c, head_c + n_match, kv_shift);

                                            for (size_t i = 0; i < n_match; i++) {
                                                slot.cache_tokens.set_token(head_p + i, slot.cache_tokens[head_c + i]);
                                                slot.n_past++;
                                            }

                                            head_c += n_match;
                                            head_p += n_match;
                                        } else {
                                            head_c += 1;
                                        }
                                    }

                                    SLT_DBG(slot, "after context reuse, new slot.n_past = %d\n", slot.n_past);
                                }
                            } else {
                                // if we don't cache the prompt, we have to remove the entire KV cache
                                slot.n_past = 0;
                            }

                            const auto n_swa = llama_model_n_swa(model);

                            if (slot.n_past > 0 && slot.n_past < (int) slot.cache_tokens.size()) {
                                const auto pos_min = llama_memory_seq_pos_min(llama_get_memory(ctx), slot.id);
                                if (pos_min == -1) {
                                    SLT_ERR(slot, "n_past = %d, cache_tokens.size() = %d, seq_id = %d, pos_min = %d\n", slot.n_past, (int) slot.cache_tokens.size(), slot.id, pos_min);
                                    GGML_ABORT("pos_min == -1, but n_past > 0 - should not happen: https://github.com/ggml-org/llama.cpp/pull/13833#discussion_r2116181237");
                                }

                                const auto pos_min_thold = std::max(0, slot.n_past - n_swa);

                                if (pos_min > pos_min_thold) {
                                    SLT_WRN(slot, "n_past = %d, cache_tokens.size() = %d, seq_id = %d, pos_min = %d, n_swa = %d\n", slot.n_past, (int) slot.cache_tokens.size(), slot.id, pos_min, n_swa);

                                    // search for a SWA checkpoint
                                    const auto it = std::find_if(
                                        slot.swa_checkpoints.rbegin(),
                                        slot.swa_checkpoints.rend(),
                                        [&](const auto & cur) {
                                            return cur.pos_min <= pos_min_thold;
                                        }
                                    );

                                    bool do_reset = it == slot.swa_checkpoints.rend();

                                    if (!do_reset) {
                                        // restore the checkpoint
                                        const size_t swa_size = it->data.size();
                                        const size_t n = llama_state_seq_set_data_ext(ctx, it->data.data(), swa_size, slot.id, LLAMA_STATE_SEQ_FLAGS_SWA_ONLY);

                                        if (n != swa_size) {
                                            SLT_ERR(slot, "failed to restore SWA checkpoint, pos_min = %d, pos_max = %d, size = %.3f MiB\n", it->pos_min, it->pos_max, (float) swa_size / 1024 / 1024);
                                            do_reset = true;
                                        } else {
                                            slot.n_past = std::min(slot.n_past, it->pos_max);

                                            SLT_WRN(slot, "SWA checkpoint restore, pos_min = %d, pos_max = %d, size = %.3f MiB\n", it->pos_min, it->pos_max, (float) swa_size / 1024 / 1024);
                                        }
                                    }

                                    if (do_reset) {
                                        SLT_WRN(slot, "forcing full prompt re-processing due to lack of cache data (likely due to SWA, see %s)\n",
                                                "https://github.com/ggml-org/llama.cpp/pull/13194#issuecomment-2868343055");

                                        slot.n_past = 0;
                                        slot.swa_checkpoints.clear();
                                    }
                                }
                            }

                            if (n_swa > 0) {
                                const auto pos_min_thold = std::max(0, slot.n_past - n_swa);

                                // erase any checkpoints with pos_min > pos_min_thold
                                for (int i = (int) slot.swa_checkpoints.size() - 1; i >= 0; i--) {
                                    const auto & cur = slot.swa_checkpoints[i];
                                    if (cur.pos_min > pos_min_thold) {
                                        slot.swa_checkpoints.erase(slot.swa_checkpoints.begin() + i);

                                        SLT_WRN(slot, "SWA checkpoint erase, pos_min = %d, pos_max = %d, size = %.3f MiB\n", cur.pos_min, cur.pos_max, (float) cur.data.size() / 1024 / 1024);
                                    }
                                }
                            }
                        }

                        if (slot.n_past == slot.n_prompt_tokens && slot.n_past > 0) {
                            SLT_WRN(slot, "need to evaluate at least 1 token for each active slot, n_past = %d, n_prompt_tokens = %d\n", slot.n_past, slot.n_prompt_tokens);

                            slot.n_past--;
                        }

                        slot.n_prompt_tokens_processed = 0;
                    }

                    if (!slot.can_split()) {
                        // cannot fit the prompt in the current batch - will try next iter
                        if (batch.n_tokens + slot.n_prompt_tokens > n_batch) {
                            continue;
                        }
                    }

                    // keep only the common part
                    if (!llama_memory_seq_rm(llama_get_memory(ctx), slot.id, slot.n_past, -1)) {
                        // could not partially delete (likely using a non-Transformer model)
                        llama_memory_seq_rm(llama_get_memory(ctx), slot.id, -1, -1);

                        // there is no common part left
                        slot.n_past = 0;
                    }

                    SLT_INF(slot, "kv cache rm [%d, end)\n", slot.n_past);

                    // remove the non-common part from the cache
                    slot.cache_tokens.keep_first(slot.n_past);

                    // check if we should process the image
                    if (slot.n_past < slot.n_prompt_tokens && slot.prompt_tokens[slot.n_past] == LLAMA_TOKEN_NULL) {
                        // process the image
                        int32_t new_n_past;
                        int32_t res = slot.prompt_tokens.process_chunk(ctx, mctx, slot.n_past, slot.id, new_n_past);
                        int32_t n_pos = new_n_past - slot.n_past;

                        if (res != 0) {
                            SLT_ERR(slot, "failed to process image, res = %d\n", res);
                            slot.release();
                            send_error(slot, "failed to process image", ERROR_TYPE_SERVER);
                            continue;
                        }

                        // add the image chunk to cache
                        {
                            const auto & chunk = slot.prompt_tokens.find_chunk(slot.n_past);
                            slot.cache_tokens.push_back(chunk.get()); // copy
                        }

                        slot.n_past                    += n_pos;
                        slot.n_prompt_tokens_processed += n_pos;
                    }

                    // add prompt tokens for processing in the current batch
                    while (slot.n_past < slot.n_prompt_tokens && batch.n_tokens < n_batch) {
                        // get next token to process
                        llama_token cur_tok = slot.prompt_tokens[slot.n_past];
                        if (cur_tok == LLAMA_TOKEN_NULL) {
                            break; // end of text chunk
                        }

                        // embedding requires all tokens in the batch to be output
                        const bool need_embd = server_task_type_need_embd(slot.task_type);

                        common_batch_add(batch, cur_tok, slot.n_past, { slot.id }, need_embd);
                        slot.cache_tokens.push_back(cur_tok);

                        slot.n_prompt_tokens_processed++;
                        slot.n_past++;
                    }

                    // SLT_INF(slot, "new cache_tokens: %s\n", slot.cache_tokens.str().c_str());

                    SLT_INF(slot, "prompt processing progress, n_past = %d, n_tokens = %d, progress = %f\n", slot.n_past, batch.n_tokens, (float) slot.n_prompt_tokens_processed / slot.n_prompt_tokens);

                    // entire prompt has been processed
                    if (slot.n_past == slot.n_prompt_tokens) {
                        slot.state = SLOT_STATE_DONE_PROMPT;

                        GGML_ASSERT(batch.n_tokens > 0);
                        GGML_ASSERT((size_t) slot.n_prompt_tokens == slot.prompt_tokens.size());

                        common_sampler_reset(slot.smpl);

                        // Process all prompt tokens through sampler system
                        for (int i = 0; i < slot.n_prompt_tokens; ++i) {
                            llama_token id = slot.prompt_tokens[i];
                            if (id != LLAMA_TOKEN_NULL) {
                                common_sampler_accept(slot.smpl, id, false);
                            }
                        }

                        // extract the logits only for the last token
                        batch.logits[batch.n_tokens - 1] = true;

                        slot.n_decoded = 0;
                        slot.i_batch   = batch.n_tokens - 1;

                        SLT_INF(slot, "prompt done, n_past = %d, n_tokens = %d\n", slot.n_past, batch.n_tokens);
                    }
                }

                if (batch.n_tokens >= n_batch) {
                    break;
                }
            }
        }

        if (batch.n_tokens == 0) {
            SRV_WRN("%s", "no tokens to decode\n");
            return;
        }

        SRV_DBG("decoding batch, n_tokens = %d\n", batch.n_tokens);

        if (slot_batched) {
            // apply lora, only need to do it once per batch
            common_set_adapter_lora(ctx, slot_batched->lora);

            llama_set_embeddings(ctx, slot_batched->need_embd());
        }

        int32_t i_next = 0;

        // process the created batch of tokens
        for (int32_t i = 0; i < batch.n_tokens; i = i_next) {
            const int32_t n_tokens = std::min(n_batch, batch.n_tokens - i);

            llama_batch batch_view = {
                n_tokens,
                batch.token    + i,
                nullptr,
                batch.pos      + i,
                batch.n_seq_id + i,
                batch.seq_id   + i,
                batch.logits   + i,
            };

            const int ret = llama_decode(ctx, batch_view);

            metrics.on_decoded(slots);

            if (ret != 0) {
                {
                    std::string err;

                    if (n_batch == 1 && ret == 1) {
                        err = "Context size has been exceeded.";
                    }

                    if (ret == -1) {
                        err = "Invalid input batch.";
                    }

                    if (ret < -1) {
                        // TODO: update slot state based on llama_memory_seq_pos_min() and llama_memory_seq_pos_max()
                        err = "Compute error.";
                    }

                    // TODO: handle ret == 2 (abort) when we start aborting

                    if (!err.empty()) {
                        SRV_ERR("%s, i = %d, n_batch = %d, ret = %d\n", err.c_str(), i, n_batch, ret);
                        for (auto & slot : slots) {
                            slot.release();
                            send_error(slot, err);
                        }
                        break;
                    }
                }

                // retry with half the batch size to try to find a free slot in the KV cache
                n_batch /= 2;

                SRV_WRN("failed to find free space in the KV cache, retrying with smaller batch size, i = %d, n_batch = %d, ret = %d\n", i, n_batch, ret);

                continue; // continue loop of n_batch
            }

            // move the head of the batch forward with the number of tokens we just processed
            i_next = i + n_tokens;

            // on successful decode, restore the original batch size
            n_batch = llama_n_batch(ctx);

            for (auto & slot : slots) {
                if (slot.i_batch < (int) i || slot.i_batch >= (int) (i + n_tokens)) {
                    continue; // continue loop of slots
                }

                if (slot.state == SLOT_STATE_DONE_PROMPT) {
                    if (slot.task_type == SERVER_TASK_TYPE_EMBEDDING) {
                        // prompt evaluated for embedding
                        send_embedding(slot, batch_view);
                        slot.release();
                        slot.i_batch = -1;
                        continue; // continue loop of slots
                    }

                    if (slot.task_type == SERVER_TASK_TYPE_RERANK) {
                        send_rerank(slot, batch_view);
                        slot.release();
                        slot.i_batch = -1;
                        continue; // continue loop of slots
                    }

                    // prompt evaluated for next-token prediction
                    slot.state = SLOT_STATE_GENERATING;

                    // make a checkpoint with the SWA memory
                    // checkpoints are needed only if we are not using "--swa-full"
                    if (llama_model_n_swa(model) > 0 && !params_base.swa_full && params_base.n_swa_checkpoints > 0) {
                        if (slot.swa_checkpoints.size() >= (size_t) params_base.n_swa_checkpoints) {
                            {
                                const auto & cur = slot.swa_checkpoints.back();

                                SLT_WRN(slot, "SWA checkpoint erase, pos_min = %d, pos_max = %d, size = %.3f MiB\n",
                                        cur.pos_min, cur.pos_max, (float) cur.data.size() / 1024 / 1024);
                            }

                            slot.swa_checkpoints.erase(slot.swa_checkpoints.begin());
                        }

                        const size_t swa_size = llama_state_seq_get_size_ext(ctx, slot.id, LLAMA_STATE_SEQ_FLAGS_SWA_ONLY);

                        auto & cur = slot.swa_checkpoints.emplace_back(swa_checkpoint{
                            /*.pos_min = */ llama_memory_seq_pos_min(llama_get_memory(ctx), slot.id),
                            /*.pos_max = */ llama_memory_seq_pos_max(llama_get_memory(ctx), slot.id),
                            /*.data    = */ std::vector<uint8_t>(swa_size),
                        });

                        llama_state_seq_get_data_ext(ctx, cur.data.data(), swa_size, slot.id, LLAMA_STATE_SEQ_FLAGS_SWA_ONLY);

                        float size_total = 0.0f;
                        for (const auto & checkpoint : slot.swa_checkpoints) {
                            size_total += (float) checkpoint.data.size() / 1024 / 1024;
                        }

                        SLT_WRN(slot, "SWA checkpoint create, pos_min = %d, pos_max = %d, size = %.3f MiB, total = %d/%d (%.3f MiB)\n",
                                cur.pos_min, cur.pos_max, (float) cur.data.size() / 1024 / 1024, (int) slot.swa_checkpoints.size(), params_base.n_swa_checkpoints, size_total);
                    }
                } else if (slot.state != SLOT_STATE_GENERATING) {
                    continue; // continue loop of slots
                }

                const int tok_idx = slot.i_batch - i;

                llama_token id = common_sampler_sample(slot.smpl, ctx, tok_idx);

                slot.i_batch = -1;

                common_sampler_accept(slot.smpl, id, true);

                slot.n_decoded += 1;

                const int64_t t_current = ggml_time_us();

                if (slot.n_decoded == 1) {
                    slot.t_start_generation = t_current;
                    slot.t_prompt_processing = (slot.t_start_generation - slot.t_start_process_prompt) / 1e3;
                    metrics.on_prompt_eval(slot);
                }

                slot.t_token_generation = (t_current - slot.t_start_generation) / 1e3;

                completion_token_output result;
                result.tok          = id;
                result.text_to_send = common_token_to_piece(ctx, result.tok, accept_special_token(slot, result.tok));
                result.prob         = 1.0f; // TODO: set it here instead of doing inside populate_token_probs

                if (slot.params.sampling.n_probs > 0) {
                    populate_token_probs(slot, result, slot.params.post_sampling_probs, params_base.special, tok_idx);
                }

                if (!process_token(result, slot)) {
                    // release slot because of stop condition
                    slot.release();
                    slot.print_timings();
                    send_final_response(slot);
                    metrics.on_prediction(slot);
                    continue;
                }
            }

            // do speculative decoding
            for (auto & slot : slots) {
                if (!slot.is_processing() || !slot.can_speculate()) {
                    continue;
                }

                if (slot.state != SLOT_STATE_GENERATING) {
                    continue;
                }

                if (mctx) {
                    // we should never reach this, as speculative is automatically disabled if mmproj is loaded
                    GGML_ABORT("not supported by multimodal");
                }

                // determine the max draft that fits the current slot state
                int n_draft_max = slot.params.speculative.n_max;

                // note: n_past is not yet increased for the `id` token sampled above
                //       also, need to leave space for 1 extra token to allow context shifts
                n_draft_max = std::min(n_draft_max, slot.n_ctx - slot.n_past - 2);

                if (slot.n_remaining > 0) {
                    n_draft_max = std::min(n_draft_max, slot.n_remaining - 1);
                }

                SLT_DBG(slot, "max possible draft: %d\n", n_draft_max);

                if (n_draft_max < slot.params.speculative.n_min) {
                    SLT_DBG(slot, "the max possible draft is too small: %d < %d - skipping speculative decoding\n", n_draft_max, slot.params.speculative.n_min);

                    continue;
                }

                llama_token id = slot.sampled;

                struct common_speculative_params params_spec;
                params_spec.n_draft   = n_draft_max;
                params_spec.n_reuse   = llama_n_ctx(slot.ctx_dft) - slot.params.speculative.n_max;
                params_spec.p_min     = slot.params.speculative.p_min;

                const llama_tokens & cached_text_tokens = slot.cache_tokens.get_text_tokens();
                llama_tokens draft = common_speculative_gen_draft(slot.spec, params_spec, cached_text_tokens, id);

                // ignore small drafts
                if (slot.params.speculative.n_min > (int) draft.size()) {
                    SLT_DBG(slot, "ignoring small draft: %d < %d\n", (int) draft.size(), slot.params.speculative.n_min);

                    continue;
                }

                // keep track of total number of drafted tokens tested
                slot.n_draft_total += draft.size();

                // construct the speculation batch
                common_batch_clear(slot.batch_spec);
                common_batch_add  (slot.batch_spec, id, slot.n_past, { slot.id }, true);

                for (size_t i = 0; i < draft.size(); ++i) {
                    common_batch_add(slot.batch_spec, draft[i], slot.n_past + 1 + i, { slot.id }, true);
                }

                SLT_DBG(slot, "decoding speculative batch, size = %d\n", slot.batch_spec.n_tokens);

                llama_decode(ctx, slot.batch_spec);

                // the accepted tokens from the speculation
                const auto ids = common_sampler_sample_and_accept_n(slot.smpl, ctx, draft);

                slot.n_past    += ids.size();
                slot.n_decoded += ids.size();

                // update how many tokens out of those tested were accepted
                slot.n_draft_accepted += ids.size() - 1;

                slot.cache_tokens.push_back(id);
                slot.cache_tokens.insert({ids.begin(), ids.end() - 1});

                llama_memory_seq_rm(llama_get_memory(ctx), slot.id, slot.n_past, -1);

                for (size_t i = 0; i < ids.size(); ++i) {
                    completion_token_output result;

                    result.tok          = ids[i];
                    result.text_to_send = common_token_to_piece(ctx, result.tok, accept_special_token(slot, result.tok));
                    result.prob         = 1.0f; // set later

                    // TODO: set result.probs

                    if (!process_token(result, slot)) {
                        // release slot because of stop condition
                        slot.release();
                        slot.print_timings();
                        send_final_response(slot);
                        metrics.on_prediction(slot);
                        break;
                    }
                }

                SLT_DBG(slot, "accepted %d/%d draft tokens, new n_past = %d\n", (int) ids.size() - 1, (int) draft.size(), slot.n_past);
            }
        }

        SRV_DBG("%s", "run slots completed\n");
    }

    json model_meta() const {
        return json {
            {"vocab_type",  llama_vocab_type       (vocab)},
            {"n_vocab",     llama_vocab_n_tokens   (vocab)},
            {"n_ctx_train", llama_model_n_ctx_train(model)},
            {"n_embd",      llama_model_n_embd     (model)},
            {"n_params",    llama_model_n_params   (model)},
            {"size",        llama_model_size       (model)},
        };
    }
};

static void log_server_request(const httplib::Request & req, const httplib::Response & res) {
    // skip GH copilot requests when using default port
    if (req.path == "/v1/health" || req.path == "/v1/completions") {
        return;
    }

    // reminder: this function is not covered by httplib's exception handler; if someone does more complicated stuff, think about wrapping it in try-catch

    SRV_INF("request: %s %s %s %d\n", req.method.c_str(), req.path.c_str(), req.remote_addr.c_str(), res.status);

    SRV_DBG("request:  %s\n", req.body.c_str());
    SRV_DBG("response: %s\n", res.body.c_str());
}

std::function<void(int)> shutdown_handler;
std::atomic_flag is_terminating = ATOMIC_FLAG_INIT;

inline void signal_handler(int signal) {
    if (is_terminating.test_and_set()) {
        // in case it hangs, we can force terminate the server by hitting Ctrl+C twice
        // this is for better developer experience, we can remove when the server is stable enough
        fprintf(stderr, "Received second interrupt, terminating immediately.\n");
        exit(1);
    }

    shutdown_handler(signal);
}

/*
 * main函数 - 程序的入口点
 * 参数说明:
 * - argc: 命令行参数的数量
 * - argv: 命令行参数的字符串数组
 * 返回值: 0表示成功，1表示失败
 */
int main(int argc, char ** argv) {
    /*
     * 步骤1: 初始化参数结构体
     * common_params是一个结构体，用来存储服务器运行所需的所有配置参数，即通过参数结构体来控制服务器运行状态，目前有一些默认的参数配置，需求解析命令行进行填充。
     * 比如端口号、模型文件路径、线程数等等，在 server 目录中的 README.md 文件中罗列出来了所有的可用命令行参数，也是可以通过命令行打印出来。
     */
    common_params params;

    /*
     * 步骤2: 解析命令行参数
     * 这个函数会读取用户在命令行输入的参数(如 --port 8080 --model model.gguf)
     * 并将这些参数存储到params结构体中
     * LLAMA_EXAMPLE_SERVER指定这是server模式的参数解析
     * 如果参数解析失败(比如用户输入了无效参数)，返回1退出程序
     */
    if (!common_params_parse(argc, argv, params, LLAMA_EXAMPLE_SERVER)) {
        return 1;
    }

    /*
     * 步骤3: 通用初始化
     * 执行一些基础的初始化工作，比如设置日志系统
     */
    common_init();

    /*
     * 步骤4: 创建服务器上下文对象
     * server_context是一个包含所有服务器状态的结构体
     * 包括模型加载状态、推理上下文、任务队列等
     * 这个对象是整个服务器的核心数据结构
     */
    server_context ctx_server;

    /*
     * 步骤5: 初始化llama后端
     * 这个函数初始化llama.cpp库的底层组件
     * 为后续的模型加载和推理做准备
     */
    llama_backend_init();
    
    /*
     * 步骤6: 初始化NUMA (Non-Uniform Memory Access) 支持
     * NUMA是一种多处理器系统的内存架构，这里的处理器系统指的就是多物理CPU处理器。
     * 这个函数根据用户配置来优化内存访问性能
     * params.numa包含了NUMA相关的配置参数
     */
    llama_numa_init(params.numa);

    /*
     * 步骤7: 打印系统信息到日志
     * 这些信息对于调试和性能优化非常有用:
     * - n_threads: 用于推理的线程数
     * - n_threads_batch: 用于批处理的线程数  
     * - total_threads: 系统总的可用CPU线程数
     */
    LOG_INF("system info: n_threads = %d, n_threads_batch = %d, total_threads = %d\n", params.cpuparams.n_threads, params.cpuparams_batch.n_threads, std::thread::hardware_concurrency());
    LOG_INF("\n");
    
    /*
     * 打印详细的系统信息，包括CPU型号、内存大小等
     * 这些信息有助于用户了解服务器运行环境
     */
    LOG_INF("%s\n", common_params_get_system_info(params).c_str());
    LOG_INF("\n");

    /*
     * 步骤8: 创建HTTP服务器对象
     * 使用std::unique_ptr智能指针来管理服务器生命周期
     * 这里需要判断是否启用SSL加密
     */
    std::unique_ptr<httplib::Server> svr;
    
/*
 * SSL支持检查: 编译时定义的宏，用来检查是否支持OpenSSL，OpenSSL 是为了 client 和 server 之间建立更安全的通信。
 */
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    /*
     * 如果用户提供了SSL私钥和证书文件，就创建SSL服务器
     * SSL(安全套接字层)提供HTTPS加密通信功能
     * 证书文件用于证明服务器身份，私钥用于加密解密
     */
    if (params.ssl_file_key != "" && params.ssl_file_cert != "") {
        LOG_INF("Running with SSL: key = %s, cert = %s\n", params.ssl_file_key.c_str(), params.ssl_file_cert.c_str());
        svr.reset(
            new httplib::SSLServer(params.ssl_file_cert.c_str(), params.ssl_file_key.c_str())
        );
    } else {
        /*
         * 如果没有提供SSL证书，就创建普通的HTTP服务器
         * 这种情况下通信不加密，不适合生产环境
         */
        LOG_INF("Running without SSL\n");
        svr.reset(new httplib::Server());
    }
#else
    /*
     * 如果编译时没有包含SSL支持，但用户又提供了SSL证书
     * 就报错并退出，因为无法满足用户的SSL需求
     */
    if (params.ssl_file_key != "" && params.ssl_file_cert != "") {
        LOG_ERR("Server is built without SSL support\n");
        return 1;
    }
    /*
     * 没有SSL支持且用户也没要求SSL，就创建普通的HTTP服务器，如果是内网部署的话那么不需要使用 OpenSSL 使其安全性更好，这样可以节省算力资源消耗。
     */
    svr.reset(new httplib::Server());
#endif

    /*
     * 步骤9: 初始化服务器状态
     * 使用std::atomic保证在多线程环境下状态访问的线程安全性，目前还不知道为什么必须使用std::atomic
     * SERVER_STATE_LOADING_MODEL表示服务器初始状态为“正在加载模型”
     * 这个状态会在模型加载完成后改变为“就绪”状态
     */
    std::atomic<server_state> state{SERVER_STATE_LOADING_MODEL};

    /*
     * 步骤10: 设置服务器默认HTTP头
     * 在所有HTTP响应中都会包含"Server: llama.cpp"头
     * 这有助于客户端识别服务器类型和版本
     */
    svr->set_default_headers({{"Server", "llama.cpp"}});
    
    /*
     * 设置请求日志记录器
     * log_server_request函数会记录所有进入的HTTP请求
     * 包括请求方法、URL、响应状态码等信息，方便调试和监控
     */
    svr->set_logger(log_server_request);

    /*
     * 步骤11: 定义错误响应处理函数 (Lambda表达式)
     * 这个lambda函数用于统一处理错误响应的格式
     * 参数:
     * - res: HTTP响应对象，用于设置响应内容和状态码
     * - error_data: 错误信息的JSON数据
     * 功能: 将错误信息包装为标准的JSON格式并返回给客户端
     */
    auto res_error = [](httplib::Response & res, const json & error_data) {
        /* 将错误数据包装在"error"字段中，形成标准的错误响应格式 */
        json final_response {{"error", error_data}};
        /* 设置响应内容为JSON格式，并指定内容类型 */
        res.set_content(safe_json_to_str(final_response), MIMETYPE_JSON);
        /* 从错误数据中提取状态码，如果没有则默认使用500(内部服务器错误) */
        res.status = json_value(error_data, "code", 500);
    };

    /*
     * 定义成功响应处理函数 (Lambda表达式)
     * 这个lambda函数用于统一处理成功响应的格式
     * 参数:
     * - res: HTTP响应对象
     * - data: 要返回给客户端的JSON数据
     * 功能: 将数据转换为JSON格式并设置200状态码(成功)
     */
    auto res_ok = [](httplib::Response & res, const json & data) {
        /* 设置响应内容为JSON格式 */
        res.set_content(safe_json_to_str(data), MIMETYPE_JSON);
        /* 设置200状态码表示请求成功 */
        res.status = 200;
    };

    /*
     * 步骤12: 设置全局异常处理器
     * 当服务器处理请求时发生未捕获的异常时，这个处理器会被调用
     * 它保证服务器不会因为异常而崩溃，而是返回友好的错误响应
     * 参数:
     * - req: HTTP请求对象(这里没使用所以用_占位)
     * - res: HTTP响应对象，用于返回错误信息
     * - ep: 异常指针，包含了具体的异常信息
     */
    svr->set_exception_handler([&res_error](const httplib::Request &, httplib::Response & res, const std::exception_ptr & ep) {
        std::string message;
        try {
            /*
             * 重新抛出异常，这样可以捕获到具体的异常类型
             * 这是一个C++标准做法，用于处理std::exception_ptr
             */
            std::rethrow_exception(ep);
        } catch (const std::exception & e) {
            /* 捕获标准的C++异常，获取其错误消息 */
            message = e.what();
        } catch (...) {
            /* 捕获所有其他类型的异常(非标准异常) */
            message = "Unknown Exception";
        }

        try {
            /*
             * 将异常信息格式化为标准的错误响应格式
             * ERROR_TYPE_SERVER表示这是服务器内部错误
             */
            json formatted_error = format_error_response(message, ERROR_TYPE_SERVER);
            /* 记录警告日志，方便开发者调试 */
            LOG_WRN("got exception: %s\n", formatted_error.dump().c_str());
            /* 使用统一的错误响应处理函数 */
            res_error(res, formatted_error);
        } catch (const std::exception & e) {
            /*
             * 如果在处理异常时又发生了异常(双重异常)
             * 记录错误日志，这通常表示严重的程序问题
             */
            LOG_ERR("got another exception: %s | while hanlding exception: %s\n", e.what(), message.c_str());
        }
    });

    /*
     * 步骤13: 设置HTTP错误处理器
     * 当HTTP请求发生错误(如404、500等)时，这个处理器会被调用
     * 它主要处理一些特定的HTTP状态码，提供更友好的错误信息
     */
    svr->set_error_handler([&res_error](const httplib::Request &, httplib::Response & res) {
        /*
         * 特别处理404错误(找不到文件)
         * 返回标准化的JSON错误响应而不是简单的HTML页面
         */
        if (res.status == 404) {
            res_error(res, format_error_response("File Not Found", ERROR_TYPE_NOT_FOUND));
        }
        /*
         * 对于其他错误码，我们不在这里处理
         * 因为它们通常已经通过res_error()函数处理过了
         */
    });

    /*
     * 步骤14: 设置服务器超时参数
     * 超时设置对于防止请求太慢或客户端无响应非常重要
     */
    /* 设置读取超时: 服务器等待读取客户端数据的最长时间 */
    svr->set_read_timeout (params.timeout_read);
    /* 设置写入超时: 服务器等待向客户端发送数据的最长时间 */
    svr->set_write_timeout(params.timeout_write);

    /*
     * 步骤15: 准备日志数据
     * 创建一个哈希表来存储要记录到日志中的关键信息
     * 这些信息在服务器启动时会被打印，方便管理员检查配置
     */
    std::unordered_map<std::string, std::string> log_data;

    /* 记录服务器的主机名(如localhost或IP地址) */
    log_data["hostname"] = params.hostname;
    /* 记录服务器的端口号(转换为字符串以便存储) */
    log_data["port"]     = std::to_string(params.port);

    /*
     * 步骤16: 处理API密钥日志信息
     * 出于安全考虑，不能在日志中显示完整的API密钥
     * 只显示部分信息或统计数据
     */
    if (params.api_keys.size() == 1) {
        /* 如果只有一个API密钥，只显示后4位字符，其余用****隐藏 */
        auto key = params.api_keys[0];
        log_data["api_key"] = "api_key: ****" + key.substr(std::max((int)(key.length() - 4), 0));
    } else if (params.api_keys.size() > 1) {
        /* 如果有多个API密钥，只显示数量而不显示具体内容 */
        log_data["api_key"] = "api_key: " + std::to_string(params.api_keys.size()) + " keys loaded";
    }

    /*
     * 步骤17: 设置提示相似性阈值
     * 这个参数用于智能选择处理插槽(slot)
     * 当新请求的提示与某个插槽中的提示相似度超过这个阈值时
     * 可以复用该插槽，提高性能和资源利用率
     */
    ctx_server.slot_prompt_similarity = params.slot_prompt_similarity;

    /*
     * ================================================================
     * 中间件设置部分 (Middlewares)
     * ================================================================
     * 中间件是在处理HTTP请求之前执行的函数
     * 它们可以用于验证、权限检查、日志记录等
     */

    /*
     * 步骤18: 定义API密钥验证中间件
     * 这个中间件负责验证客户端提供的API密钥是否有效
     * 它会在每个需要身份验证的请求之前被调用
     * 
     * 参数:
     * - params: 包含服务器配置的参数对象(包括允许的API密钥列表)
     * - res_error: 错误响应处理函数
     * 返回值: true表示验证通过，false表示验证失败
     */
    auto middleware_validate_api_key = [&params, &res_error](const httplib::Request & req, httplib::Response & res) {
        /*
         * 定义公开端点列表
         * 这些端点不需要API密钥验证，任何人都可以访问
         * static关键字保证这个列表只初始化一次，提高性能
         */
        static const std::unordered_set<std::string> public_endpoints = {
            "/health",      /* 健康检查端点 */
            "/models",      /* 模型信息端点 */
            "/v1/models",   /* OpenAI兼容的模型信息端点 */
            "/api/tags"     /* 标签信息端点 */
        };

        /*
         * 检查是否启用了API密钥验证
         * 如果管理员没有配置API密钥，则跳过验证，允许所有请求
         * 这种情况下服务器就是完全开放的
         */
        if (params.api_keys.empty()) {
            return true;
        }

        /*
         * 检查请求的路径是否为公开端点
         * 公开端点和首页("/")不需要API密钥验证
         * find()函数在集合中查找元素，未找到时返回end()
         */
        if (public_endpoints.find(req.path) != public_endpoints.end() || req.path == "/") {
            return true;
        }

        /*
         * 从请求头中获取Authorization字段
         * 按照HTTP标准，API密钥通常以"Bearer 密钥"的格式传递
         */
        auto auth_header = req.get_header_value("Authorization");

        /*
         * 检查Authorization头是否以"Bearer "开头
         * Bearer是一种标准的HTTP身份验证方式
         */
        std::string prefix = "Bearer ";
        if (auth_header.substr(0, prefix.size()) == prefix) {
            /*
             * 提取实际的API密钥(去掉"Bearer "前缀)
             * substr()函数从指定位置开始截取子字符串
             */
            std::string received_api_key = auth_header.substr(prefix.size());
            /*
             * 在允许的API密钥列表中查找接收到的密钥
             * std::find()在容器中查找元素，找到则返回迭代器
             */
            if (std::find(params.api_keys.begin(), params.api_keys.end(), received_api_key) != params.api_keys.end()) {
                return true; /* API密钥有效，验证通过 */
            }
        }

        /*
         * 执行到这里说明API密钥无效或未提供
         * 使用统一的错误响应处理函数返回身份验证错误
         */
        res_error(res, format_error_response("Invalid API Key", ERROR_TYPE_AUTHENTICATION));

        /* 记录未授权访问的警告日志 */
        LOG_WRN("Unauthorized: Invalid API Key\n");

        return false; /* 验证失败 */
    };

    /*
     * 步骤19: 定义服务器状态检查中间件
     * 这个中间件负责检查服务器当前的状态
     * 在模型加载期间，大部分请求都会被拒绝或返回特殊页面
     * 
     * 参数:
     * - res_error: 错误响应处理函数
     * - state: 服务器当前状态的原子变量引用
     * 返回值: true表示允许继续处理请求，false表示已处理完成，无需继续
     */
    auto middleware_server_state = [&res_error, &state](const httplib::Request & req, httplib::Response & res) {
        /*
         * 获取服务器当前状态
         * load()是原子操作，保证在多线程环境下安全读取
         */
        server_state current_state = state.load();
        
        /*
         * 如果服务器正在加载模型，需要特殊处理
         * 在这个状态下，服务器还不能提供正常的AI服务
         */
        if (current_state == SERVER_STATE_LOADING_MODEL) {
            /*
             * 解析请求路径的文件扩展名
             * string_split函数以'.'为分隔符切分路径
             * 这样可以判断请求的是静态文件还是API接口
             */
            auto tmp = string_split<std::string>(req.path, '.');
            
            /*
             * 对于首页或HTML文件请求，返回加载页面
             * 这是一个特殊的等待页面，告诉用户模型正在加载
             */
            if (req.path == "/" || tmp.back() == "html") {
                /*
                 * 设置响应内容为内嵌的HTML加载页面
                 * loading_html是编译时嵌入的静态HTML数据
                 * reinterpret_cast将字节数据转换为字符数据
                 */
                res.set_content(reinterpret_cast<const char*>(loading_html), loading_html_len, "text/html; charset=utf-8");
                /*
                 * 设置503状态码(服务不可用)
                 * 这告诉客户端服务器暂时不可用，请稍后重试
                 */
                res.status = 503;
            } else if (req.path == "/models" || req.path == "/v1/models" || req.path == "/api/tags") {
                /*
                 * 特殊情况: 允许在加载期间访问模型信息端点
                 * 这些端点不需要模型就能返回基本信息
                 * 返回true表示继续处理请求，不在这里拦截
                 */
                return true;
            } else {
                /*
                 * 对于其他所有API请求，返回“服务不可用”错误
                 * 因为模型还没加载完成，无法提供AI推理服务
                 */
                res_error(res, format_error_response("Loading model", ERROR_TYPE_UNAVAILABLE));
            }
            /*
             * 返回false表示请求已经被处理完成
             * 不需要继续传递给后续的路由处理器
             */
            return false;
        }
        /*
         * 如果服务器处于正常状态(模型已加载)
         * 返回true允许请求继续处理
         */
        return true;
    };

    /*
     * 步骤20: 注册中间件到HTTP服务器
     * set_pre_routing_handler设置一个在路由匹配之前执行的处理器
     * 所有进入的HTTP请求都会先经过这个处理器
     * 
     * 处理器的执行顺序:
     * 1. CORS跨域处理
     * 2. 服务器状态检查
     * 3. API密钥验证
     */
    svr->set_pre_routing_handler([&middleware_validate_api_key, &middleware_server_state](const httplib::Request & req, httplib::Response & res) {
        /*
         * 步骤21: 处理CORS(跨域资源共享)请求
         * CORS允许前端网页从不同的域名访问这个API服务器
         * 这对于Web应用的前后端分离非常重要
         */
        res.set_header("Access-Control-Allow-Origin", req.get_header_value("Origin"));
        
        /*
         * 特殊处理OPTIONS请求(预检请求)
         * 浏览器在发送实际的CORS请求之前会发送OPTIONS请求
         * 来检查服务器是否允许跨域访问
         */
        if (req.method == "OPTIONS") {
            /* 告诉浏览器允许携带身份验证信息(如cookies、身份验证头) */
            res.set_header("Access-Control-Allow-Credentials", "true");
            /* 指定允许的HTTP方法 */
            res.set_header("Access-Control-Allow-Methods",     "GET, POST");
            /* 允许所有请求头(通配符*) */
            res.set_header("Access-Control-Allow-Headers",     "*");
            /* 返回空内容，OPTIONS请求不需要实际数据 */
            res.set_content("", "text/html");
            /*
             * 返回Handled表示请求已处理完成
             * 跳过后续的中间件和路由处理
             */
            return httplib::Server::HandlerResponse::Handled;
        }
        
        /*
         * 步骤22: 执行服务器状态检查中间件
         * 如果服务器正在加载模型或其他不可用状态
         * 就会在这里被拦截并返回相应的错误或等待页面
         */
        if (!middleware_server_state(req, res)) {
            /* 如果中间件返回false，说明请求已处理，不需继续 */
            return httplib::Server::HandlerResponse::Handled;
        }
        
        /*
         * 步骤23: 执行API密钥验证中间件
         * 如果启用了API密钥验证，会在这里检查请求的授权信息
         * 无效的API密钥请求会被拒绝
         */
        if (!middleware_validate_api_key(req, res)) {
            /* 如果API密钥验证失败，请求被拒绝 */
            return httplib::Server::HandlerResponse::Handled;
        }
        
        /*
         * 所有中间件检查都通过了
         * 返回Unhandled表示请求可以继续传递给路由处理器
         */
        return httplib::Server::HandlerResponse::Unhandled;
    });

    /*
     * ================================================================
     * 路由处理器部分 (Route Handlers / Controllers)
     * ================================================================
     * 路由处理器是处理具体HTTP请求的函数
     * 每个处理器对应一个或多个API端点
     * 它们实现了服务器的核心业务逻辑
     */

    /*
     * 步骤24: 定义健康检查处理器
     * 这个处理器提供一个简单的健康检查端点
     * 通常用于负载均衡器、监控系统或容器编排器检查服务器状态
     * 
     * API端点: GET /health
     * 响应格式: {"status": "ok"}
     * 特点: 不需要API密钥，在模型加载期间也可访问
     */
    const auto handle_health = [&](const httplib::Request &, httplib::Response & res) {
        /*
         * 错误和加载状态已经在中间件中处理了
         * 如果能执行到这里，说明服务器状态正常
         * 直接返回成功状态
         */
        json health = {{"status", "ok"}};
        res_ok(res, health); /* 使用统一的成功响应处理函数 */
    };

    /*
     * 步骤25: 定义插槽管理处理器
     * 插槽(Slot)是服务器用来并发处理多个推理请求的机制
     * 每个插槽可以独立处理一个对话或推理任务
     * 这个端点提供插槽的实时状态信息，用于监控和调试
     * 
     * API端点: GET /slots
     * 可选参数: fail_on_no_slot=true (如果没有空闲插槽就返回错误)
     * 响应格式: 插槽状态数据的JSON数组
     */
    const auto handle_slots = [&](const httplib::Request & req, httplib::Response & res) {
        /*
         * 检查是否启用了插槽端点
         * 管理员需要在启动服务器时使用--slots参数才能启用这个功能
         * 这样设计可以减少不必要的计算开销
         */
        if (!params.endpoint_slots) {
            res_error(res, format_error_response("This server does not support slots endpoint. Start it with `--slots`", ERROR_TYPE_NOT_SUPPORTED));
            return;
        }

        /*
         * 使用任务队列机制请求插槽数据
         * 这是一种异步设计模式，避免阻塞HTTP线程
         * 先获取一个唯一的任务ID
         */
        int task_id = ctx_server.queue_tasks.get_new_id();
        {
            /*
             * 创建一个类型为SERVER_TASK_TYPE_METRICS的任务
             * 这个任务类型专门用于收集服务器指标和插槽状态
             */
            server_task task(SERVER_TASK_TYPE_METRICS);
            task.id = task_id;
            /*
             * 将任务ID添加到等待结果的列表中
             * 这样当任务完成时可以获得通知
             */
            ctx_server.queue_results.add_waiting_task_id(task_id);
            /*
             * 将任务提交到任务队列
             * true参数表示这是高优先级任务，会被优先处理
             * std::move用于移动语义，避免不必要的对象复制
             */
            ctx_server.queue_tasks.post(std::move(task), true);
        }

        /*
         * 等待任务执行完成并获取结果
         * recv()函数会阻塞当前线程直到任务完成
         * 返回的是一个智能指针，包含任务的执行结果
         */
        server_task_result_ptr result = ctx_server.queue_results.recv(task_id);
        /*
         * 任务完成后从等待列表中移除该任务ID
         * 释放相关的内存资源
         */
        ctx_server.queue_results.remove_waiting_task_id(task_id);

        /*
         * 检查任务执行是否出错
         * 如果出错，直接返回错误信息给客户端
         */
        if (result->is_error()) {
            res_error(res, result->to_json());
            return;
        }

        /*
         * 将通用的任务结果转换为具体的指标结果类型
         * dynamic_cast是运行时类型转换，可以安全地转换类型
         * TODO注释表示这是一个需要优化的地方
         */
        auto res_metrics = dynamic_cast<server_task_result_metrics*>(result.get());
        /* 断言检查，确保类型转换成功 */
        GGML_ASSERT(res_metrics != nullptr);

        /*
         * 处理可选的fail_on_no_slot参数
         * 如果客户端设置了这个参数且当前没有空闲插槽
         * 就返回错误而不是正常的状态信息
         * 这对于客户端的负载均衡很有用
         */
        if (req.has_param("fail_on_no_slot")) {
            if (res_metrics->n_idle_slots == 0) {
                res_error(res, format_error_response("no slot available", ERROR_TYPE_UNAVAILABLE));
                return;
            }
        }

        /*
         * 返回插槽状态数据
         * slots_data包含所有插槽的详细信息，如占用状态、处理进度等
         */
        res_ok(res, res_metrics->slots_data);
    };

    /*
     * 步骤26: 定义指标监控处理器
     * 这个处理器提供服务器的详细性能指标
     * 主要用于监控系统(Prometheus)和性能分析
     * 返回的数据包括处理速度、令牌数量、请求状态等
     * 
     * API端点: GET /metrics
     * 响应格式: Prometheus格式的文本数据
     * 特点: 需要在启动时使用--metrics参数才能开启
     */
    const auto handle_metrics = [&](const httplib::Request &, httplib::Response & res) {
        /*
         * 检查是否启用了指标端点
         * 管理员需要在启动服务器时使用--metrics参数才能启用这个功能
         * 这样设计可以避免不必要的性能开销，因为统计数据需要额外的计算
         */
        if (!params.endpoint_metrics) {
            res_error(res, format_error_response("This server does not support metrics endpoint. Start it with `--metrics`", ERROR_TYPE_NOT_SUPPORTED));
            return;
        }

        /*
         * 使用任务队列机制请求指标数据
         * 这与插槽处理器类似，使用相同的异步模式
         * 避免阻塞HTTP处理线程，提高并发性能
         */
        int task_id = ctx_server.queue_tasks.get_new_id();
        {
            /*
             * 创建指标收集任务
             * SERVER_TASK_TYPE_METRICS类型的任务专门用于收集各种性能指标
             * 包括处理时间、令牌数量、插槽使用情况等
             */
            server_task task(SERVER_TASK_TYPE_METRICS);
            task.id = task_id;
            /* 将任务ID添加到等待结果的列表中 */
            ctx_server.queue_results.add_waiting_task_id(task_id);
            /*
             * 提交高优先级任务
             * 指标收集通常需要快速响应，因为监控系统会定期轮询
             */
            ctx_server.queue_tasks.post(std::move(task), true);
        }

        /*
         * 等待指标收集任务完成并获取结果
         * 这里会阻塞直到服务器内部完成所有指标的计算
         */
        server_task_result_ptr result = ctx_server.queue_results.recv(task_id);
        /* 清理任务ID，释放内存资源 */
        ctx_server.queue_results.remove_waiting_task_id(task_id);

        /*
         * 检查指标收集是否成功
         * 如果失败，可能是由于服务器内部错误或资源不足
         */
        if (result->is_error()) {
            res_error(res, result->to_json());
            return;
        }

        /*
         * 将通用任务结果转换为具体的指标结果类型
         * 这里使用dynamic_cast是为了安全地访问指标数据
         * TODO注释表示这是一个可以优化的地方，可能有更高效的做法
         */
        auto res_metrics = dynamic_cast<server_task_result_metrics*>(result.get());
        /* 断言检查，确保类型转换成功 */
        GGML_ASSERT(res_metrics != nullptr);

        /*
         * 定义所有指标的结构和数据
         * 按照Prometheus标准进行命名和分类
         * 参考: https://prometheus.io/docs/practices/naming/#metric-names
         * 
         * 指标类型说明:
         * - counter: 计数器，只增不减的指标(如总请求数)
         * - gauge: 仪表，可以上下波动的指标(如当前连接数)
         */
        json all_metrics_def = json {
            /*
             * Counter类型指标 - 累积性数据，用于衡量总量和趋势
             */
            {"counter", {{
                /*
                 * 提示令牌总数 - 服务器启动以来处理的所有提示令牌数量
                 * 用于衡量服务器的工作负载和处理量
                 */
                    {"name",  "prompt_tokens_total"},
                    {"help",  "Number of prompt tokens processed."},
                    {"value",  (uint64_t) res_metrics->n_prompt_tokens_processed_total}
            }, {
                /*
                 * 提示处理总时间 - 处理所有提示所花费的总秒数
                 * 除以1000是为了将毫秒转换为秒
                 */
                    {"name",  "prompt_seconds_total"},
                    {"help",  "Prompt process time"},
                    {"value",  (uint64_t) res_metrics->t_prompt_processing_total / 1.e3}
            }, {
                /*
                 * 生成令牌总数 - 服务器生成的所有回复令牌数量
                 * 用于衡量输出的总量
                 */
                    {"name",  "tokens_predicted_total"},
                    {"help",  "Number of generation tokens processed."},
                    {"value",  (uint64_t) res_metrics->n_tokens_predicted_total}
            }, {
                /*
                 * 令牌生成总时间 - 生成所有令牌所花费的总秒数
                 * 用于计算平均生成速度
                 */
                    {"name",  "tokens_predicted_seconds_total"},
                    {"help",  "Predict process time"},
                    {"value",  (uint64_t) res_metrics->t_tokens_generation_total / 1.e3}
            }, {
                /*
                 * 解码调用总次数 - llama_decode()函数的调用次数
                 * 这是核心推理函数，反映模型的实际工作量
                 */
                    {"name",  "n_decode_total"},
                    {"help",  "Total number of llama_decode() calls"},
                    {"value",  res_metrics->n_decode_total}
            }, {
                /*
                 * 历史最大上下文长度 - 曾经处理过的最长对话上下文
                 * n_past表示已处理的令牌数，反映内存使用情况
                 */
                    {"name",  "n_past_max"},
                    {"help",  "Largest observed n_past."},
                    {"value",  res_metrics->n_past_max}
            }, {
                /*
                 * 平均繁忙插槽数 - 每次解码时平均有多少个插槽在工作
                 * 用于衡量并发处理的效率，防止除零错误
                 */
                    {"name",  "n_busy_slots_per_decode"},
                    {"help",  "Average number of busy slots per llama_decode() call"},
                    {"value",  (float) res_metrics->n_busy_slots_total / std::max((float) res_metrics->n_decode_total, 1.f)}
            }}},
            /*
             * Gauge类型指标 - 即时数据，反映当前状态
             */
            {"gauge", {{
                /*
                 * 提示处理速度 - 每秒处理的提示令牌数
                 * 计算公式: 令牌数 / (处理时间/1000)
                 * 三元运算符避免除零错误
                 */
                    {"name",  "prompt_tokens_seconds"},
                    {"help",  "Average prompt throughput in tokens/s."},
                    {"value",  res_metrics->n_prompt_tokens_processed ? 1.e3 / res_metrics->t_prompt_processing * res_metrics->n_prompt_tokens_processed : 0.}
            },{
                /*
                 * 令牌生成速度 - 每秒生成的令牌数
                 * 这是衡量模型性能的关键指标
                 */
                    {"name",  "predicted_tokens_seconds"},
                    {"help",  "Average generation throughput in tokens/s."},
                    {"value",  res_metrics->n_tokens_predicted ? 1.e3 / res_metrics->t_tokens_generation * res_metrics->n_tokens_predicted : 0.}
            },{
                /*
                 * 当前处理中的请求数 - 正在被处理的请求数量
                 * 反映服务器的实时负载
                 */
                    {"name",  "requests_processing"},
                    {"help",  "Number of requests processing."},
                    {"value",  (uint64_t) res_metrics->n_processing_slots}
            },{
                /*
                 * 延迟处理的请求数 - 等待处理的请求数量
                 * 如果这个数值过高，说明服务器负载过重
                 */
                    {"name",  "requests_deferred"},
                    {"help",  "Number of requests deferred."},
                    {"value",  (uint64_t) res_metrics->n_tasks_deferred}
            }}}
        };

        /*
         * 创建字符串流用于构建Prometheus格式的输出
         * Prometheus是一种标准的监控数据交换格式
         * 支持大多数监控系统如Grafana、Datadog等
         */
        std::stringstream prometheus;

        /*
         * 遍历所有指标类型(counter和gauge)
         * 将JSON结构转换为Prometheus文本格式
         */
        for (const auto & el : all_metrics_def.items()) {
            /* 获取指标类型(如"counter"或"gauge") */
            const auto & type        = el.key();
            /* 获取该类型下的所有指标定义 */
            const auto & metrics_def = el.value();

            /*
             * 遍历当前类型下的所有具体指标
             * 为每个指标生成符合Prometheus标准的文本行
             */
            for (const auto & metric_def : metrics_def) {
                /* 提取指标名称 */
                const std::string name = metric_def.at("name");
                /* 提取指标说明文本 */
                const std::string help = metric_def.at("help");

                /*
                 * 安全地提取指标数值
                 * json_value函数提供默认值，避免缺失字段时的错误
                 */
                auto value = json_value(metric_def, "value", 0.);
                
                /*
                 * 按照Prometheus标准格式输出每个指标:
                 * 1. # HELP 行: 描述指标的作用
                 * 2. # TYPE 行: 指定指标类型
                 * 3. 数据行: 实际的指标名和数值
                 * "llamacpp:"前缀用于区分不同服务的指标
                 */
                prometheus << "# HELP llamacpp:" << name << " " << help  << "\n"
                            << "# TYPE llamacpp:" << name << " " << type  << "\n"
                            << "llamacpp:"        << name << " " << value << "\n";
            }
        }

        /*
         * 添加自定义HTTP头信息
         * Process-Start-Time-Unix包含服务器的启动时间戳
         * 用于计算服务器运行时长和重启动监控
         */
        res.set_header("Process-Start-Time-Unix", std::to_string(res_metrics->t_start));

        /*
         * 设置响应内容和类型
         * "text/plain; version=0.0.4"是Prometheus标准的MIME类型
         * version参数指定Prometheus数据格式的版本
         */
        res.set_content(prometheus.str(), "text/plain; version=0.0.4");
        /* 设置200状态码表示请求成功 */
        res.status = 200;
    };

    /*
     * 步骤27: 定义插槽保存处理器
     * 这个处理器允许将某个插槽的当前状态保存到文件
     * 主要用于保存对话上下文、中间状态等，方便后续恢复
     * 这对于长期对话或服务器重启可恢复性非常有用
     * 
     * API端点: POST /slots/{id}/save
     * 请求参数: {"filename": "文件名"}
     * 响应格式: 保存操作的结果信息
     */
    const auto handle_slots_save = [&ctx_server, &res_error, &res_ok, &params](const httplib::Request & req, httplib::Response & res, int id_slot) {
        /*
         * 解析HTTP请求体中的JSON数据
         * 客户端需要提供要保存的文件名
         */
        json request_data = json::parse(req.body);
        /* 提取文件名参数 */
        std::string filename = request_data.at("filename");
        
        /*
         * 验证文件名的安全性
         * fs_validate_filename函数检查文件名是否包含非法字符
         * 防止路径遍历攻击(如"../../../etc/passwd")
         */
        if (!fs_validate_filename(filename)) {
            res_error(res, format_error_response("Invalid filename", ERROR_TYPE_INVALID_REQUEST));
            return;
        }
        
        /*
         * 构建完整的文件路径
         * params.slot_save_path是管理员配置的保存目录
         * 这样可以限制文件只能保存在指定目录下
         */
        std::string filepath = params.slot_save_path + filename;

        /*
         * 使用任务队列机制执行保存操作
         * 保存操作可能涉及大量数据写入，不宜在HTTP线程中直接执行
         */
        int task_id = ctx_server.queue_tasks.get_new_id();
        {
            /*
             * 创建插槽保存任务
             * SERVER_TASK_TYPE_SLOT_SAVE类型专门处理插槽状态的保存
             */
            server_task task(SERVER_TASK_TYPE_SLOT_SAVE);
            task.id = task_id;
            /* 设置要保存的插槽 ID */
            task.slot_action.slot_id  = id_slot;
            /* 设置文件名(不包含路径) */
            task.slot_action.filename = filename;
            /* 设置完整的文件路径 */
            task.slot_action.filepath = filepath;

            /* 注册等待结果并提交任务 */
            ctx_server.queue_results.add_waiting_task_id(task_id);
            ctx_server.queue_tasks.post(std::move(task));
        }

        /*
         * 等待保存操作完成
         * 这个过程可能耗时较长，取决于插槽中数据的大小
         */
        server_task_result_ptr result = ctx_server.queue_results.recv(task_id);
        /* 清理任务资源 */
        ctx_server.queue_results.remove_waiting_task_id(task_id);

        /*
         * 检查保存操作是否成功
         * 失败原因可能包括:磁盘空间不足、权限问题、插槽不存在等
         */
        if (result->is_error()) {
            res_error(res, result->to_json());
            return;
        }

        /* 返回保存成功的结果信息 */
        res_ok(res, result->to_json());
    };

    /*
     * 步骤28: 定义插槽恢复处理器
     * 这个处理器允许从之前保存的文件中恢复插槽的状态
     * 主要用于恢复对话上下文、继续中断的对话等
     * 这对于提供稳定的长期对话服务非常重要
     * 
     * API端点: POST /slots/{id}/restore
     * 请求参数: {"filename": "要恢复的文件名"}
     * 响应格式: 恢复操作的结果信息
     */
    const auto handle_slots_restore = [&ctx_server, &res_error, &res_ok, &params](const httplib::Request & req, httplib::Response & res, int id_slot) {
        /*
         * 解析请求中的JSON数据
         * 客户端需要指定要恢复的文件名
         */
        json request_data = json::parse(req.body);
        /* 获取要恢复的文件名 */
        std::string filename = request_data.at("filename");
        
        /*
         * 验证文件名的合法性和安全性
         * 防止恶意文件访问，保护系统安全
         */
        if (!fs_validate_filename(filename)) {
            res_error(res, format_error_response("Invalid filename", ERROR_TYPE_INVALID_REQUEST));
            return;
        }
        
        /*
         * 构建完整的文件路径
         * 确保文件只能从指定的保存目录中读取
         */
        std::string filepath = params.slot_save_path + filename;

        /*
         * 使用任务队列机制执行恢复操作
         * 恢复操作可能需要加载大量数据，耗时较长
         */
        int task_id = ctx_server.queue_tasks.get_new_id();
        {
            /*
             * 创建插槽恢复任务
             * SERVER_TASK_TYPE_SLOT_RESTORE类型专门处理插槽状态的恢复
             */
            server_task task(SERVER_TASK_TYPE_SLOT_RESTORE);
            task.id = task_id;
            /* 设置要恢复的目标插槽 ID */
            task.slot_action.slot_id  = id_slot;
            /* 设置源文件名 */
            task.slot_action.filename = filename;
            /* 设置源文件的完整路径 */
            task.slot_action.filepath = filepath;

            /* 注册等待结果并提交任务 */
            ctx_server.queue_results.add_waiting_task_id(task_id);
            ctx_server.queue_tasks.post(std::move(task));
        }

        /*
         * 等待恢复操作完成
         * 恢复过程包括读取文件、解析数据、重建插槽状态等
         */
        server_task_result_ptr result = ctx_server.queue_results.recv(task_id);
        /* 清理任务资源 */
        ctx_server.queue_results.remove_waiting_task_id(task_id);

        /*
         * 检查恢复操作是否成功
         * 失败原因可能包括:文件不存在、文件损坏、插槽占用等
         */
        if (result->is_error()) {
            res_error(res, result->to_json());
            return;
        }

        /*
         * 验证结果类型是否正确
         * 确保返回的是插槽保存/加载类型的结果
         */
        GGML_ASSERT(dynamic_cast<server_task_result_slot_save_load*>(result.get()) != nullptr);
        /* 返回恢复成功的结果信息 */
        res_ok(res, result->to_json());
    };

    /*
     * 步骤29: 定义插槽擦除处理器
     * 这个处理器用于清空指定插槽的所有状态和数据
     * 包括对话上下文、生成历史、缓存数据等
     * 这对于释放内存和重置插槽状态非常有用
     * 
     * API端点: POST /slots/{id}/erase
     * 请求参数: 无(只需要插槽ID)
     * 响应格式: 擦除操作的结果信息
     */
    const auto handle_slots_erase = [&ctx_server, &res_error, &res_ok](const httplib::Request & /* req */, httplib::Response & res, int id_slot) {
        /*
         * 使用任务队列机制执行擦除操作
         * 擦除操作可能需要清理大量内存数据，耗时不定
         */
        int task_id = ctx_server.queue_tasks.get_new_id();
        {
            /*
             * 创建插槽擦除任务
             * SERVER_TASK_TYPE_SLOT_ERASE类型专门处理插槽的完全清空
             * 这是一个破坏性操作，不可恢复
             */
            server_task task(SERVER_TASK_TYPE_SLOT_ERASE);
            task.id = task_id;
            /* 设置要擦除的插槽 ID */
            task.slot_action.slot_id = id_slot;

            /* 注册等待结果并提交任务 */
            ctx_server.queue_results.add_waiting_task_id(task_id);
            ctx_server.queue_tasks.post(std::move(task));
        }

        /*
         * 等待擦除操作完成
         * 擦除过程包括清理内存、重置状态变量、释放资源等
         */
        server_task_result_ptr result = ctx_server.queue_results.recv(task_id);
        /* 清理任务资源 */
        ctx_server.queue_results.remove_waiting_task_id(task_id);

        /*
         * 检查擦除操作是否成功
         * 失败原因可能包括:插槽不存在、插槽正在使用中等
         */
        if (result->is_error()) {
            res_error(res, result->to_json());
            return;
        }

        /*
         * 验证结果类型是否正确
         * 确保返回的是插槽擦除类型的结果
         */
        GGML_ASSERT(dynamic_cast<server_task_result_slot_erase*>(result.get()) != nullptr);
        /* 返回擦除成功的结果信息 */
        res_ok(res, result->to_json());
    };

    /*
     * 步骤30: 定义插槽动作统一处理器
     * 这个处理器是插槽管理功能的统一入口
     * 根据请求参数中的action字段来路由到具体的操作
     * 支持save(保存)、restore(恢复)、erase(擦除)三种操作
     * 
     * API端点: POST /slots/{id}?action=save|restore|erase
     * 路径参数: id - 插槽ID
     * 查询参数: action - 要执行的动作
     */
    const auto handle_slots_action = [&params, &res_error, &handle_slots_save, &handle_slots_restore, &handle_slots_erase](const httplib::Request & req, httplib::Response & res) {
        /*
         * 检查是否配置了插槽保存路径
         * 如果没有配置保存路径，则不支持任何插槽操作
         * 这是一个安全措施，防止文件系统被正用
         */
        if (params.slot_save_path.empty()) {
            res_error(res, format_error_response("This server does not support slots action. Start it with `--slot-save-path`", ERROR_TYPE_NOT_SUPPORTED));
            return;
        }

        /*
         * 从路径参数中获取插槽ID
         * URL格式如: /slots/5?action=save
         * path_params.at("id_slot")获取路径中的{id}部分
         */
        std::string id_slot_str = req.path_params.at("id_slot");
        int id_slot;

        /*
         * 将字符串转换为整数
         * 使用try-catch捕获转换错误，防止程序崩溃
         * 如果输入不是数字("abc"、"1.5"等)就会抛异常
         */
        try {
            id_slot = std::stoi(id_slot_str);
        } catch (const std::exception &) {
            res_error(res, format_error_response("Invalid slot ID", ERROR_TYPE_INVALID_REQUEST));
            return;
        }

        /*
         * 从查询参数中获取动作类型
         * 如: /slots/5?action=save 中的 "save"
         */
        std::string action = req.get_param_value("action");

        /*
         * 根据动作类型路由到对应的处理器
         * 这种设计模式叫做“策略模式”，便于扩展和维护
         */
        if (action == "save") {
            /* 调用插槽保存处理器 */
            handle_slots_save(req, res, id_slot);
        } else if (action == "restore") {
            /* 调用插槽恢复处理器 */
            handle_slots_restore(req, res, id_slot);
        } else if (action == "erase") {
            /* 调用插槽擦除处理器 */
            handle_slots_erase(req, res, id_slot);
        } else {
            /*
             * 如果动作类型不在支持列表中，返回错误
             * 帮助客户端发现参数错误
             */
            res_error(res, format_error_response("Invalid action", ERROR_TYPE_INVALID_REQUEST));
        }
    };

    /*
     * 步骤31: 定义服务器属性查询处理器
     * 这个处理器提供服务器的基本信息和配置
     * 主要用于客户端发现服务器的能力和限制
     * 这个端点是公开的，不需要API密钥验证
     * 
     * API端点: GET /props
     * 响应格式: 包含服务器配置信息的JSON对象
     * 特点: 只返回安全的、允许公开的信息
     */
    const auto handle_props = [&ctx_server, &res_ok](const httplib::Request &, httplib::Response & res) {
        /*
         * 重要安全注意事项:
         * 这个端点是公开可访问的，请仅返回安全的信息
         * 不要暴露敏感配置、API密钥、内部路径等
         */
        json data = {
            /*
             * 默认生成设置 - 为客户端提供参考的默认参数
             * 包括温度、top-p、最大令牌数等推理参数
             */
            { "default_generation_settings", ctx_server.default_generation_settings_for_props },
            
            /*
             * 插槽总数 - 服务器可以同时处理的最大请求数
             * 客户端可以根据这个信息来控制并发数
             */
            { "total_slots",                 ctx_server.params_base.n_parallel },
            
            /*
             * 模型文件路径 - 当前加载的模型文件位置
             * 帮助客户端确认正在使用的模型
             */
            { "model_path",                  ctx_server.params_base.model.path },
            
            /*
             * 模型支持的模态 - 指明模型的能力范围
             * vision: 是否支持图像处理(多模态模型)
             * audio: 是否支持音频处理
             */
            { "modalities",                  json{
                {"vision", ctx_server.oai_parser_opt.allow_image},
                {"audio",  ctx_server.oai_parser_opt.allow_audio},
            } },
            
            /*
             * 聊天模板 - 用于格式化对话的模板字符串
             * 不同模型可能有不同的对话格式要求
             */
            { "chat_template",               common_chat_templates_source(ctx_server.chat_templates.get()) },
            
            /*
             * BOS令牌 - Begin Of Sequence，序列开始令牌
             * 用于标记文本的开始，对于正确的令牌化非常重要
             */
            { "bos_token",                   common_token_to_piece(ctx_server.ctx, llama_vocab_bos(ctx_server.vocab), /* special= */ true)},
            
            /*
             * EOS令牌 - End Of Sequence，序列结束令牌
             * 用于标记文本的结束，告诉模型停止生成
             */
            { "eos_token",                   common_token_to_piece(ctx_server.ctx, llama_vocab_eos(ctx_server.vocab), /* special= */ true)},
            
            /*
             * 构建信息 - 服务器的版本和编译信息
             * 用于调试和版本兼容性检查
             */
            { "build_info",                  build_info },
        };
        
        /*
         * 条件性添加工具使用模板
         * 只有在启用Jinja模板引擎时才会可用
         * tool_use模板用于处理函数调用(Function Calling)功能
         */
        if (ctx_server.params_base.use_jinja) {
            if (auto tool_use_src = common_chat_templates_source(ctx_server.chat_templates.get(), "tool_use")) {
                data["chat_template_tool_use"] = tool_use_src;
            }
        }

        /* 返回组装好的服务器属性信息 */
        res_ok(res, data);
    };

    /*
     * 步骤32: 定义服务器属性修改处理器
     * 这个处理器允许动态修改服务器的全局属性
     * 主要用于运行时调整服务器参数，而无需重启
     * 这是一个高级功能，需要特殊权限才能使用
     * 
     * API端点: POST /props
     * 请求参数: 要修改的属性JSON对象
     * 响应格式: {"success": true} 或错误信息
     * 注意: 需要在启动时使用--props参数才能启用
     */
    const auto handle_props_change = [&ctx_server, &res_error, &res_ok](const httplib::Request & req, httplib::Response & res) {
        /*
         * 检查是否启用了属性修改端点
         * 这是一个安全措施，防止未授权的配置修改
         * 只有管理员明确指定--props参数才会开放这个功能
         */
        if (!ctx_server.params_base.endpoint_props) {
            res_error(res, format_error_response("This server does not support changing global properties. Start it with `--props`", ERROR_TYPE_NOT_SUPPORTED));
            return;
        }

        /*
         * 解析请求体中的JSON数据
         * 客户端需要发送要修改的属性和新值
         * 格式如: {"max_tokens": 1000, "temperature": 0.8}
         */
        json data = json::parse(req.body);

        /*
         * TODO: 在这里实现具体的属性更新逻辑
         * 可能包括:
         * - 更新默认生成参数
         * - 修改日志级别
         * - 调整性能参数
         * - 验证参数的有效性和安全性
         */
        // update any props here

        /*
         * 返回成功响应
         * 在实际实现中，应该返回更详细的信息
         * 如修改了哪些属性、新的值是什么等
         */
        res_ok(res, {{ "success", true }});
    };

    /*
     * 步骤33: 定义API信息展示处理器
     * 这个处理器提供详细的模型和API信息
     * 主要用于客户端发现和展示服务器的详细能力
     * 格式与Ollama API兼容，方便集成已有工具
     * 
     * API端点: GET /api/show
     * 响应格式: 包含模型详细信息的JSON对象
     * 特点: 公开端点，不需要身份验证
     */
    const auto handle_api_show = [&ctx_server, &res_ok](const httplib::Request &, httplib::Response & res) {
        json data = {
            /*
             * 聊天模板 - 用于格式化对话的模板
             * 这里出现了两次，可能是为了兼容不同版本的客户端
             */
            {
                "template", common_chat_templates_source(ctx_server.chat_templates.get()),
            },
            /*
             * 模型基本信息 - 包含模型的核心参数
             * context_length: 模型支持的最大上下文长度
             * 这是从最后一个插槽中获取的，所有插槽应该有相同的配置
             */
            {
                "model_info", {
                    { "llama.context_length", ctx_server.slots.back().n_ctx, },
                }
            },
            
            /*
             * Ollama兼容字段 - 为了与Ollama API保持兼容
             * 这些字段在llama.cpp中可能不适用，所以留空
             */
            {"modelfile", ""},    /* 模型文件内容，在llama.cpp中不适用 */
            {"parameters", ""},   /* 模型参数，已在其他地方提供 */
            
            /* 聊天模板(重复，可能是历史原因) */
            {"template", common_chat_templates_source(ctx_server.chat_templates.get())},
            
            /*
             * 模型详细信息 - 描述模型的技术细节
             * 这些信息在llama.cpp中大部分都是空的或固定的
             */
            {"details", {
                {"parent_model", ""},        /* 父模型，用于模型继承 */
                {"format", "gguf"},          /* 模型格式，llama.cpp使用GGUF格式 */
                {"family", ""},             /* 模型家族(如GPT、Llama等) */
                {"families", {""}},          /* 模型家族列表 */
                {"parameter_size", ""},      /* 模型参数数量(如7B、8B等) */
                {"quantization_level", ""}  /* 量化级别(如Q4_0、Q8_0等) */
            }},
            
            /*
             * 额外的模型信息字段(空的，可能是为了兼容性)
             * 具体信息已在上面的model_info字段中提供
             */
            {"model_info", ""},
            
            /*
             * API能力列表 - 显示服务器支持的功能
             * completion: 支持文本补全功能
             * 可以扩展为: ["completion", "chat", "embedding", "vision"]
             */
            {"capabilities", {"completion"}}
        };

        /* 返回详细的API信息 */
        res_ok(res, data);
    };

    /*
     * 步骤34: 定义补全类请求的统一处理器
     * 这是服务器的核心处理器，负责处理所有的AI文本生成请求
     * 支持多种类型的请求:
     * - completion: 文本补全(给定提示，生成继续内容)
     * - chat: 对话式交互(基于聊天模板的对话)
     * - infill: 代码填充(基于上下文生成中间内容)
     * 
     * 特色:
     * - 支持流式输出(边生成边返回)
     * - 支持多模态输入(文本+图片)
     * - 兼容OpenAI API格式
     * - 自动连接断开检测
     */
    const auto handle_completions_impl = [&ctx_server, &res_error, &res_ok](
            server_task_type type,                                      /* 任务类型: 补全或填充 */
            json & data,                                               /* 请求参数JSON数据 */
            const std::vector<raw_buffer> & files,                     /* 上传的文件数据(图片等) */
            const std::function<bool()> & is_connection_closed,        /* 连接断开检测函数 */
            httplib::Response & res,                                   /* HTTP响应对象 */
            oaicompat_type oaicompat                                   /* OpenAI兼容模式 */
        ) -> void {
        /*
         * 断言检查: 确保任务类型的正确性
         * 只支持补全(COMPLETION)和填充(INFILL)两种类型
         * 聊天(CHAT)类型会在外层先转换为补全格式
         */
        GGML_ASSERT(type == SERVER_TASK_TYPE_COMPLETION || type == SERVER_TASK_TYPE_INFILL);

        /*
         * 生成唯一的补全ID
         * 用于跟踪和标识这次请求，方便日志记录和调试
         * 格式通常为 "chatcmpl-" + 随机字符串
         */
        auto completion_id = gen_chatcmplid();
        
        /*
         * 任务ID集合 - 用于跟踪此请求创建的所有子任务
         * 在请求取消或失败时，需要清理所有相关任务
         */
        std::unordered_set<int> task_ids;
        
        /*
         * 使用try-catch捕获处理过程中的各种异常
         * 包括参数错误、资源不足、网络断开等
         */
        try {
            /*
             * 任务列表 - 存储将要提交给任务队列的所有任务
             * 一个请求可能会分解为多个并行任务来提高效率
             */
            std::vector<server_task> tasks;

            /*
             * 提取提示内容
             * prompt可以是字符串或复杂的JSON结构
             * 例如在聊天模式下可能包含多轮对话历史
             */
            const auto & prompt = data.at("prompt");
            /*
             * TODO: 这个日志可能会变得非常长
             * 应该放在一个标志后面或者考虑更紧凑的格式
             * 目前被注释掉了以避免日志过多
             */
            //SRV_DBG("Prompt: %s\n", prompt.is_string() ? prompt.get<std::string>().c_str() : prompt.dump(2).c_str());

            /*
             * 处理上传的文件(主要是图片)
             * 这是多模态功能的核心部分，允许模型同时理解文本和图像
             */
            mtmd::bitmaps bitmaps;  /* 存储处理后的图片数据 */
            const bool has_mtmd = ctx_server.mctx != nullptr;  /* 检查是否支持多模态 */
            {
                /*
                 * 检查多模态支持
                 * 如果服务器不支持多模态但客户端上传了文件，就抛出异常
                 * 这避免了无意义的处理尝试和混乱的错误信息
                 */
                if (!has_mtmd && !files.empty()) {
                    throw std::runtime_error("This server does not support multimodal");
                }
                
                /*
                 * 遍历所有上传的文件
                 * 将它们转换为模型可以理解的位图格式
                 */
                for (auto & file : files) {
                    /*
                     * 从原始数据缓冲区初始化位图
                     * mtmd_helper_bitmap_init_from_buf函数处理图片解码和预处理
                     * 支持常见的图片格式如JPEG、PNG等
                     */
                    mtmd::bitmap bmp(mtmd_helper_bitmap_init_from_buf(ctx_server.mctx, file.data(), file.size()));
                    /*
                     * 检查位图初始化是否成功
                     * 如果文件格式不支持或损坏，就会失败
                     */
                    if (!bmp.ptr) {
                        throw std::runtime_error("Failed to load image or audio file");
                    }
                    
                    /*
                     * 计算位图的哈希值(用于KV缓存)
                     * KV缓存是一个重要的优化技术，可以:
                     * - 避免重复处理相同的图片
                     * - 加速相似请求的处理速度
                     * - 节约GPU显存和计算资源
                     */
                    std::string hash = fnv_hash(bmp.data(), bmp.n_bytes());
                    bmp.set_id(hash.c_str());  /* 设置位图的唯一标识符 */
                    
                    /*
                     * 将处理好的位图添加到集合中
                     * std::move用于移动语义，避免不必要的内存复制
                     */
                    bitmaps.entries.push_back(std::move(bmp));
                }
            }

            /*
             * 处理提示词 - 这是文本生成的核心步骤
             * 将用户输入的文本转换为模型可以理解的令牌序列
             */
            std::vector<server_tokens> inputs;

            /*
             * 根据是否支持多模态来选择不同的处理方式
             * oaicompat表示是否使用OpenAI兼容模式
             */
            if (oaicompat && has_mtmd) {
                /*
                 * 多模态处理分支 - 同时处理文本和图片
                 * 这是更复杂的处理流程，需要特殊的令牌化方式
                 */
                std::string prompt_str = prompt.get<std::string>();
                
                /*
                 * 多模态输入文本结构
                 * add_special: 是否添加特殊令牌(如BOS/EOS)
                 * parse_special: 是否解析文本中的特殊标记
                 */
                mtmd_input_text inp_txt = {
                    prompt_str.c_str(),
                    /* add_special */   true,
                    /* parse_special */ true,
                };
                
                /*
                 * 初始化多模态输入块结构
                 * chunks用于存储混合了文本和图片的令牌序列
                 */
                mtmd::input_chunks chunks(mtmd_input_chunks_init());
                auto bitmaps_c_ptr = bitmaps.c_ptr();  /* 获取C风格指针以兼容C API */
                
                /*
                 * 执行多模态令牌化
                 * 这个函数会将文本和图片结合成一个统一的令牌序列
                 * 返回0表示成功，非0表示失败
                 */
                int32_t tokenized = mtmd_tokenize(ctx_server.mctx,
                                                    chunks.ptr.get(),
                                                    &inp_txt,
                                                    bitmaps_c_ptr.data(),
                                                    bitmaps_c_ptr.size());
                if (tokenized != 0) {
                    throw std::runtime_error("Failed to tokenize prompt");
                }

                /*
                 * 将多模态chunks转换为server_tokens格式
                 * true参数表示这是多模态数据
                 */
                server_tokens tmp(chunks, true);
                inputs.push_back(std::move(tmp));
            } else {
                /*
                 * 纯文本处理分支 - 只处理文本输入
                 * 这是更简单、更快速的处理方式
                 */
                auto tokenized_prompts = tokenize_input_prompts(ctx_server.vocab, prompt, true, true);
                
                /*
                 * 将每个令牌化的提示转换为server_tokens格式
                 * 一个请求可能包含多个子提示(如批处理请求)
                 */
                for (auto & p : tokenized_prompts) {
                    auto tmp = server_tokens(p, ctx_server.mctx != nullptr);
                    inputs.push_back(std::move(tmp));
                }
            }

            /*
             * 为任务列表预分配内存空间
             * 这有助于避免动态内存分配的开销，提高性能
             */
            tasks.reserve(inputs.size());
            
            /*
             * 为每个输入创建一个对应的任务
             * 这支持批处理请求，一次可以处理多个提示
             */
            for (size_t i = 0; i < inputs.size(); i++) {
                /*
                 * 创建一个新的服务器任务
                 * type可以是补全(COMPLETION)或填充(INFILL)
                 */
                server_task task = server_task(type);

                /* 设置任务的唯一标识符，用于跟踪和管理 */
                task.id    = ctx_server.queue_tasks.get_new_id();
                /* 设置任务在批处理中的索引位置 */
                task.index = i;

                /*
                 * 设置任务的令牌化提示
                 * std::move用于移动语义，避免复制大量数据
                 */
                task.prompt_tokens    = std::move(inputs[i]);
                
                /*
                 * 从请求JSON中解析生成参数
                 * 包括温度、top-p、最大令牌数等所有推理参数
                 */
                task.params           = server_task::params_from_json_cmpl(
                        ctx_server.ctx,      /* llama上下文 */
                        ctx_server.params_base,  /* 基础参数 */
                        data                 /* 请求数据 */
                );
                
                /*
                 * 设置指定的插槽 ID(可选)
                 * 如果用户指定了-1以外的值，就会尝试使用指定插槽
                 * -1表示由服务器自动选择可用插槽
                 */
                task.id_selected_slot = json_value(data, "id_slot", -1);

                /*
                 * OpenAI兼容性设置
                 * 这些参数用于确保响应格式符合OpenAI API标准
                 */
                task.params.oaicompat         = oaicompat;      /* 是否启用OpenAI兼容模式 */
                task.params.oaicompat_cmpl_id = completion_id;  /* 补全请求的唯一ID */
                /*
                 * oaicompat_model已经在params_from_json_cmpl中填充
                 * 它指定了客户端请求的模型名称
                 */

                /* 将配置好的任务添加到任务列表 */
                tasks.push_back(std::move(task));
            }

            /*
             * 提取所有任务的ID列表
             * 用于后续的任务管理和清理工作
             */
            task_ids = server_task::get_list_id(tasks);
            
            /*
             * 将任务添加到等待结果的列表中
             * 这样当任务完成时，可以通知请求处理器
             */
            ctx_server.queue_results.add_waiting_tasks(tasks);
            
            /*
             * 将任务提交到任务队列
             * 从这里开始，任务就会被工作线程异步处理
             * std::move避免不必要的数据复制
             */
            ctx_server.queue_tasks.post(std::move(tasks));
        } catch (const std::exception & e) {
            res_error(res, format_error_response(e.what(), ERROR_TYPE_INVALID_REQUEST));
            return;
        }

        /*
         * 检查是否启用流式响应模式
         * stream=true: 边生成边发送，类似ChatGPT的打字机效果
         * stream=false: 等待完整生成后一次性返回所有内容
         */
        bool stream = json_value(data, "stream", false);

        /*
         * 非流式响应处理分支
         * 等待所有任务完成后统一返回结果
         */
        if (!stream) {
            /*
             * 等待并接收所有任务的完整结果
             * 这是一个阻塞操作，会等到所有任务都完成
             */
            ctx_server.receive_multi_results(task_ids, [&](std::vector<server_task_result_ptr> & results) {
                /*
                 * 成功回调函数 - 处理任务完成的结果
                 * 根据结果数量决定返回格式
                 */
                if (results.size() == 1) {
                    /*
                     * 单个结果 - 直接返回JSON对象
                     * 这是最常见的情况(单个提示请求)
                     */
                    res_ok(res, results[0]->to_json());
                } else {
                    /*
                     * 多个结果 - 包装成JSON数组返回
                     * 这发生在批处理请求中(一次提交多个提示)
                     */
                    json arr = json::array();
                    for (auto & res : results) {
                        /* 将每个结果转换为JSON并添加到数组 */
                        arr.push_back(res->to_json());
                    }
                    res_ok(res, arr);
                }
            }, [&](const json & error_data) {
                /*
                 * 错误回调函数 - 处理任务执行过程中的错误
                 * 例如模型加载失败、内存不足、参数错误等
                 */
                res_error(res, error_data);
            }, is_connection_closed);

            /*
             * 清理等待列表中的任务ID
             * 无论成功还是失败，都需要从等待队列中移除这些任务
             * 防止内存泄漏和资源占用
             */
            ctx_server.queue_results.remove_waiting_task_ids(task_ids);
        } else {
            /*
             * 流式响应处理分支
             * 实时发送生成的内容，边生成边传输
             * 使用Server-Sent Events(SSE)协议进行实时通信
             */
            const auto chunked_content_provider = [task_ids, &ctx_server, oaicompat](size_t, httplib::DataSink & sink) {
                /*
                 * 开始接收流式结果
                 * 每当有新内容生成时就立即发送给客户端
                 */
                ctx_server.receive_cmpl_results_stream(task_ids, [&](server_task_result_ptr & result) -> bool {
                    /*
                     * 数据回调函数 - 处理每个生成的数据块
                     * 返回true继续接收，返回false停止生成
                     */
                    json res_json = result->to_json();
                    if (res_json.is_array()) {
                        /*
                         * 批处理结果 - 遍历数组中的每个元素
                         * 为每个结果单独发送一个SSE事件
                         */
                        for (const auto & res : res_json) {
                            if (!server_sent_event(sink, "data", res)) {
                                /*
                                 * 发送失败 - 通常是HTTP连接已关闭
                                 * 立即取消生成，避免浪费计算资源
                                 */
                                return false;
                            }
                        }
                        return true;
                    } else {
                        /*
                         * 单个结果 - 直接发送SSE事件
                         * 包含生成的文本片段和相关元数据
                         */
                        return server_sent_event(sink, "data", res_json);
                    }
                }, [&](const json & error_data) {
                    /*
                     * 错误回调函数 - 发送错误信息给客户端
                     * 使用SSE的error事件类型通知前端发生了错误
                     */
                    server_sent_event(sink, "error", error_data);
                }, [&sink]() {
                    /*
                     * 连接检查回调函数 - 检测客户端是否断开连接
                     * 注意：这里不能使用req.is_connection_closed，因为req对象已被销毁
                     * 通过sink的可写状态来判断连接是否还有效
                     */
                    return !sink.is_writable();
                });
                
                /*
                 * OpenAI兼容性处理
                 * 发送流式响应结束标记，符合OpenAI API规范
                 */
                if (oaicompat != OAICOMPAT_TYPE_NONE) {
                    static const std::string ev_done = "data: [DONE]\n\n";
                    sink.write(ev_done.data(), ev_done.size());
                }
                
                /*
                 * 完成响应传输
                 * 通知HTTP库响应已完成，可以关闭连接
                 */
                sink.done();
                return false;  /* 表示内容提供器已完成工作 */
            };

            /*
             * 响应完成回调函数
             * 无论流式传输成功还是失败都会被调用
             * 用于清理资源和移除等待中的任务
             */
            auto on_complete = [task_ids, &ctx_server] (bool) {
                /*
                 * 从等待结果队列中移除任务ID
                 * 释放相关资源，防止内存泄漏
                 * bool参数表示是否成功完成，但这里我们不关心
                 */
                ctx_server.queue_results.remove_waiting_task_ids(task_ids);
            };

            /*
             * 设置HTTP响应为分块传输模式
             * Content-Type: text/event-stream 表示这是SSE流
             * chunked_content_provider: 内容提供器，负责生成和发送数据
             * on_complete: 完成回调，用于资源清理
             */
            res.set_chunked_content_provider("text/event-stream", chunked_content_provider, on_complete);
        }
    };

    /*
     * 标准补全接口处理器
     * 处理 /completion 端点的请求，使用llama.cpp原生格式
     * 不进行OpenAI兼容性转换，直接使用原始请求格式
     */
    const auto handle_completions = [&handle_completions_impl](const httplib::Request & req, httplib::Response & res) {
        /*
         * 解析请求体中的JSON数据
         * 包含提示文本、生成参数等所有必要信息
         */
        json data = json::parse(req.body);
        
        /*
         * 创建空的文件数组
         * 标准补全接口不支持文件上传，所以这里是空的
         */
        std::vector<raw_buffer> files; // dummy
        
        /*
         * 调用通用补全实现函数
         * SERVER_TASK_TYPE_COMPLETION: 指定任务类型为文本补全
         * OAICOMPAT_TYPE_NONE: 不启用OpenAI兼容模式
         */
        handle_completions_impl(
            SERVER_TASK_TYPE_COMPLETION,
            data,
            files,
            req.is_connection_closed,
            res,
            OAICOMPAT_TYPE_NONE);
    };

    /*
     * OpenAI兼容补全接口处理器
     * 处理 /v1/completions 端点的请求，兼容OpenAI API格式
     * 将OpenAI格式的请求转换为llama.cpp内部格式
     */
    const auto handle_completions_oai = [&handle_completions_impl](const httplib::Request & req, httplib::Response & res) {
        /*
         * 解析并转换OpenAI格式的请求参数
         * oaicompat_completion_params_parse函数负责：
         * - 将OpenAI字段名映射到llama.cpp字段名
         * - 处理参数格式差异(如温度范围、停止词格式等)
         * - 添加默认值和参数验证
         */
        json data = oaicompat_completion_params_parse(json::parse(req.body));
        
        /*
         * 创建空的文件数组
         * OpenAI补全接口不支持文件上传(文件上传在chat接口中支持)
         */
        std::vector<raw_buffer> files; // dummy
        
        /*
         * 调用通用补全实现函数
         * SERVER_TASK_TYPE_COMPLETION: 指定任务类型为文本补全
         * OAICOMPAT_TYPE_COMPLETION: 启用OpenAI补全兼容模式
         * 响应格式会符合OpenAI API规范
         */
        handle_completions_impl(
            SERVER_TASK_TYPE_COMPLETION,
            data,
            files,
            req.is_connection_closed,
            res,
            OAICOMPAT_TYPE_COMPLETION);
    };

    /*
     * 代码填充(Infill)接口处理器
     * 处理 /infill 端点的请求，用于代码自动补全功能
     * 实现Fill-In-the-Middle(FIM)技术，在指定位置插入代码
     */
    const auto handle_infill = [&ctx_server, &res_error, &handle_completions_impl](const httplib::Request & req, httplib::Response & res) {
        /*
         * 检查模型兼容性 - 验证模型是否支持FIM功能
         * FIM需要特殊的词汇表令牌来标记前缀、后缀和中间部分
         */
        std::string err;
        
        /*
         * 检查前缀令牌 - 标记代码前部分的特殊token
         * 如果模型词汇表中没有这个token，就无法进行填充操作
         */
        if (llama_vocab_fim_pre(ctx_server.vocab) == LLAMA_TOKEN_NULL) {
            err += "prefix token is missing. ";
        }
        
        /*
         * 检查后缀令牌 - 标记代码后部分的特殊token
         * 用于告诉模型在哪里结束填充内容
         */
        if (llama_vocab_fim_suf(ctx_server.vocab) == LLAMA_TOKEN_NULL) {
            err += "suffix token is missing. ";
        }
        
        /*
         * 检查中间令牌 - 标记需要填充位置的特殊token
         * 这是FIM的关键token，告诉模型在此处生成内容
         */
        if (llama_vocab_fim_mid(ctx_server.vocab) == LLAMA_TOKEN_NULL) {
            err += "middle token is missing. ";
        }
        
        /*
         * 如果缺少任何必要的FIM令牌，返回不支持错误
         * 避免用户尝试使用不兼容的模型进行填充操作
         */
        if (!err.empty()) {
            res_error(res, format_error_response(string_format("Infill is not supported by this model: %s", err.c_str()), ERROR_TYPE_NOT_SUPPORTED));
            return;
        }

        /*
         * 解析请求体中的JSON数据
         * 包含前缀、后缀、额外上下文等填充所需的所有信息
         */
        json data = json::parse(req.body);

        /*
         * 输入参数验证 - 确保请求格式正确
         * 严格的参数验证可以避免后续处理中的错误
         */
        
        /*
         * 验证可选的prompt参数
         * prompt用于提供额外的指导信息，如编程语言类型、代码风格等
         */
        if (data.contains("prompt") && !data.at("prompt").is_string()) {
            res_error(res, format_error_response("\"prompt\" must be a string", ERROR_TYPE_INVALID_REQUEST));
        }

        /*
         * 验证必需的input_prefix参数
         * 包含光标位置之前的所有代码内容
         */
        if (!data.contains("input_prefix")) {
            res_error(res, format_error_response("\"input_prefix\" is required", ERROR_TYPE_INVALID_REQUEST));
        }

        /*
         * 验证必需的input_suffix参数
         * 包含光标位置之后的所有代码内容
         */
        if (!data.contains("input_suffix")) {
            res_error(res, format_error_response("\"input_suffix\" is required", ERROR_TYPE_INVALID_REQUEST));
        }

        /*
         * 验证可选的input_extra参数
         * 用于提供额外的上下文信息，如其他相关文件内容
         * 必须是包含filename和text字段的对象数组
         */
        if (data.contains("input_extra") && !data.at("input_extra").is_array()) {
            res_error(res, format_error_response("\"input_extra\" must be an array of {\"filename\": string, \"text\": string}", ERROR_TYPE_INVALID_REQUEST));
            return;
        }

        /*
         * 处理额外上下文信息
         * 这些信息可以帮助模型更好地理解代码结构和意图
         */
        json input_extra = json_value(data, "input_extra", json::array());
        for (const auto & chunk : input_extra) {
            /*
             * 验证每个上下文块的格式
             * 每个块必须包含text字段，filename字段是可选的
             */
            if (!chunk.contains("text") || !chunk.at("text").is_string()) {
                res_error(res, format_error_response("extra_context chunk must contain a \"text\" field with a string value", ERROR_TYPE_INVALID_REQUEST));
                return;
            }
            
            /*
             * 验证可选的filename字段
             * 如果提供了filename，它必须是字符串类型
             */
            if (chunk.contains("filename") && !chunk.at("filename").is_string()) {
                res_error(res, format_error_response("extra_context chunk's \"filename\" field must be a string", ERROR_TYPE_INVALID_REQUEST));
                return;
            }
        }
        
        /*
         * 确保input_extra字段存在
         * 如果请求中没有提供，就设置为空数组
         * 这样后续处理函数可以安全地访问这个字段
         */
        data["input_extra"] = input_extra;

        /*
         * 提取和令牌化提示文本
         * 将用户提供的文本指导转换为模型可理解的token序列
         */
        std::string prompt = json_value(data, "prompt", std::string());
        std::vector<llama_tokens> tokenized_prompts = tokenize_input_prompts(ctx_server.vocab, prompt, false, true);
        SRV_DBG("creating infill tasks, n_prompts = %d\n", (int) tokenized_prompts.size());
        
        /*
         * 格式化填充提示 - 这是FIM的核心步骤
         * format_infill函数会：
         * - 将前缀、后缀、额外上下文按FIM格式重新排列
         * - 插入特殊的FIM令牌(prefix, suffix, middle)
         * - 考虑上下文长度限制，适当截断内容
         * - 生成最终的提示序列供模型处理
         */
        data["prompt"] = format_infill(
            ctx_server.vocab,                        /* 词汇表，用于令牌转换 */
            data.at("input_prefix"),                 /* 光标前的代码 */
            data.at("input_suffix"),                 /* 光标后的代码 */
            data.at("input_extra"),                  /* 额外上下文信息 */
            ctx_server.params_base.n_batch,          /* 批处理大小 */
            ctx_server.params_base.n_predict,        /* 最大预测令牌数 */
            ctx_server.slots[0].n_ctx,               /* 上下文窗口大小 TODO: 应该有更好的方式获取 */
            ctx_server.params_base.spm_infill,       /* 是否使用SentencePiece填充格式 */
            tokenized_prompts[0]                     /* 令牌化的提示 */
        );

        /*
         * 创建空的文件数组
         * 填充接口不支持文件上传，只处理纯文本代码
         */
        std::vector<raw_buffer> files; // dummy
        
        /*
         * 调用通用补全实现函数
         * SERVER_TASK_TYPE_INFILL: 指定任务类型为代码填充
         * OAICOMPAT_TYPE_NONE: 填充功能不兼容OpenAI API
         */
        handle_completions_impl(
            SERVER_TASK_TYPE_INFILL,
            data,
            files,
            req.is_connection_closed,
            res,
            OAICOMPAT_TYPE_NONE); // infill is not OAI compatible
    };

    /*
     * OpenAI兼容聊天补全接口处理器
     * 处理 /v1/chat/completions 端点，兼容OpenAI Chat API格式
     * 支持多轮对话、角色扮演、多模态输入(图片、音频等)
     */
    const auto handle_chat_completions = [&ctx_server, &handle_completions_impl](const httplib::Request & req, httplib::Response & res) {
        /*
         * 记录调试信息 - 打印完整的请求体
         * 这有助于调试聊天请求的格式和内容
         * 注意：生产环境中可能包含敏感信息，需要谨慎记录
         */
        LOG_DBG("request: %s\n", req.body.c_str());

        /*
         * 解析请求体JSON数据
         * 聊天请求通常包含messages数组、模型名称、生成参数等
         */
        auto body = json::parse(req.body);
        
        /*
         * 用于存储解析出的文件数据
         * 聊天接口支持多模态输入，如图片、音频等附件
         */
        std::vector<raw_buffer> files;
        
        /*
         * 解析并转换OpenAI聊天格式
         * oaicompat_chat_params_parse函数负责：
         * - 将messages数组转换为单一的prompt字符串
         * - 处理system、user、assistant角色的消息
         * - 提取并解码base64编码的图片/音频数据
         * - 应用聊天模板格式化对话历史
         * - 转换OpenAI参数到llama.cpp内部格式
         */
        json data = oaicompat_chat_params_parse(
            body,                        /* 原始请求JSON */
            ctx_server.oai_parser_opt,   /* OpenAI解析器选项 */
            files                        /* 输出：解析出的文件数据 */
        );

        /*
         * 调用通用补全实现函数
         * SERVER_TASK_TYPE_COMPLETION: 聊天最终也是文本补全任务
         * OAICOMPAT_TYPE_CHAT: 启用OpenAI聊天兼容模式
         * 响应格式会符合OpenAI Chat API规范
         */
        handle_completions_impl(
            SERVER_TASK_TYPE_COMPLETION,
            data,
            files,
            req.is_connection_closed,
            res,
            OAICOMPAT_TYPE_CHAT);
    };

    /*
     * 聊天模板应用接口处理器
     * 与handle_chat_completions相同的解析逻辑，但不执行推理
     * 只返回应用聊天模板后的最终prompt，用于调试和验证
     */
    const auto handle_apply_template = [&ctx_server, &res_ok](const httplib::Request & req, httplib::Response & res) {
        /*
         * 解析请求体JSON数据
         * 包含与聊天接口相同的messages数组和参数
         */
        auto body = json::parse(req.body);
        
        /*
         * 创建空的文件数组(此接口不使用文件功能)
         * 只是为了满足解析函数的参数要求
         */
        std::vector<raw_buffer> files; // dummy, unused
        
        /*
         * 使用与聊天接口相同的解析逻辑
         * 将messages转换为格式化的prompt字符串
         * 应用模型的聊天模板、角色标记等
         */
        json data = oaicompat_chat_params_parse(
            body,                        /* 原始请求JSON */
            ctx_server.oai_parser_opt,   /* OpenAI解析器选项 */
            files                        /* 未使用的文件数组 */
        );
        
        /*
         * 返回处理后的prompt
         * 这让开发者可以预览最终发送给模型的prompt内容
         * 有助于调试聊天模板的效果和问题排查
         */
        res_ok(res, {{ "prompt", std::move(data.at("prompt")) }});
    };

    /*
     * 模型信息查询接口处理器
     * 处理 /v1/models 端点，兼容OpenAI API格式
     * 返回当前加载模型的详细信息和能力列表
     */
    const auto handle_models = [&params, &ctx_server, &state, &res_ok](const httplib::Request &, httplib::Response & res) {
        /*
         * 获取当前服务器状态
         * 使用原子操作确保状态读取的线程安全性
         */
        server_state current_state = state.load();
        
        /*
         * 模型元数据初始化
         * 只有在服务器就绪状态下才能获取模型信息
         */
        json model_meta = nullptr;
        if (current_state == SERVER_STATE_READY) {
            /*
             * 获取模型元数据信息
             * 包括模型架构、参数数量、词汇表大小等详细信息
             */
            model_meta = ctx_server.model_meta();
        }

        /*
         * 构建模型列表响应
         * 包含两种格式：Ollama兼容格式和OpenAI兼容格式
         */
        json models = {
            /*
             * Ollama API兼容格式
             * 提供更详细的模型信息，包括能力和参数
             */
            {"models", {
                {
                    /* 模型名称，优先使用别名，否则使用文件路径 */
                    {"name", params.model_alias.empty() ? params.model.path : params.model_alias},
                    {"model", params.model_alias.empty() ? params.model.path : params.model_alias},
                    
                    /* 模型文件信息(这些字段为占位符，llama.cpp目前不支持) */
                    {"modified_at", ""},           /* 修改时间 */
                    {"size", ""},                  /* 文件大小 */
                    {"digest", ""},               /* 文件哈希值 - llama.cpp不支持管理模型文件哈希 */
                    
                    /* 模型基本属性 */
                    {"type", "model"},            /* 类型：模型 */
                    {"description", ""},          /* 描述信息 */
                    {"tags", {""}},              /* 标签列表 */
                    {"capabilities", {"completion"}}, /* 支持的功能：文本补全 */
                    {"parameters", ""},           /* 参数信息 */
                    
                    /* 详细信息 */
                    {"details", {
                        {"parent_model", ""},            /* 父模型 */
                        {"format", "gguf"},             /* 模型格式：GGUF */
                        {"family", ""},                 /* 模型家族 */
                        {"families", {""}},             /* 模型家族列表 */
                        {"parameter_size", ""},         /* 参数规模 */
                        {"quantization_level", ""}      /* 量化级别 */
                    }}
                }
            }},
            
            /*
             * OpenAI API兼容格式
             * 符合OpenAI /v1/models 端点的响应格式
             */
            {"object", "list"},               /* 对象类型：列表 */
            {"data", {
                {
                    /* 模型标识符，与模型名称相同 */
                    {"id",       params.model_alias.empty() ? params.model.path : params.model_alias},
                    {"object",   "model"},        /* 对象类型：模型 */
                    {"created",  std::time(0)},   /* 创建时间戳(使用当前时间) */
                    {"owned_by", "llamacpp"},     /* 所有者：llama.cpp */
                    {"meta",     model_meta},     /* 模型元数据(详细的模型信息) */
                },
            }}
        };

        /*
         * 返回成功响应
         * 客户端可以根据需要解析Ollama格式或OpenAI格式的数据
         */
        res_ok(res, models);
    };

    /*
     * 文本令牌化接口处理器
     * 处理 /tokenize 端点，将文本转换为token ID序列
     * 用于调试、分析文本如何被模型理解，以及计算token使用量
     */
    const auto handle_tokenize = [&ctx_server, &res_ok](const httplib::Request & req, httplib::Response & res) {
        /*
         * 解析请求体JSON数据
         * 应包含要令牌化的文本内容和相关选项
         */
        const json body = json::parse(req.body);

        /*
         * 初始化令牌响应数组
         * 将存储令牌ID和对应的文本片段
         */
        json tokens_response = json::array();
        
        /*
         * 检查是否提供了content字段
         * content包含需要令牌化的文本内容
         */
        if (body.count("content") != 0) {
            /*
             * 解析令牌化选项
             * add_special: 是否添加特殊令牌(如BOS/EOS)
             * parse_special: 是否解析文本中的特殊标记
             * with_pieces: 是否在响应中包含每个token对应的文本片段
             */
            const bool add_special = json_value(body, "add_special", false);
            const bool parse_special = json_value(body, "parse_special", true);
            const bool with_pieces = json_value(body, "with_pieces", false);

            /*
             * 执行令牌化操作
             * tokenize_mixed函数处理混合内容(文本+特殊标记)
             * 返回token ID的向量
             */
            llama_tokens tokens = tokenize_mixed(ctx_server.vocab, body.at("content"), add_special, parse_special);

            /*
             * 根据with_pieces选项决定响应格式
             */
            if (with_pieces) {
                /*
                 * 详细模式：包含每个token的ID和对应的文本片段
                 * 这对于理解令牌化过程很有帮助
                 */
                for (const auto& token : tokens) {
                    /*
                     * 获取token对应的文本片段
                     * 将token ID转换回原始文本表示
                     */
                    std::string piece = common_token_to_piece(ctx_server.ctx, token);
                    json piece_json;

                    /*
                     * 检查文本片段是否为有效的UTF-8编码
                     * 某些token可能包含特殊字符或字节序列
                     */
                    if (is_valid_utf8(piece)) {
                        /*
                         * 有效UTF-8：直接存储为字符串
                         * 这是大多数普通文本token的情况
                         */
                        piece_json = piece;
                    } else {
                        /*
                         * 无效UTF-8：存储为字节值数组
                         * 这通常发生在特殊字符或二进制数据中
                         * 保留原始字节信息以便调试
                         */
                        piece_json = json::array();
                        for (unsigned char c : piece) {
                            piece_json.push_back(static_cast<int>(c));
                        }
                    }

                    /*
                     * 将token信息添加到响应中
                     * 包含ID和对应的文本片段
                     */
                    tokens_response.push_back({
                        {"id", token},          /* token ID */
                        {"piece", piece_json}   /* 对应的文本片段 */
                    });
                }
            } else {
                /*
                 * 简单模式：只返回token ID数组
                 * 适用于只需要token数量或ID序列的场景
                 */
                tokens_response = tokens;
            }
        }

        /*
         * 格式化令牌化响应
         * 包装成标准的API响应格式
         */
        const json data = format_tokenizer_response(tokens_response);
        res_ok(res, data);
    };

    /*
     * 令牌反向转换接口处理器
     * 处理 /detokenize 端点，将token ID序列转换回原始文本
     * 与tokenize操作相反，用于验证令牌化结果或调试
     */
    const auto handle_detokenize = [&ctx_server, &res_ok](const httplib::Request & req, httplib::Response & res) {
        /*
         * 解析请求体JSON数据
         * 应包含要反向转换的token ID数组
         */
        const json body = json::parse(req.body);

        /*
         * 初始化输出文本内容
         * 将存储从token序列重建的文本
         */
        std::string content;
        
        /*
         * 检查是否提供了tokens字段
         * tokens包含需要转换的token ID数组
         */
        if (body.count("tokens") != 0) {
            /*
             * 获取token ID数组
             * 从JSON中提取llama_tokens类型的token序列
             */
            const llama_tokens tokens = body.at("tokens");
            
            /*
             * 执行反向令牌化操作
             * tokens_to_str函数将token ID序列转换为连续的文本字符串
             * 使用迭代器范围[begin, end)处理整个token序列
             */
            content = tokens_to_str(ctx_server.ctx, tokens.cbegin(), tokens.cend());
        }

        /*
         * 格式化反向令牌化响应
         * 包装成标准的API响应格式，包含重建的文本内容
         */
        const json data = format_detokenized_response(content);
        res_ok(res, data);
    };

    /*
     * 嵌入向量生成接口实现
     * 处理文本嵌入请求，将文本转换为高维向量表示
     * 支持OpenAI API兼容格式，用于语义搜索、文本相似度等任务
     */
    const auto handle_embeddings_impl = [&ctx_server, &res_error, &res_ok](const httplib::Request & req, httplib::Response & res, oaicompat_type oaicompat) {
        /*
         * 检查嵌入功能是否启用
         * 嵌入功能需要在服务器启动时使用 --embeddings 参数开启
         * 这是因为嵌入模式与文本生成模式使用不同的推理配置
         */
        if (!ctx_server.params_base.embedding) {
            res_error(res, format_error_response("This server does not support embeddings. Start it with `--embeddings`", ERROR_TYPE_NOT_SUPPORTED));
            return;
        }

        /*
         * 检查OpenAI兼容性要求
         * OpenAI API要求使用特定的pooling类型来聚合token embeddings
         * LLAMA_POOLING_TYPE_NONE不兼容OpenAI格式
         */
        if (oaicompat != OAICOMPAT_TYPE_NONE && llama_pooling_type(ctx_server.ctx) == LLAMA_POOLING_TYPE_NONE) {
            res_error(res, format_error_response("Pooling type 'none' is not OAI compatible. Please use a different pooling type", ERROR_TYPE_INVALID_REQUEST));
            return;
        }

        /*
         * 解析请求体JSON数据
         * 包含要嵌入的文本内容和相关配置选项
         */
        const json body = json::parse(req.body);

        /*
         * 提取输入内容
         * 支持两种字段名：input(OpenAI兼容) 和 content(llama.cpp原生)
         * 输入格式参考 tokenize_input_prompts() 函数的要求
         */
        json prompt;
        if (body.count("input") != 0) {
            /*
             * OpenAI兼容格式：使用"input"字段
             * 保持OpenAI API的兼容性
             */
            prompt = body.at("input");
        } else if (body.contains("content")) {
            /*
             * llama.cpp原生格式：使用"content"字段
             * 这种格式不兼容OpenAI API，需要重置兼容性标志
             */
            oaicompat = OAICOMPAT_TYPE_NONE;
            prompt = body.at("content");
        } else {
            /*
             * 输入内容缺失错误
             * 必须提供input或content中的一个字段
             */
            res_error(res, format_error_response("\"input\" or \"content\" must be provided", ERROR_TYPE_INVALID_REQUEST));
            return;
        }

        /*
         * 解析编码格式选项
         * 决定嵌入向量的返回格式：浮点数组或base64编码
         */
        bool use_base64 = false;
        if (body.count("encoding_format") != 0) {
            const std::string& format = body.at("encoding_format");
            if (format == "base64") {
                /*
                 * base64格式：适用于需要压缩传输或存储的场景
                 * 可以减少网络传输大小，但需要客户端解码
                 */
                use_base64 = true;
            } else if (format != "float") {
                /*
                 * 不支持的格式错误
                 * 只支持float(默认)和base64两种格式
                 */
                res_error(res, format_error_response("The format to return the embeddings in. Can be either float or base64", ERROR_TYPE_INVALID_REQUEST));
                return;
            }
        }

        /*
         * 令牌化输入文本
         * 将文本转换为模型可处理的token序列
         * 添加特殊token(BOS等)以确保正确的模型理解
         */
        auto tokenized_prompts = tokenize_input_prompts(ctx_server.vocab, prompt, true, true);
        for (const auto & tokens : tokenized_prompts) {
            /*
             * 检查token序列有效性
             * 某些模型不会自动添加BOS token，需要确保输入不为空
             * 空输入会导致嵌入计算失败
             */
            if (tokens.empty()) {
                res_error(res, format_error_response("Input content cannot be empty", ERROR_TYPE_INVALID_REQUEST));
                return;
            }
        }

        /*
         * 解析嵌入向量归一化选项
         * 控制如何对生成的嵌入向量进行标准化处理
         */
        int embd_normalize = 2; // 默认使用欧几里得/L2范数归一化
        if (body.count("embd_normalize") != 0) {
            embd_normalize = body.at("embd_normalize");
            /*
             * 检查归一化兼容性
             * 某些pooling类型不支持归一化，会忽略此参数
             */
            if (llama_pooling_type(ctx_server.ctx) == LLAMA_POOLING_TYPE_NONE) {
                SRV_DBG("embd_normalize is not supported by pooling type %d, ignoring it\n", llama_pooling_type(ctx_server.ctx));
            }
        }

        /*
         * 创建并提交嵌入任务
         * 将每个令牌化的提示转换为独立的嵌入任务
         */
        json responses = json::array();  /* 存储所有嵌入结果 */
        bool error = false;              /* 错误标志 */
        std::unordered_set<int> task_ids; /* 任务ID集合，用于跟踪 */
        {
            /*
             * 任务创建代码块
             * 使用代码块限制tasks变量的作用域，及时释放内存
             */
            std::vector<server_task> tasks;
            for (size_t i = 0; i < tokenized_prompts.size(); i++) {
                /*
                 * 创建嵌入任务
                 * 每个输入文本都会创建一个独立的嵌入任务
                 */
                server_task task = server_task(SERVER_TASK_TYPE_EMBEDDING);

                /* 设置任务基本属性 */
                task.id            = ctx_server.queue_tasks.get_new_id(); /* 获取唯一任务ID */
                task.index         = i;                                   /* 批处理中的索引位置 */
                task.prompt_tokens = server_tokens(tokenized_prompts[i], ctx_server.mctx != nullptr); /* 令牌化的输入 */

                /* 设置OpenAI兼容性和嵌入参数 */
                task.params.oaicompat = oaicompat;           /* OpenAI兼容模式 */
                task.params.embd_normalize = embd_normalize; /* 向量归一化选项 */

                /* 将任务添加到批处理列表 */
                tasks.push_back(std::move(task));
            }

            /*
             * 提交任务到处理队列
             * 这些任务会被后台工作线程异步处理
             */
            task_ids = server_task::get_list_id(tasks);      /* 提取所有任务ID */
            ctx_server.queue_results.add_waiting_tasks(tasks); /* 添加到等待结果列表 */
            ctx_server.queue_tasks.post(std::move(tasks));   /* 提交到任务队列 */
        }

        /*
         * 等待并获取嵌入结果
         * 这是一个阻塞操作，会等待所有嵌入任务完成
         */
        ctx_server.receive_multi_results(task_ids, [&](std::vector<server_task_result_ptr> & results) {
            /*
             * 成功回调：处理所有嵌入结果
             * 将每个结果转换为JSON格式并添加到响应数组
             */
            for (auto & res : results) {
                /*
                 * 类型安全检查：确保结果是嵌入类型
                 * 使用动态类型转换验证结果的正确性
                 */
                GGML_ASSERT(dynamic_cast<server_task_result_embd*>(res.get()) != nullptr);
                responses.push_back(res->to_json());
            }
        }, [&](const json & error_data) {
            /*
             * 错误回调：处理嵌入计算过程中的错误
             * 例如内存不足、模型加载失败等
             */
            res_error(res, error_data);
            error = true;
        }, req.is_connection_closed);

        /*
         * 清理等待列表中的任务ID
         * 无论成功还是失败，都需要从等待队列中移除
         */
        ctx_server.queue_results.remove_waiting_task_ids(task_ids);

        /*
         * 检查是否发生错误
         * 如果有错误，提前返回(错误响应已在回调中发送)
         */
        if (error) {
            return;
        }

        /*
         * 构建并发送JSON响应
         * 根据兼容性模式选择不同的响应格式
         */
        json root = oaicompat == OAICOMPAT_TYPE_EMBEDDING
            ? format_embeddings_response_oaicompat(body, responses, use_base64) /* OpenAI兼容格式 */
            : json(responses);                                                   /* llama.cpp原生格式 */
        res_ok(res, root);
    };

    /*
     * 标准嵌入接口处理器
     * 处理 /embeddings 端点，使用llama.cpp原生格式
     * 不进行OpenAI兼容性转换，直接使用原始响应格式
     */
    const auto handle_embeddings = [&handle_embeddings_impl](const httplib::Request & req, httplib::Response & res) {
        handle_embeddings_impl(req, res, OAICOMPAT_TYPE_NONE);
    };

    /*
     * OpenAI兼容嵌入接口处理器
     * 处理 /v1/embeddings 端点，兼容OpenAI Embeddings API格式
     * 响应格式符合OpenAI API规范，便于现有应用迁移
     */
    const auto handle_embeddings_oai = [&handle_embeddings_impl](const httplib::Request & req, httplib::Response & res) {
        handle_embeddings_impl(req, res, OAICOMPAT_TYPE_EMBEDDING);
    };

    /*
     * 文档重排序接口处理器
     * 处理 /rerank 端点，根据查询对文档列表进行相关性排序
     * 用于搜索系统中提高检索结果的精准度
     */
    const auto handle_rerank = [&ctx_server, &res_error, &res_ok](const httplib::Request & req, httplib::Response & res) {
        /*
         * 检查重排序功能是否启用
         * 重排序需要特定的embedding模式和pooling类型
         * 必须使用 --reranking 参数启动服务器
         */
        if (!ctx_server.params_base.embedding || ctx_server.params_base.pooling_type != LLAMA_POOLING_TYPE_RANK) {
            res_error(res, format_error_response("This server does not support reranking. Start it with `--reranking`", ERROR_TYPE_NOT_SUPPORTED));
            return;
        }

        /*
         * 解析请求体JSON数据
         * 包含查询文本和待排序的文档列表
         */
        const json body = json::parse(req.body);

        /*
         * TODO: 待实现的top_n功能
         * top_n参数用于限制返回的最相关文档数量
         * 目前被注释掉，将来可能会实现
         */
        //int top_n = 1;
        //if (body.count("top_n") != 1) {
        //    top_n = body.at("top_n");
        //} else {
        //    res_error(res, format_error_response("\"top_n\" must be provided", ERROR_TYPE_INVALID_REQUEST));
        //    return;
        //}

        /*
         * 检测API格式类型
         * 支持两种API格式：
         * - TEI (Text Embeddings Inference): 使用"texts"字段
         * - Jina: 使用"documents"字段
         * 参考链接:
         * Jina: https://jina.ai/reranker/
         * TEI: https://huggingface.github.io/text-embeddings-inference/#/Text%20Embeddings%20Inference/rerank
         */
        bool is_tei_format = body.contains("texts");

        /*
         * 提取并验证查询文本
         * 查询文本是用来对文档进行排序的基准
         */
        json query;
        if (body.count("query") == 1) {
            query = body.at("query");
            /*
             * 验证查询格式：必须是字符串类型
             * 不支持复杂的查询结构
             */
            if (!query.is_string()) {
                res_error(res, format_error_response("\"query\" must be a string", ERROR_TYPE_INVALID_REQUEST));
                return;
            }
        } else {
            /*
             * 查询文本缺失错误
             * query字段是重排序的必需参数
             */
            res_error(res, format_error_response("\"query\" must be provided", ERROR_TYPE_INVALID_REQUEST));
            return;
        }

        /*
         * 提取文档列表
         * 支持两种字段名：documents(Jina格式) 和 texts(TEI格式)
         * 优先使用documents，如果不存在则尝试texts
         */
        std::vector<std::string> documents = json_value(body, "documents",
                                             json_value(body, "texts", std::vector<std::string>()));
        if (documents.empty()) {
            /*
             * 文档列表验证
             * 必须提供至少一个文档用于排序
             */
            res_error(res, format_error_response("\"documents\" must be a non-empty string array", ERROR_TYPE_INVALID_REQUEST));
            return;
        }

        /*
         * 令牌化查询文本
         * 不添加特殊token，直接处理原始文本
         * [0]取第一个结果，因为查询只有一个字符串
         */
        llama_tokens tokenized_query = tokenize_input_prompts(ctx_server.vocab, query, /* add_special */ false, true)[0];

        /*
         * 创建并提交重排序任务
         * 为每个文档创建一个与查询的相关性评分任务
         */
        json responses = json::array();  /* 存储所有排序结果 */
        bool error = false;              /* 错误标志 */
        std::unordered_set<int> task_ids; /* 任务ID集合，用于跟踪 */
        {
            /*
             * 任务创建代码块
             * 限制tasks变量的作用域，及时释放内存
             */
            std::vector<server_task> tasks;
            
            /*
             * 令牌化所有文档
             * 不添加特殊token，保持文档的原始语义
             */
            auto tokenized_docs = tokenize_input_prompts(ctx_server.vocab, documents, /* add_special */ false, true);
            
            /*
             * 预分配任务容器空间
             * 提高性能，避免动态扩容
             */
            tasks.reserve(tokenized_docs.size());
            
            /*
             * 为每个文档创建重排序任务
             * 每个任务计算一个文档与查询的相关性得分
             */
            for (size_t i = 0; i < tokenized_docs.size(); i++) {
                /*
                 * 格式化重排序输入
                 * format_rerank函数将查询和文档组合成模型能理解的格式
                 * 通常是 [查询] [分隔符] [文档] 的形式
                 */
                auto tmp = format_rerank(ctx_server.vocab, tokenized_query, tokenized_docs[i]);
                
                /* 创建重排序任务 */
                server_task task   = server_task(SERVER_TASK_TYPE_RERANK);
                task.id            = ctx_server.queue_tasks.get_new_id(); /* 获取唯一任务ID */
                task.index         = i;                                   /* 文档在批处理中的索引 */
                task.prompt_tokens = server_tokens(tmp, ctx_server.mctx != nullptr); /* 格式化后的令牌序列 */
                
                /* 将任务添加到批处理列表 */
                tasks.push_back(std::move(task));
            }

            /*
             * 提交任务到处理队列
             * 这些任务会被后台工作线程异步处理
             */
            task_ids = server_task::get_list_id(tasks);      /* 提取所有任务ID */
            ctx_server.queue_results.add_waiting_tasks(tasks); /* 添加到等待结果列表 */
            ctx_server.queue_tasks.post(std::move(tasks));   /* 提交到任务队列 */
        }

        /*
         * 等待并获取重排序结果
         * 收集所有文档的相关性得分
         */
        ctx_server.receive_multi_results(task_ids, [&](std::vector<server_task_result_ptr> & results) {
            /*
             * 成功回调：处理所有重排序结果
             * 每个结果包含一个文档的相关性得分
             */
            for (auto & res : results) {
                /*
                 * 类型安全检查：确保结果是重排序类型
                 * 使用动态类型转换验证结果的正确性
                 */
                GGML_ASSERT(dynamic_cast<server_task_result_rerank*>(res.get()) != nullptr);
                responses.push_back(res->to_json());
            }
        }, [&](const json & error_data) {
            /*
             * 错误回调：处理重排序计算过程中的错误
             * 例如模型加载失败、内存不足等
             */
            res_error(res, error_data);
            error = true;
        }, req.is_connection_closed);

        /*
         * 检查是否发生错误
         * 如果有错误，提前返回(错误响应已在回调中发送)
         */
        if (error) {
            return;
        }

        /*
         * 构建并发送JSON响应
         * 格式化重排序结果，按相关性得分排序文档
         */
        json root = format_response_rerank(
            body,           /* 原始请求体 */
            responses,      /* 所有文档的得分结果 */
            is_tei_format,  /* API格式类型(TEI或Jina) */
            documents       /* 原始文档列表 */
        );

        res_ok(res, root);
    };

    /*
     * LoRA适配器列表查询接口处理器
     * 处理 /lora_adapters 端点，返回当前加载的所有LoRA适配器信息
     * LoRA (Low-Rank Adaptation) 是一种参数高效的微调技术
     */
    const auto handle_lora_adapters_list = [&](const httplib::Request &, httplib::Response & res) {
        /*
         * 初始化结果数组
         * 用于存储所有LoRA适配器的信息
         */
        json result = json::array();
        
        /*
         * 获取服务器配置的LoRA适配器列表
         * 这些适配器在服务器启动时通过命令行参数配置
         */
        const auto & loras = ctx_server.params_base.lora_adapters;
        
        /*
         * 遍历所有LoRA适配器
         * 将每个适配器的信息转换为JSON对象
         */
        for (size_t i = 0; i < loras.size(); ++i) {
            auto & lora = loras[i];
            /*
             * 构建适配器信息对象
             * 包含ID、文件路径和缩放因子
             */
            result.push_back({
                {"id", i},              /* 适配器的唯一标识符(索引) */
                {"path", lora.path},    /* LoRA权重文件的路径 */
                {"scale", lora.scale},  /* 适配器的缩放因子(影响强度) */
            });
        }
        
        /*
         * 返回成功响应
         * 包含所有可用的LoRA适配器信息
         */
        res_ok(res, result);
        res.status = 200; // HTTP OK
    };

    /*
     * LoRA适配器应用接口处理器
     * 处理 /lora_adapters/apply 端点，动态切换或组合LoRA适配器
     * 允许在运行时改变模型的行为，无需重启服务器
     */
    const auto handle_lora_adapters_apply = [&](const httplib::Request & req, httplib::Response & res) {
        /*
         * 解析请求体JSON数据
         * 应包含要应用的LoRA适配器配置数组
         */
        const json body = json::parse(req.body);
        
        /*
         * 验证请求体格式
         * 必须是JSON数组，每个元素描述一个适配器的应用配置
         */
        if (!body.is_array()) {
            res_error(res, format_error_response("Request body must be an array", ERROR_TYPE_INVALID_REQUEST));
            return;
        }

        /*
         * 创建LoRA配置任务
         * 这是一个同步操作，需要等待完成
         */
        int task_id = ctx_server.queue_tasks.get_new_id();
        {
            /*
             * 构建LoRA设置任务
             * 任务包含新的适配器配置信息
             */
            server_task task(SERVER_TASK_TYPE_SET_LORA);
            task.id = task_id;
            /*
             * 解析LoRA应用请求
             * parse_lora_request函数验证并转换请求格式
             * 检查适配器ID的有效性、缩放因子的合理性等
             */
            task.set_lora = parse_lora_request(ctx_server.params_base.lora_adapters, body);
            
            /*
             * 提交任务并等待结果
             * LoRA切换需要修改模型权重，是一个相对重要的操作
             */
            ctx_server.queue_results.add_waiting_task_id(task_id);
            ctx_server.queue_tasks.post(std::move(task));
        }

        /*
         * 等待并获取任务执行结果
         * 这是一个阻塞操作，会等待LoRA应用完成
         */
        server_task_result_ptr result = ctx_server.queue_results.recv(task_id);
        ctx_server.queue_results.remove_waiting_task_id(task_id);

        /*
         * 检查任务执行结果
         * 如果应用失败，返回错误信息
         */
        if (result->is_error()) {
            res_error(res, result->to_json());
            return;
        }

        /*
         * 类型安全检查：确保结果是LoRA应用类型
         * 验证返回结果的正确性
         */
        GGML_ASSERT(dynamic_cast<server_task_result_apply_lora*>(result.get()) != nullptr);
        res_ok(res, result->to_json());
    };

    /*
     * ================================
     * HTTP路由注册和服务器配置
     * ================================
     * 这部分代码负责设置HTTP服务器的路由规则
     * 将URL路径映射到对应的处理函数
     */

    /*
     * Web UI配置
     * 决定是否提供Web用户界面服务
     */
    if (!params.webui) {
        /*
         * Web UI被禁用
         * 服务器只提供API接口，不提供Web界面
         */
        LOG_INF("Web UI is disabled\n");
    } else {
        /*
         * Web UI已启用
         * 配置静态文件服务或嵌入式Web界面
         */
        
        /*
         * 静态文件路由注册
         * 检查是否指定了自定义的静态文件目录
         */
        if (!params.public_path.empty()) {
            /*
             * 使用外部静态文件目录
             * 将指定目录挂载到HTTP根路径，用于提供HTML、CSS、JS等静态资源
             */
            bool is_found = svr->set_mount_point(params.api_prefix + "/", params.public_path);
            if (!is_found) {
                /*
                 * 静态文件目录不存在或无法访问
                 * 这是一个致命错误，服务器无法启动
                 */
                LOG_ERR("%s: static assets path not found: %s\n", __func__, params.public_path.c_str());
                return 1;
            }
        } else {
            /*
             * 使用嵌入式Web界面
             * 静态HTML文件已编译到二进制文件中，无需外部文件
             */
            svr->Get(params.api_prefix + "/", [](const httplib::Request & req, httplib::Response & res) {
                /*
                 * 检查浏览器是否支持gzip压缩
                 * 嵌入的HTML文件使用gzip压缩以减小体积
                 */
                if (req.get_header_value("Accept-Encoding").find("gzip") == std::string::npos) {
                    /*
                     * 浏览器不支持gzip
                     * 返回错误信息，现代浏览器都应该支持gzip
                     */
                    res.set_content("Error: gzip is not supported by this browser", "text/plain");
                } else {
                    /*
                     * 设置gzip压缩响应头
                     * 告诉浏览器内容是gzip压缩的
                     */
                    res.set_header("Content-Encoding", "gzip");
                    
                    /*
                     * 设置跨域安全策略头
                     * COEP和COOP头部是pyodide(Python解释器)所必需的
                     * 这些头部增强了Web应用的安全性
                     */
                    res.set_header("Cross-Origin-Embedder-Policy", "require-corp");
                    res.set_header("Cross-Origin-Opener-Policy", "same-origin");
                    
                    /*
                     * 返回嵌入式HTML内容
                     * index_html_gz是编译时嵌入的gzip压缩HTML数据
                     */
                    res.set_content(reinterpret_cast<const char*>(index_html_gz), index_html_gz_len, "text/html; charset=utf-8");
                }
                return false; /* 表示请求已处理完成 */
            });
        }
    }

    /*
     * ================================
     * API路由注册
     * ================================
     * 将HTTP端点映射到对应的处理函数
     * 支持多种API格式：llama.cpp原生、OpenAI兼容、Ollama兼容
     */

    /*
     * 系统状态和信息接口
     * 用于监控服务器健康状态和获取系统信息
     */
    svr->Get (params.api_prefix + "/health",              handle_health);         /* 健康检查 - 公开端点(无需API密钥) */
    svr->Get (params.api_prefix + "/metrics",             handle_metrics);        /* 性能指标监控 */
    svr->Get (params.api_prefix + "/props",               handle_props);          /* 获取服务器属性配置 */
    svr->Post(params.api_prefix + "/props",               handle_props_change);   /* 动态修改服务器配置 */
    svr->Post(params.api_prefix + "/api/show",            handle_api_show);       /* 显示API详细信息 */

    /*
     * 模型信息查询接口
     * 支持多种API格式，便于不同客户端集成
     */
    svr->Get (params.api_prefix + "/models",              handle_models);         /* llama.cpp原生格式 - 公开端点(无需API密钥) */
    svr->Get (params.api_prefix + "/v1/models",           handle_models);         /* OpenAI兼容格式 - 公开端点(无需API密钥) */
    svr->Get (params.api_prefix + "/api/tags",            handle_models);         /* Ollama特定端点 - 公开端点(无需API密钥) */

    /*
     * 文本生成接口
     * 核心功能：文本补全和聊天对话
     */
    svr->Post(params.api_prefix + "/completion",          handle_completions);    /* 传统补全接口(已弃用) */
    svr->Post(params.api_prefix + "/completions",         handle_completions);    /* llama.cpp原生补全接口 */
    svr->Post(params.api_prefix + "/v1/completions",      handle_completions_oai); /* OpenAI兼容补全接口 */
    svr->Post(params.api_prefix + "/chat/completions",    handle_chat_completions); /* llama.cpp聊天接口 */
    svr->Post(params.api_prefix + "/v1/chat/completions", handle_chat_completions); /* OpenAI兼容聊天接口 */
    svr->Post(params.api_prefix + "/api/chat",            handle_chat_completions); /* Ollama特定聊天端点 */

    /*
     * 代码补全接口
     * 专门用于Fill-In-the-Middle(FIM)代码生成
     */
    svr->Post(params.api_prefix + "/infill",              handle_infill);         /* 代码填充接口 */

    /*
     * 嵌入向量生成接口
     * 用于文本的向量化表示，支持语义搜索等任务
     */
    svr->Post(params.api_prefix + "/embedding",           handle_embeddings);     /* 传统嵌入接口(已弃用) */
    svr->Post(params.api_prefix + "/embeddings",          handle_embeddings);     /* llama.cpp原生嵌入接口 */
    svr->Post(params.api_prefix + "/v1/embeddings",       handle_embeddings_oai); /* OpenAI兼容嵌入接口 */

    /*
     * 文档重排序接口
     * 根据查询对文档进行相关性排序，提高搜索精度
     */
    svr->Post(params.api_prefix + "/rerank",              handle_rerank);         /* 重排序接口 */
    svr->Post(params.api_prefix + "/reranking",           handle_rerank);         /* 重排序接口(别名) */
    svr->Post(params.api_prefix + "/v1/rerank",           handle_rerank);         /* v1版本重排序接口 */
    svr->Post(params.api_prefix + "/v1/reranking",        handle_rerank);         /* v1版本重排序接口(别名) */

    /*
     * 令牌化工具接口
     * 用于调试和分析文本的令牌化过程
     */
    svr->Post(params.api_prefix + "/tokenize",            handle_tokenize);       /* 文本转令牌 */
    svr->Post(params.api_prefix + "/detokenize",          handle_detokenize);     /* 令牌转文本 */
    svr->Post(params.api_prefix + "/apply-template",      handle_apply_template); /* 应用聊天模板 */

    /*
     * LoRA适配器热切换接口
     * 允许在运行时动态切换模型适配器，改变模型行为
     */
    svr->Get (params.api_prefix + "/lora-adapters",       handle_lora_adapters_list); /* 查询可用LoRA适配器 */
    svr->Post(params.api_prefix + "/lora-adapters",       handle_lora_adapters_apply); /* 应用LoRA适配器配置 */

    /*
     * 插槽管理接口
     * 用于保存和加载推理状态，支持多会话管理
     */
    svr->Get (params.api_prefix + "/slots",               handle_slots);          /* 查询所有插槽状态 */
    svr->Post(params.api_prefix + "/slots/:id_slot",      handle_slots_action);   /* 对特定插槽执行操作 */

    /*
     * ================================
     * 启动HTTP服务器
     * ================================
     * 配置并启动HTTP服务器，开始监听客户端请求
     */

    /*
     * 配置HTTP服务器线程池
     * 决定服务器能够并发处理的HTTP请求数量
     */
    if (params.n_threads_http < 1) {
        /*
         * 自动计算HTTP线程数
         * +2线程用于监控端点
         * 取并行推理线程数+2与硬件线程数-1的较大值
         * 确保有足够的线程处理HTTP请求和监控任务
         */
        params.n_threads_http = std::max(params.n_parallel + 2, (int32_t) std::thread::hardware_concurrency() - 1);
    }
    log_data["n_threads_http"] =  std::to_string(params.n_threads_http);
    
    /*
     * 创建HTTP线程池工厂函数
     * 为HTTP服务器提供线程池，用于并发处理请求
     */
    svr->new_task_queue = [&params] { return new httplib::ThreadPool(params.n_threads_http); };

    /*
     * 定义清理函数
     * 在服务器退出前释放所有资源，确保优雅关闭
     */
    auto clean_up = [&svr, &ctx_server]() {
        SRV_INF("%s: cleaning up before exit...\n", __func__);
        svr->stop();                             /* 停止HTTP服务器 */
        ctx_server.queue_results.terminate();   /* 终止结果队列 */
        llama_backend_free();                   /* 释放llama后端资源 */
    };

    /*
     * 绑定网络监听地址
     * 支持TCP Socket和Unix Domain Socket两种模式
     */
    bool was_bound = false;  /* 绑定成功标志 */
    bool is_sock = false;    /* Unix Socket标志 */
    
    /*
     * 检查是否使用Unix Domain Socket
     * 如果hostname以.sock结尾，则使用Unix Socket
     */
    if (string_ends_with(std::string(params.hostname), ".sock")) {
        is_sock = true;
        LOG_INF("%s: setting address family to AF_UNIX\n", __func__);
        /*
         * 配置为Unix Socket模式
         * AF_UNIX地址族用于本地进程间通信
         */
        svr->set_address_family(AF_UNIX);
        /*
         * 绑定到Unix Socket文件
         * bind_to_port需要第二个参数，但对Unix Socket会被忽略
         */
        was_bound = svr->bind_to_port(params.hostname, 8080);
    } else {
        /*
         * 使用标准TCP Socket
         * 适用于网络访问和跨机器通信
         */
        LOG_INF("%s: binding port with default address family\n", __func__);
        
        /*
         * 绑定HTTP监听端口
         * 支持自动端口分配和指定端口两种模式
         */
        if (params.port == 0) {
            /*
             * 自动端口分配
             * 系统自动选择一个可用端口
             */
            int bound_port = svr->bind_to_any_port(params.hostname);
            if ((was_bound = (bound_port >= 0))) {
                params.port = bound_port;  /* 记录实际分配的端口号 */
            }
        } else {
            /*
             * 绑定到指定端口
             * 使用用户指定的端口号
             */
            was_bound = svr->bind_to_port(params.hostname, params.port);
        }
    }

    /*
     * 检查网络绑定结果
     * 如果绑定失败，清理资源并退出
     */
    if (!was_bound) {
        LOG_ERR("%s: couldn't bind HTTP server socket, hostname: %s, port: %d\n", __func__, params.hostname.c_str(), params.port);
        clean_up();
        return 1;
    }

    /*
     * 在独立线程中运行HTTP服务器
     * 避免阻塞主线程，允许并发处理HTTP请求和模型推理
     */
    std::thread t([&]() { svr->listen_after_bind(); });
    svr->wait_until_ready();  /* 等待HTTP服务器完全启动 */

    LOG_INF("%s: HTTP server is listening, hostname: %s, port: %d, http threads: %d\n", __func__, params.hostname.c_str(), params.port, params.n_threads_http);

    /*
     * ================================
     * 加载和初始化模型
     * ================================
     * 这是服务器启动的关键步骤，加载AI模型并准备推理
     */
    LOG_INF("%s: loading model\n", __func__);

    /*
     * 加载模型文件
     * 这个过程可能需要较长时间，取决于模型大小和硬件性能
     */
    if (!ctx_server.load_model(params)) {
        /*
         * 模型加载失败，清理资源并退出
         * 这通常是由于模型文件损坏、内存不足或格式不兼容等原因
         */
        clean_up();
        t.join();  /* 等待HTTP服务器线程结束 */
        LOG_ERR("%s: exiting due to model loading error\n", __func__);
        return 1;
    }

    /*
     * 初始化服务器上下文
     * 设置推理参数、分配内存、准备推理状态等
     */
    ctx_server.init();
    
    /*
     * 更新服务器状态为就绪
     * 此时服务器可以开始接受和处理推理请求
     */
    state.store(SERVER_STATE_READY);

    LOG_INF("%s: model loaded\n", __func__);

    /*
     * 打印聊天模板示例
     * 帮助用户了解模型使用的聊天格式，便于正确构造请求
     */
    LOG_INF("%s: chat template, chat_template: %s, example_format: '%s'\n", __func__,
        common_chat_templates_source(ctx_server.chat_templates.get()),
        common_chat_format_example(ctx_server.chat_templates.get(), ctx_server.params_base.use_jinja, ctx_server.params_base.default_template_kwargs).c_str());

    /*
     * ================================
     * 配置任务处理回调
     * ================================
     * 设置任务队列的事件处理器，实现异步任务处理
     */

    /*
     * 注册新任务处理回调
     * 当有新任务加入队列时，会调用此回调函数进行处理
     */
    ctx_server.queue_tasks.on_new_task([&ctx_server](server_task && task) {
        ctx_server.process_single_task(std::move(task));
    });

    /*
     * 注册插槽更新回调
     * 定期更新推理插槽的状态，管理并发会话
     */
    ctx_server.queue_tasks.on_update_slots([&ctx_server]() {
        ctx_server.update_slots();
    });

    /*
     * 设置优雅关闭处理器
     * 处理SIGINT、SIGTERM等信号，实现服务器的优雅停止
     */
    shutdown_handler = [&](int) {
        /*
         * 终止任务队列处理循环
         * 这会解除start_loop()的阻塞状态，让主线程继续执行
         */
        ctx_server.queue_tasks.terminate();
    };

    /*
     * ================================
     * 注册系统信号处理器
     * ================================
     * 配置跨平台的信号处理，实现优雅关闭功能
     */
#if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))
    /*
     * Unix/Linux/macOS系统的信号处理
     * 使用sigaction函数注册信号处理器
     */
    struct sigaction sigint_action;
    sigint_action.sa_handler = signal_handler;        /* 设置信号处理函数 */
    sigemptyset (&sigint_action.sa_mask);            /* 清空信号掩码 */
    sigint_action.sa_flags = 0;                      /* 设置信号处理标志 */
    sigaction(SIGINT, &sigint_action, NULL);         /* 注册SIGINT信号(Ctrl+C) */
    sigaction(SIGTERM, &sigint_action, NULL);        /* 注册SIGTERM信号(终止请求) */
#elif defined (_WIN32)
    /*
     * Windows系统的控制台事件处理
     * 使用SetConsoleCtrlHandler函数注册处理器
     */
    auto console_ctrl_handler = +[](DWORD ctrl_type) -> BOOL {
        /*
         * 检查是否为Ctrl+C事件
         * 如果是，调用信号处理器并返回true表示已处理
         */
        return (ctrl_type == CTRL_C_EVENT) ? (signal_handler(SIGINT), true) : false;
    };
    SetConsoleCtrlHandler(reinterpret_cast<PHANDLER_ROUTINE>(console_ctrl_handler), true);
#endif

    /*
     * 输出服务器启动完成信息
     * 显示监听地址，便于用户连接和测试
     */
    LOG_INF("%s: server is listening on %s - starting the main loop\n", __func__,
            is_sock ? string_format("unix://%s", params.hostname.c_str()).c_str() :      /* Unix Socket格式 */
                      string_format("http://%s:%d", params.hostname.c_str(), params.port).c_str()); /* HTTP格式 */

    /*
     * ================================
     * 启动主事件循环
     * ================================
     * 进入任务处理主循环，服务器开始正式工作
     */
    
    /*
     * 启动任务队列主循环
     * 这个调用会阻塞主线程，直到queue_tasks.terminate()被调用
     * 在这个循环中，服务器会不断处理新的推理任务
     */
    ctx_server.queue_tasks.start_loop();

    /*
     * ================================
     * 服务器关闭和资源清理
     * ================================
     * 当主循环结束后，执行清理工作并优雅退出
     */
    
    /*
     * 执行资源清理
     * 停止HTTP服务器、释放模型内存、清理队列等
     */
    clean_up();
    
    /*
     * 等待HTTP服务器线程结束
     * 确保所有线程都正确终止
     */
    t.join();

    /*
     * 程序正常退出
     * 返回0表示服务器成功完成了所有工作
     */
    return 0;
}
