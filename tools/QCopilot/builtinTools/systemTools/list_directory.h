#pragma once

#include "../common/tool_types.h"

namespace BuiltinTools {
namespace SystemTools {

// 获取list_directory工具的定义
ToolDefinition getListDirectoryDefinition();

// 执行list_directory工具
json executeListDirectory(const json& args);

} // namespace SystemTools
} // namespace BuiltinTools