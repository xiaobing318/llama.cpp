#pragma once
#include "json.hpp"

#include <string>
#include <functional>
#include <unordered_map>
#include <mutex>

using json = nlohmann::ordered_json;

/*
职责：负责注册内置/外部工具、按名称执行工具、用模板拼装命令、通过子进程执行命令并收集输出。
依赖：参数校验在 qcopilot_utils.*，内置工具清单在 qcopilot_builtin_tools.*。
*/
class ToolExecutor {
public:
    // 定义函数类型以方便使用，这个函数类型的定义放在类的内部是为了只供该类使用
    using ToolFunction = std::function<json(const json&)>;

    ToolExecutor();
    ~ToolExecutor() = default;

    // 注册一个外部工具的工具调用定义，没有外部工具的具体实现
    bool registerExternalTools(const json& tool_definition);

    // 给定工具名称、工具参数执行对应的工具
    json execute(const std::string& name, const json& arguments) const;

    // 检查给定名称的工具是否存在
    bool hasTool(const std::string & name) const;

    // 获取得到所有已注册的工具
    json getTools() const;

private:
    // 工具的名称和实现的映射（内置工具 + 外部工具）
    std::unordered_map<std::string, ToolFunction> tool_functions;

    // 工具的名称和定义的映射（内置工具 + 外部工具）
    std::unordered_map<std::string, json> tool_definitions;

    // 访问工具注册表的线程安全保护
    mutable std::mutex tools_mutex;

    // 注册内置工具，包括内置工具的实现
    void registerBuiltinTools();

    // 使用命令模板和超时执行外部工具 (ms, -1 = no timeout)
    json executeExternalTool(
        const std::string& executable,
        const json& arguments,
        const std::string& command_template,
        long long timeout_ms) const;

    // 从模板和参数构建命令行（用于日志记录/诊断）
    std::string buildCommandFromTemplate(
        const std::string& command_template,
        const json& arguments,
        const std::string& executable = "");
};
