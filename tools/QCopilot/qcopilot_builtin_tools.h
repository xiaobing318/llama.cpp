#pragma once

#include <functional>
#include <map>
#include <vector>
#include <string>
#include "json.hpp"
#include "qcopilot_utils.h"

using json = nlohmann::ordered_json;

// 前向声明
class ToolExecutor;

namespace BuiltinTools {

// 内置工具定义结构
struct ToolDefinition {
    std::string name;
    json definition;
};

// 内置工具执行器函数类型
using ToolFunction = std::function<json(const json&)>;

// 获取所有内置工具的定义
std::vector<ToolDefinition> getBuiltinToolDefinitions();

// 获取内置工具的执行器函数映射
std::map<std::string, ToolFunction> getBuiltinToolFunctions();

// 辅助函数
bool matchPattern(const std::string& text, const std::string& pattern);
std::vector<json> searchInFile(
    const std::string& filepath,
    const std::string& pattern, 
    bool case_sensitive,
    bool line_numbers);

// 各个内置工具的具体实现函数，静态函数，完全独立
json executeGetCurrentTime(const json& args);
json executeCalculate(const json& args);
json executeReadFile(const json& args);
json executeWriteFile(const json& args);
json executeGlob(const json& args);
json executeGrep(const json& args);
json executeMultiEdit(const json& args);
json executeEdit(const json& args);
json executeBash(const json& args);
json executeListDirectory(const json& args);
json executeFileStats(const json& args);
} // namespace BuiltinTools
