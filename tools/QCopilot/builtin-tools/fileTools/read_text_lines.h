#pragma once

#include "../common/tool_types.h"

namespace BuiltinTools {
namespace FileTools {

// 获取 read_text_lines 工具的定义
ToolDefinition get_read_text_lines_definition();

// 执行 read_text_lines 工具
json run_read_text_lines(const json& args);

} // namespace FileTools
} // namespace BuiltinTools
