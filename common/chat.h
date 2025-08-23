// Chat support (incl. tool call grammar constraining & output parsing) w/ generic & custom template handlers.

#pragma once

#include "common.h"
#include <functional>
#include <chrono>
#include <string>
#include <vector>
#include <map>

struct common_chat_templates;

struct common_chat_tool_call {
    std::string name;
    std::string arguments;
    std::string id;

    bool operator==(const common_chat_tool_call & other) const {
        return name == other.name && arguments == other.arguments && id == other.id;
    }
};

struct common_chat_msg_content_part {
    std::string type;
    std::string text;

    bool operator==(const common_chat_msg_content_part & other) const {
        return type == other.type && text == other.text;
    }
};

/*
 * ========== OpenAI兼容聊天消息结构定义 ==========
 * 
 * 这个结构体定义了符合OpenAI Chat Completions API标准的消息格式。
 * 用于在llama.cpp内部表示和处理聊天对话消息，支持多模态内容和工具调用。
 *
 * 设计原理:
 * - 完全兼容OpenAI Chat API的消息格式规范
 * - 支持角色系统(system/user/assistant/tool)
 * - 支持多模态内容(文本+图片等)
 * - 支持工具调用和函数功能
 * - 提供与OpenAI API的无缝互操作性
 *
 * 应用场景:
 * - 聊天机器人和对话系统的消息存储
 * - HTTP请求解析后的内部数据表示  
 * - 模板渲染和提示词构建
 * - 工具调用和函数执行的参数传递
 */
struct common_chat_msg {
    /*
     * role - 消息角色标识
     * 
     * 符合OpenAI标准的角色类型:
     * - "system": 系统提示消息，定义AI的行为和规则
     * - "user": 用户输入消息，包含用户的问题或请求
     * - "assistant": AI助手回复消息，包含模型生成的响应
     * - "tool": 工具执行结果消息，包含函数调用的返回值
     * 
     * 作用:
     * - 区分对话中不同参与者的消息
     * - 为模型提供上下文理解的角色信息
     * - 支持多轮对话和角色扮演场景
     */
    std::string role;
    
    /*
     * content - 消息文本内容
     * 
     * 包含消息的主要文本信息:
     * - 用户的问题或指令(role="user"时)
     * - AI的回复内容(role="assistant"时)  
     * - 系统提示信息(role="system"时)
     * - 工具执行结果(role="tool"时)
     * 
     * 注意: 当使用content_parts多模态内容时，此字段应为空
     */
    std::string content;
    
    /*
     * content_parts - 多模态内容部分数组
     * 
     * 支持包含多种类型内容的复杂消息:
     * - 文本片段 (type="text")
     * - 图片内容 (type="image_url")  
     * - 音频内容 (type="audio")
     * 
     * 应用场景:
     * - 图文混合的多模态对话
     * - 需要同时处理文本和媒体内容的应用
     * - 复杂的结构化输入消息
     * 
     * 注意: 与content字段互斥，不能同时使用
     */
    std::vector<common_chat_msg_content_part> content_parts = {};
    
    /*
     * tool_calls - 工具调用请求数组
     * 
     * 当AI需要调用外部函数或工具时使用:
     * - 函数名称和参数信息
     * - 调用ID用于结果匹配
     * - 支持并行多个工具调用
     * 
     * 应用场景:
     * - 函数调用和API集成
     * - 外部数据查询和计算
     * - 工具增强的AI助手功能
     * 
     * 对应OpenAI的tool_calls字段
     */
    std::vector<common_chat_tool_call> tool_calls = {};
    
    /*
     * reasoning_content - 推理过程内容
     * 
     * 用于支持思维链(Chain of Thought)和推理展示:
     * - 存储AI的内部思考过程
     * - 支持可解释的AI推理
     * - 类似OpenAI o1模型的thinking过程
     * 
     * 应用场景:
     * - 需要展示推理过程的应用
     * - 教育和解释性AI系统
     * - 调试和分析AI的决策逻辑
     */
    std::string reasoning_content;
    
    /*
     * tool_name - 工具名称
     * 
     * 当消息类型为"tool"时使用:
     * - 标识执行结果来自哪个工具
     * - 用于工具调用的结果回传
     * - 对应OpenAI API中的name字段
     */
    std::string tool_name;
    
