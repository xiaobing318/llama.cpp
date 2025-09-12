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
    // 定义函数类型以方便使用
    using ToolFunction = std::function<json(const json&)>;

    ToolExecutor();
    ~ToolExecutor() = default;

    // Register a tool with its implementation
    bool registerExternalTools(const json& tool_definition);

    // Execute a tool by name with arguments
    json execute(const std::string& name, const json& arguments) const;

    // Check if a tool exists
    bool hasTool(const std::string & name) const;

    // Get all registered tools
    json getTools() const;

private:
    // 仅包含内置工具的名称和实现映射
    std::unordered_map<std::string, ToolFunction> builtinTools;

    // 包含内置工具和外部工具定义的映射
    std::unordered_map<std::string, json> tool_definitions;

    // 访问工具注册表的线程安全保护
    mutable std::mutex tools_mutex;

    // Register built-in tools
    void registerBuiltinTools();

    // Execute external tool with command template and timeout (ms, -1 = no timeout)
    json executeExternalTool(const std::string& executable, const json& arguments, const std::string& command_template, long long timeout_ms) const;

    // Build command line from template and arguments (for logging/diagnostics)
    std::string buildCommandFromTemplate(const std::string& command_template, const json& arguments, const std::string& executable = "");

    // Validate tool definition JSON schema
    bool validateToolDefinition(const json& tool_definition, std::string& error_message) const;
};
