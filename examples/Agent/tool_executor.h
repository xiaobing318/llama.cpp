#pragma once

#include <string>
#include <functional>
#include <unordered_map>
#include "json.hpp"

using json = nlohmann::ordered_json;

class ToolExecutor {
public:
    using ToolFunction = std::function<json(const json&)>;

    ToolExecutor();
    ~ToolExecutor() = default;

    // Register a tool with its implementation
    void registerExternalTools(const json& tool_definition);

    // Execute a tool by name with arguments
    json execute(const std::string& name, const json& arguments);

    // Check if a tool exists
    bool hasTool(const std::string & name) const;

    // Get all registered tools
    json getTools() const;

private:
    std::unordered_map<std::string, ToolFunction> tools;
    std::unordered_map<std::string, json> tool_definitions;

    // Built-in tool implementations
    json executeGetCurrentTime(const json& args);
    json executeCalculate(const json& args);
    json executeReadFile(const json& args);
    json executeWriteFile(const json& args);
    json executeListFiles(const json& args);

    // Register built-in tools
    void registerBuiltinTools();
    
    // Execute external tool (backward compatibility)
    json executeExternalTool(const std::string& executable, const json& arguments);
    
    // Execute external tool with command template
    json executeExternalTool(const std::string& executable, const json& arguments, const std::string& command_template);
    
    // Build command line from template and arguments
    std::string buildCommandFromTemplate(const std::string& command_template, const json& arguments);
};
