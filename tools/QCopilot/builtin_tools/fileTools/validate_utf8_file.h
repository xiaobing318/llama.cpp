#pragma once

#include "../common/tool_types.h"

namespace BuiltinTools {
namespace FileTools {

// 获取validate_utf8_file工具的定义
ToolDefinition get_validate_utf8_file_definition();

// 执行validate_utf8_file工具
json run_validate_utf8_file(const json& args);

} // namespace FileTools
} // namespace BuiltinTools