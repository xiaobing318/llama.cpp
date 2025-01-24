/*
    Copyright 2024 Google LLC

    Use of this source code is governed by an MIT-style
    license that can be found in the LICENSE file or at
    https://opensource.org/licenses/MIT.
*/
// SPDX-License-Identifier: MIT

/*
Note:杨小兵-2025-01-23

1、2024：这是版权声明的年份，表明该代码在2024年由版权所有者发布或更新。
2、Google LLC：这是版权所有者的名称，指明该代码由Google有限责任公司拥有。
3、解释：
    3.1 使用权声明：指出使用此源代码需要遵循特定的许可证条款。
    3.2 MIT-style license：指该代码采用的是MIT许可证或类似于MIT许可证的条款。
    3.3 LICENSE文件：指向项目根目录中的一个文件，通常包含详细的许可证文本。
    3.4 URL链接：提供了一个指向MIT许可证官方文本的URL，便于用户查阅。
4、作用：
    4.1 明确许可条款：告知使用者可以在什么条件下使用、修改和分发该代码。
    4.2 开放源代码：MIT许可证是一种宽松的开源许可证，允许用户自由使用、复制、修改、合并、发布、分发、再许可和/或销售软件的副本，只需保留原始版权声明和许可证声明。
5、SPDX标识符的使用进一步标准化了许可证声明，便于自动化工具识别和处理。这些声明在开源项目中尤为重要，既保护了版权所有者的权益，又促进了社区的开放协作。
*/

#pragma once
/*
Note:杨小兵-2025-01-23

1、上述命令是一个预处理器指令，用于确保头文件只被包含一次，避免重复定义。
*/

#include "minja.hpp"
#include <json.hpp>
#include <string>
#include <vector>
/*
Note:杨小兵-2025-01-23

1、包含头文件，这样是为了减少重复
2、其中json.hpp、minja.hpp是一个只有头文件的第三方库
*/

using json = nlohmann::ordered_json;
/*
Note:杨小兵-2025-01-23

1、using命令同typedef在数据类型重命名的角度来理解是相同的作用。
2、其中ordered_json是nlohmann/json中的一个数据类型。
*/

namespace minja {

class chat_template {
  public:

  private:
    bool supports_tools_ = true;
    // Meta-Llama-3.1-8B-Instruct's template expects arguments to be an object.
    // Most other templates (and OpenAI's API) expect the arguments object to be stringified.
    bool requires_object_arguments_ = false;
    bool requires_typed_content_ = false;
    bool supports_system_role_ = true;
    bool supports_parallel_tool_calls_ = false;
    std::string source_;
    std::string bos_token_;
    std::string eos_token_;
    std::shared_ptr<minja::TemplateNode> template_root_;

    std::string try_raw_render(
        const nlohmann::ordered_json & messages,
        const nlohmann::ordered_json & tools,
        bool add_generation_prompt,
        const nlohmann::ordered_json & extra_context = nlohmann::ordered_json()) const
    {
        try {
            auto prompt = apply(messages, tools, add_generation_prompt, extra_context, /* adjust_inputs= */ false);
            // fprintf(stderr, "Prompt: %s\n", prompt.c_str());
            return prompt;
        } catch (const std::exception & e) {
            // fprintf(stderr, "Error: %s\n", e.what());
            return "";
        }
    }

  public:
    chat_template(const std::string & source, const std::string & bos_token, const std::string & eos_token)
        : source_(source), bos_token_(bos_token), eos_token_(eos_token)
    {
        template_root_ = minja::Parser::parse(source_, {
            /* .trim_blocks = */ true,
            /* .lstrip_blocks = */ true,
            /* .keep_trailing_newline = */ false,
        });
        supports_tools_ = source.find("tools") != std::string::npos;

        auto renders_string_arguments =
            try_raw_render({
                {
                    {"role", "user"},
                    {"content", "Hey"}
                },
                {
                    {"role", "assistant"},
                    {"tool_calls", json::array({
                        {
                            {"id", "call_1___"},
                            {"type", "function"},
                            {"function", {
                                {"arguments", "{\"code\": \"print('Hello, World!')\"}"},
                                {"name", "ipython"},
                            }},
                        },
                    })},
                }
            }, {}, false).find("{\"code\": \"print") != std::string::npos;
        if (!renders_string_arguments) {
            auto renders_object_arguments =
                try_raw_render({
                    {
                        {"role", "user"},
                        {"content", "Hey"}
                    },
                    {
                        {"role", "assistant"},
                        {"tool_calls", json::array({
                            {
                                {"id", "call_1___"},
                                {"type", "function"},
                                {"function", {
                                    {"arguments", {
                                        {"code", "print('Hello, World!')"},
                                    }},
                                    {"name", "ipython"},
                                }},
                            },
                        })},
                    }
                }, {}, false).find("{\"code\": \"print") != std::string::npos;
            requires_object_arguments_ = renders_object_arguments;
        }
        supports_parallel_tool_calls_ = source.find("tool_call_id") != std::string::npos;

        supports_system_role_ = try_raw_render({
            {{"role", "system"}, {"content", "<System Needle>"}},
            {{"role", "user"},   {"content", "Hey"}}
        }, {}, false).find("<System Needle>") != std::string::npos;

        requires_typed_content_ = try_raw_render({{{"role", "user"},   {"content", "Hey"}}}, {}, false).find("Hey") == std::string::npos
            && try_raw_render({{{"role", "user"},   {"content", {{{"type", "text"}, {"text", "Hey"}}}}}}, {}, false).find("Hey") != std::string::npos;
    }

