#pragma once

#include "../common/tool_types.h"

namespace BuiltinTools {
namespace FileTools {

// 获取validate_utf8_file工具的定义
ToolDefinition getValidateUtf8FileDefinition();

// 执行validate_utf8_file工具
json executeValidateUtf8File(const json& args);

} // namespace FileTools
} // namespace BuiltinTools