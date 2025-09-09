#pragma once

#include "../common/tool_types.h"

namespace BuiltinTools {
namespace FileTools {

// 获取 read_text_lines 工具的定义
ToolDefinition getReadTextLinesDefinition();

// 执行 read_text_lines 工具
json executeReadTextLines(const json& args);

} // namespace FileTools
} // namespace BuiltinTools