    const std::string & source() const { return source_; }
    const std::string & bos_token() const { return bos_token_; }
    const std::string & eos_token() const { return eos_token_; }
    bool supports_tools() const { return supports_tools_; }
    bool supports_parallel_tool_calls() const { return supports_parallel_tool_calls_; }

    std::string apply(
        const nlohmann::ordered_json & messages,
        const nlohmann::ordered_json & tools,
        bool add_generation_prompt,
        const nlohmann::ordered_json & extra_context = nlohmann::ordered_json(),
        bool adjust_inputs = true) const
    {
        json actual_messages;

        // First, "fix" messages so they have a chance to be rendered correctly by the template

        if (adjust_inputs && (requires_object_arguments_ || !supports_system_role_ || !supports_tools_ || requires_typed_content_)) {
            actual_messages = json::array();

            auto add_message = [&](const json & msg) {
                if (requires_typed_content_ && msg.contains("content") && !msg.at("content").is_null() && msg.at("content").is_string()) {
                    actual_messages.push_back({
                        {"role", msg.at("role")},
                        {"content", {{
                            {"type", "text"},
                            {"text", msg.at("content")},
                        }}},
                    });
                } else {
                    actual_messages.push_back(msg);
                }
            };

            std::string pending_system;
            auto flush_sys = [&]() {
                if (!pending_system.empty()) {
                    add_message({
                        {"role", "user"},
                        {"content", pending_system},
                    });
                    pending_system.clear();
                }
            };
            for (const auto & message_ : messages) {
                auto message = message_;
                if (!message.contains("role") || !message.contains("content")) {
                    throw std::runtime_error("message must have 'role' and 'content' fields: " + message.dump());
                }
                std::string role = message.at("role");

                if (message.contains("tool_calls")) {
                    if (requires_object_arguments_ || !supports_tools_) {
                        for (auto & tool_call : message.at("tool_calls")) {
                            if (tool_call["type"] == "function") {
                                auto & function = tool_call.at("function");
                                std::string arguments = function.at("arguments");
                                function["arguments"] = json::parse(arguments);
                            }
                        }
                    }
                    if (!supports_tools_) {
                        auto content = message.at("content");
                        auto tool_calls = json::array();
                        for (const auto & tool_call : message.at("tool_calls")) {
                            if (tool_call.at("type") != "function") {
                                continue;
                            }
                            const auto & function = tool_call.at("function");
                            auto tc = json {
                                {"name", function.at("name")},
                                {"arguments", function.at("arguments")},
                            };
                            if (tool_call.contains("id")) {
                                tc["id"] = tool_call["id"];
                            }
                            tool_calls.push_back(tc);
                        }
                        auto obj = json {
                            {"tool_calls", tool_calls},
                        };
                        if (!content.is_null() && content != "") {
                            obj["content"] = content;
                        }
                        message["content"] = obj.dump(2);
                        message.erase("tool_calls");
                    }
                }
                if (!supports_tools_ && role == "tool") {
                    message["role"] = "user";
                    auto obj = json {
                        {"tool_response", {
                            {"tool", message.at("name")},
                            {"content", message.at("content")},
                        }},
                    };
                    if (message.contains("tool_call_id")) {
                        obj["tool_response"]["tool_call_id"] = message.at("tool_call_id");
                    }
                    message["content"] = obj.dump(2);
                    message.erase("name");
                }

                if (!message["content"].is_null() && !supports_system_role_) {
                    std::string content = message.at("content");
                    if (role == "system") {
                        if (!pending_system.empty()) pending_system += "\n";
                        pending_system += content;
                        continue;
                    } else {
                        if (role == "user") {
                            if (!pending_system.empty()) {
                                message["content"] = pending_system + (content.empty() ? "" : "\n" + content);
                                pending_system.clear();
                            }
                        } else {
                            flush_sys();
                        }
                    }
                }
                add_message(message);
            }
            flush_sys();
        } else {
            actual_messages = messages;
        }

        auto context = minja::Context::make(json({
            {"messages", actual_messages},
            {"add_generation_prompt", add_generation_prompt},
            {"bos_token", bos_token_},
            {"eos_token", eos_token_},
        }));

        if (!tools.is_null()) {
            auto tools_val = minja::Value(tools);
            context->set("tools", tools_val);
        }
        if (!extra_context.is_null()) {
            for (auto & kv : extra_context.items()) {
                minja::Value val(kv.value());
                context->set(kv.key(), val);
            }
        }

        return template_root_->render(context);
    }
};

}  // namespace minja