    /*
     * tool_call_id - 工具调用标识
     * 
     * 用于关联工具调用请求和响应:
     * - 唯一标识一次工具调用
     * - 支持异步和并行工具执行
     * - 确保结果正确匹配到相应的调用请求
     * 
     * 对应OpenAI API中的tool_call_id字段
     */
    std::string tool_call_id;

    /*
     * to_json_oaicompat() - OpenAI兼容JSON序列化方法
     * 
     * 将内部消息结构转换为符合OpenAI API标准的JSON格式。
     * 根据模板参数T的类型，生成对应的JSON对象。
     * 
     * 实现位置: common/chat.cpp:260-326
     * 转换逻辑: 
     * - 处理role和content的基本映射
     * - 转换content_parts为OpenAI格式的内容数组
     * - 处理tool_calls的函数调用格式
     * - 支持reasoning_content的推理内容展示
     */
    template <class T> T to_json_oaicompat() const;

    bool empty() const {
        return content.empty() && content_parts.empty() && tool_calls.empty() && reasoning_content.empty() && tool_name.empty() && tool_call_id.empty();
    }
    void ensure_tool_call_ids_set(std::vector<std::string> & ids_cache, const std::function<std::string()> & gen_tool_call_id) {
        for (auto i = 0u; i < tool_calls.size(); i++) {
            if (ids_cache.size() <= i) {
                auto id = tool_calls[i].id;
                if (id.empty()) {
                    id = gen_tool_call_id();
                }
                ids_cache.push_back(id);
            }
            tool_calls[i].id = ids_cache[i];
        }
    }
    bool operator==(const common_chat_msg & other) const {
        return role == other.role
            && content == other.content
            && content_parts == other.content_parts
            && tool_calls == other.tool_calls
            && reasoning_content == other.reasoning_content
            && tool_name == other.tool_name
            && tool_call_id == other.tool_call_id;
    }
    bool operator!=(const common_chat_msg & other) const {
        return !(*this == other);
    }
};

struct common_chat_msg_diff {
    std::string reasoning_content_delta;
    std::string content_delta;
    size_t tool_call_index = std::string::npos;
    common_chat_tool_call tool_call_delta;

    static std::vector<common_chat_msg_diff> compute_diffs(const common_chat_msg & previous_msg, const common_chat_msg & new_msg);

    bool operator==(const common_chat_msg_diff & other) const {
        return content_delta == other.content_delta
        && tool_call_index == other.tool_call_index
        && tool_call_delta == other.tool_call_delta;
    }
};

struct common_chat_tool {
    std::string name;
    std::string description;
    std::string parameters;
};

enum common_chat_tool_choice {
    COMMON_CHAT_TOOL_CHOICE_AUTO,
    COMMON_CHAT_TOOL_CHOICE_REQUIRED,
    COMMON_CHAT_TOOL_CHOICE_NONE,
};

enum common_chat_format {
    COMMON_CHAT_FORMAT_CONTENT_ONLY,
    COMMON_CHAT_FORMAT_GENERIC,
    COMMON_CHAT_FORMAT_MISTRAL_NEMO,
    COMMON_CHAT_FORMAT_LLAMA_3_X,
    COMMON_CHAT_FORMAT_LLAMA_3_X_WITH_BUILTIN_TOOLS,
    COMMON_CHAT_FORMAT_DEEPSEEK_R1,
    COMMON_CHAT_FORMAT_FIREFUNCTION_V2,
    COMMON_CHAT_FORMAT_FUNCTIONARY_V3_2,
    COMMON_CHAT_FORMAT_FUNCTIONARY_V3_1_LLAMA_3_1,
    COMMON_CHAT_FORMAT_HERMES_2_PRO,
    COMMON_CHAT_FORMAT_COMMAND_R7B,
    COMMON_CHAT_FORMAT_GRANITE,
    COMMON_CHAT_FORMAT_GPT_OSS,

    COMMON_CHAT_FORMAT_COUNT, // Not a format, just the # formats
};

