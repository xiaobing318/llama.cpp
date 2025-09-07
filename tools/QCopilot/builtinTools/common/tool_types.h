#pragma once

#include "json.hpp"
#include <string>
#include <functional>

using json = nlohmann::ordered_json;

namespace BuiltinTools {

// 内置工具定义结构
struct ToolDefinition {
    std::string name;
    json definition;
};

// 内置工具执行器函数类型
using ToolFunction = std::function<json(const json&)>;

} // namespace BuiltinTools