#pragma once

#include "../common/tool_types.h"

namespace BuiltinTools {
namespace FileTools {

// 获取write_text_file工具的定义
ToolDefinition get_write_text_file_definition();

// 执行write_text_file工具
json run_write_text_file(const json& args);

} // namespace FileTools
} // namespace BuiltinTools
