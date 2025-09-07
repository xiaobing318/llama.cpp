#pragma once
#include "json.hpp"

#include <string>
#include <functional>
#include <unordered_map>


using json = nlohmann::ordered_json;

class ToolExecutor {
public:
    using ToolFunction = std::function<json(const json&)>;

    ToolExecutor();
    ~ToolExecutor() = default;

    // Register a tool with its implementation
    bool registerExternalTools(const json& tool_definition);

    // Execute a tool by name with arguments
    json execute(const std::string& name, const json& arguments);

    // Check if a tool exists
    bool hasTool(const std::string & name) const;

    // Get all registered tools
    json getTools() const;

private:
    // 仅包含内置工具的名称和实现映射
    std::unordered_map<std::string, ToolFunction> builtinTools;

    // 包含内置工具和外部工具定义的映射
    std::unordered_map<std::string, json> tool_definitions;

    // Register built-in tools
    void registerBuiltinTools();

    // Execute external tool (backward compatibility)
    json executeExternalTool(const std::string& executable, const json& arguments);

    // Execute external tool with command template
    json executeExternalTool(const std::string& executable, const json& arguments, const std::string& command_template);

    // Build command line from template and arguments
    std::string buildCommandFromTemplate(const std::string& command_template, const json& arguments, const std::string& executable = "");

    // Validate tool definition JSON schema
    bool validateToolDefinition(const json& tool_definition, std::string& error_message) const;
};
