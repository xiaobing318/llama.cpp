#pragma once

#include "../common/tool_types.h"

namespace BuiltinTools {
namespace FileTools {

// 获取read_text_file工具的定义
ToolDefinition getReadTextFileDefinition();

// 执行read_text_file工具
json executeReadTextFile(const json& args);

} // namespace FileTools
} // namespace BuiltinTools