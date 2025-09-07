#pragma once

#include "../common/tool_types.h"

namespace BuiltinTools {
namespace FileTools {

// 获取write_text_file工具的定义
ToolDefinition getWriteTextFileDefinition();

// 执行write_text_file工具
json executeWriteTextFile(const json& args);

} // namespace FileTools
} // namespace BuiltinTools
