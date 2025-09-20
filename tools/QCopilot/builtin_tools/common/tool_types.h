#pragma once

#include "json.hpp"
#include <string>
#include <functional>

using json = nlohmann::ordered_json;

namespace builtin_tools {

// 内置工具定义结构
struct ToolDefinition {
    std::string name;
    json definition;
};

// 内置工具执行器函数类型
using ToolFunction = std::function<json(const json&)>;

} // namespace builtin_tools

// 向后兼容旧命名空间，后续版本将清理
namespace BuiltinTools {
using ToolDefinition = ::builtin_tools::ToolDefinition;
using ToolFunction = ::builtin_tools::ToolFunction;
} // namespace BuiltinTools
