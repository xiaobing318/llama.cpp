#include "qcopilot-builtin-tools.h"
#include "qcopilot-utils.h"
#include "tools-registry.h"

namespace BuiltinTools {

// 获取内置工具的定义
std::vector<BuiltinTools::Types::ToolDefinition> getBuiltinToolDefinitions() {
    std::vector<BuiltinTools::Types::ToolDefinition> definitions;
    const auto& registry = getToolRegistry();
    definitions.reserve(registry.size());
    for (const auto& entry : registry) {
        definitions.push_back(entry.definition());
    }
    return definitions;
}

// 获取内置工具的执行器函数映射
std::map<std::string, BuiltinTools::Types::ToolFunction> getBuiltinToolFunctions() {
    std::map<std::string, BuiltinTools::Types::ToolFunction> functions;
    for (const auto& entry : getToolRegistry()) {
        functions[entry.name] = entry.runner;
    }
    return functions;
}

} // namespace BuiltinTools
