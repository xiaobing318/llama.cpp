#pragma once

#include "../common/tool_types.h"

namespace BuiltinTools {
namespace SystemTools {

// 获取list_directory工具的定义
ToolDefinition get_list_directory_definition();

// 执行list_directory工具
json run_list_directory(const json& args);

} // namespace SystemTools
} // namespace BuiltinTools