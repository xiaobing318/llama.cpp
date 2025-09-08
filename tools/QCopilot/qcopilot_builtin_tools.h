#pragma once

#include <functional>
#include <map>
#include <vector>
#include <string>
#include "json.hpp"

// 包含工具类型定义、内置工具头文件
#include "builtinTools/common/tool_types.h"
#include "builtinTools/timeTools/get_current_time.h"
#include "builtinTools/mathTools/calculate.h"
#include "builtinTools/fileTools/read_text_file.h"
#include "builtinTools/fileTools/write_text_file.h"
#include "builtinTools/fileTools/validate_utf8_file.h"
#include "builtinTools/systemTools/list_directory.h"
#include "builtinTools/systemTools/path_stat.h"
#include "builtinTools/systemTools/grep.h"
#include "builtinTools/systemTools/glob.h"

using json = nlohmann::ordered_json;

// 前向声明
class ToolExecutor;

namespace BuiltinTools {

// 获取所有内置工具的定义
std::vector<ToolDefinition> getBuiltinToolDefinitions();

// 获取内置工具的执行器函数映射
std::map<std::string, ToolFunction> getBuiltinToolFunctions();

} // namespace BuiltinTools