struct common_chat_templates_inputs {
    std::vector<common_chat_msg> messages;
    std::string grammar;
    std::string json_schema;
    bool add_generation_prompt = true;
    bool use_jinja = true;
    // Parameters below only supported when use_jinja is true
    std::vector<common_chat_tool> tools;
    common_chat_tool_choice tool_choice = COMMON_CHAT_TOOL_CHOICE_AUTO;
    bool parallel_tool_calls = false;
    common_reasoning_format reasoning_format = COMMON_REASONING_FORMAT_NONE;
    bool enable_thinking = true;
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::map<std::string, std::string> chat_template_kwargs;
    bool add_bos = false;
    bool add_eos = false;
};

struct common_chat_params {
    common_chat_format                  format = COMMON_CHAT_FORMAT_CONTENT_ONLY;
    std::string                         prompt;
    std::string                         grammar;
    bool                                grammar_lazy = false;
    bool                                thinking_forced_open = false;
    std::vector<common_grammar_trigger> grammar_triggers;
    std::vector<std::string>            preserved_tokens;
    std::vector<std::string>            additional_stops;
};

struct common_chat_syntax {
    common_chat_format       format                = COMMON_CHAT_FORMAT_CONTENT_ONLY;
    common_reasoning_format  reasoning_format      = COMMON_REASONING_FORMAT_NONE;
    // Whether reasoning_content should be inlined in the content (e.g. for reasoning_format=deepseek in stream mode)
    bool                     reasoning_in_content  = false;
    bool                     thinking_forced_open  = false;
    bool                     parse_tool_calls      = true;
};

// Check if the template supplied via "--chat-template" is supported or not. Returns true if it's valid
bool common_chat_verify_template(const std::string & tmpl, bool use_jinja);

void common_chat_templates_free(struct common_chat_templates * tmpls);

struct common_chat_templates_deleter { void operator()(common_chat_templates * tmpls) { common_chat_templates_free(tmpls); } };

typedef std::unique_ptr<struct common_chat_templates, common_chat_templates_deleter> common_chat_templates_ptr;

common_chat_templates_ptr common_chat_templates_init(
                                    const struct llama_model * model,
                                           const std::string & chat_template_override,
                                           const std::string & bos_token_override = "",
                                           const std::string & eos_token_override = "");

bool         common_chat_templates_was_explicit(const struct common_chat_templates * tmpls);
const char * common_chat_templates_source(const struct common_chat_templates * tmpls, const char * variant = nullptr);


struct common_chat_params      common_chat_templates_apply(
    const struct common_chat_templates * tmpls,
    const struct common_chat_templates_inputs & inputs);

// Format single message, while taking into account the position of that message in chat history
std::string common_chat_format_single(
        const struct common_chat_templates * tmpls,
        const std::vector<common_chat_msg> & past_msg,
        const common_chat_msg & new_msg,
        bool add_ass,
        bool use_jinja);

// Returns an example of formatted chat
std::string common_chat_format_example(
    const struct common_chat_templates * tmpls,
    bool use_jinja,
    const std::map<std::string, std::string> & chat_template_kwargs);

const char*               common_chat_format_name(common_chat_format format);
const char*               common_reasoning_format_name(common_reasoning_format format);
common_reasoning_format   common_reasoning_format_from_name(const std::string & format);
common_chat_msg           common_chat_parse(const std::string & input, bool is_partial, const common_chat_syntax & syntax);

common_chat_tool_choice common_chat_tool_choice_parse_oaicompat(const std::string & tool_choice);

// Parses a JSON array of messages in OpenAI's chat completion API format.
// T can be std::string containing JSON or nlohmann::ordered_json
template <class T> std::vector<common_chat_msg> common_chat_msgs_parse_oaicompat(const T & messages);
template <class T> T common_chat_msgs_to_json_oaicompat(const std::vector<common_chat_msg> & msgs, bool concat_typed_text = false);

// Parses a JSON array of tools in OpenAI's chat completion tool call API format.
// T can be std::string containing JSON or nlohmann::ordered_json
template <class T> std::vector<common_chat_tool> common_chat_tools_parse_oaicompat(const T & tools);
template <class T> T common_chat_tools_to_json_oaicompat(const std::vector<common_chat_tool> & tools);

template <class T> T common_chat_msg_diff_to_json_oaicompat(const common_chat_msg_diff & diff);
