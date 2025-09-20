#pragma once

#include <functional>
#include <map>
#include <vector>
#include <string>
#include "json.hpp"

// 包含工具类型定义、内置工具头文件
#include "builtin_tools/common/tool_types.h"
#include "builtin_tools/timeTools/get_current_time.h"
#include "builtin_tools/mathTools/calculate.h"
#include "builtin_tools/fileTools/read_text_lines.h"
#include "builtin_tools/fileTools/write_text_file.h"
#include "builtin_tools/fileTools/validate_utf8_file.h"
#include "builtin_tools/systemTools/list_directory.h"
#include "builtin_tools/systemTools/path_stat.h"
#include "builtin_tools/systemTools/grep.h"
#include "builtin_tools/systemTools/glob.h"

using json = nlohmann::ordered_json;

// 前向声明
class ToolExecutor;

namespace BuiltinTools {

// 获取所有内置工具的定义
std::vector<ToolDefinition> getBuiltinToolDefinitions();

// 获取内置工具的执行器函数映射
std::map<std::string, ToolFunction> getBuiltinToolFunctions();

} // namespace BuiltinTools
