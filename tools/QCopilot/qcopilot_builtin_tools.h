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

// 使用 Regex 模式在文件中匹配
std::vector<json> searchInFileRegex(
    const std::string& filepath,
    const std::string& pattern,
    bool use_regex,
    bool case_sensitive,
    bool line_numbers,
    int& total_matches,
    int max_matches);

// 各个内置工具的具体实现函数，静态函数，完全独立
json executeGetCurrentTime(const json& args);
json executeCalculate(const json& args);
json executeReadTextFile(const json& args);
json executeWriteTextFile(const json& args);
json executeGlob(const json& args);
json executeGrep(const json& args);
json executeEdit(const json& args);
json executeListDirectory(const json& args);
json executeInspectPath(const json& args);
json executeCheckUtf8Encoding(const json& args);
} // namespace BuiltinTools
