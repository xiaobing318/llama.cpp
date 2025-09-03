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
std::map<std::string, ToolFunction> getBuiltinToolFunctions(ToolExecutor* executor);

/*各个内置工具的具体实现函数（这些函数需要传入ToolExecutor指针来访问其方法）*/

// 基础工具
json executeGetCurrentTime(ToolExecutor* executor, const json& args);
json executeCalculate(ToolExecutor* executor, const json& args);
json executeReadFile(ToolExecutor* executor, const json& args);
json executeWriteFile(ToolExecutor* executor, const json& args);

// Claude Code风格工具
json executeGlob(ToolExecutor* executor, const json& args);
json executeGrep(ToolExecutor* executor, const json& args);
json executeMultiEdit(ToolExecutor* executor, const json& args);
json executeEdit(ToolExecutor* executor, const json& args);
json executeBash(ToolExecutor* executor, const json& args);
json executeListDirectory(ToolExecutor* executor, const json& args);
json executeFileStats(ToolExecutor* executor, const json& args);

// 辅助函数
bool matchPattern(const std::string& text, const std::string& pattern);
std::vector<json> searchInFile(const std::string& filepath, const std::string& pattern, 
                              bool case_sensitive, bool line_numbers);

} // namespace BuiltinTools
