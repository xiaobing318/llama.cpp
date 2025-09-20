# 内置工具开发约定

本文档规定了 `tools/QCopilot/builtin_tools` 目录的代码规范与扩展流程，帮助新旧工具保持一致性、易读性与跨平台行为。

## 命名与结构

- **目录与文件名**：统一使用 `snake_case`。示例：`fileTools/write_text_file.{h,cpp}`。
- **命名空间**：
  - 公共 API 位于 `builtin_tools::common`。
  - 内部实现细节放在 `builtin_tools::internal`（仅供工具内或单测引用）。
- **工具函数签名**：
  - 定义函数：`ToolDefinition get_<tool_name>_definition();`
  - 执行函数：`json run_<tool_name>(const json& args);`
- **通用响应**：统一使用 `builtin_tools::common::make_success` / `make_error`，可选搭配 `append_message`、`set_truncated` 管理附加信息。

## 注释与日志

- 注释优先使用中文，单行 `//`，多行 `/** ... */`，说明“为什么”与关键约束。
- 日志统一通过 `LOG_INF` / `LOG_WRN` / `LOG_ERR`，使用简短英文描述 + 关键变量，便于跨平台检索。
- 错误提示面向终端用户，保持清晰且不泄漏内部实现细节。

## 公共辅助库

`builtin_tools/common` 按职能拆分：

| 模块 | 功能概述 |
| --- | --- |
| `filesystem_utils` | 路径校验、UTF-8 ↔ path 转换、目录枚举、glob |
| `io_utils` | 文件读写、二进制探测、正则搜索 |
| `encoding_utils` | UTF-8 校验 |
| `string_utils` | 字符串清洗、拼接、格式化 |
| `time_utils` | 时间获取与格式化 |
| `json_utils` | JSON 解析/格式化 |
| `tool_response` | 成功/失败响应模板 |
| `common_utils_internal` | 内部复用逻辑（如 `read_text_with_range`） |

公共 API 在 `builtin_tools::common` 暴露，内部接口通过 `builtin_tools::internal` 对应测试验证。

## 添加新工具流程

1. **选择目录**：根据职能放在 `fileTools` / `mathTools` / `systemTools` / `timeTools` 等子目录。
2. **实现**：
   - 新建 `<tool>.h/.cpp`，提供 `get_<tool>_definition` 与 `run_<tool>`。
   - 充分利用 `builtin_tools::common` 中的公共函数，避免重复造轮子。
   - 错误路径使用 `return make_error(tool_name, message);`，成功时基于 `make_success` 并追加业务字段。
3. **注册**：在 `tool_registry.cpp` 中追加一行：

   ```cpp
   {"tool_name", Namespace::get_tool_name_definition, Namespace::run_tool_name},
   ```

   `tool_registry` 会被 `qcopilot_builtin_tools.cpp` 自动消费。
4. **单元测试**：
   - 在 `tests/` 中新增 `test_<domain>_<tool>.cpp`，覆盖路径/编码/异常分支。
   - 如需临时目录或断言工具，复用 `test_support.h`。
5. **文档**：
   - 更新本 README 或工具说明，记录输入输出字段、特定注意事项。

## 常见约束

- 文件系统操作需经过 `validate_existing_path`（读）或 `prepare_writable_path`（写），防止路径穿越与非法字符。
- 所有 UTF-8 相关操作依赖 `encoding_utils`，选用 `is_valid_utf8_string` / `is_valid_utf8_file` 保障跨平台一致性。
- 大文件读取需设置上限（默认 100MB），并在响应中提示被截断的原因。
- 目录与 glob 默认不包含隐藏项，需显式 `show_hidden=true` 才展示。
- 在工具层处理完安全约束、日志与参数校正后，再将原始结果传递给上层。

## 扩展建议

- 新增公共函数前请确认是否已有实现；如确有需要，按职责拆分放入对应 `*_utils` 模块。
- 若公共函数较易被误用，可放入 `internal` 命名空间并在单测中覆盖，工具层提供安全包装。
- 鼓励在工具返回中补充 `messages` 数组，用于提示自动纠正、隐藏项行为、限制说明等。
- 所有新增 API 均需在测试中覆盖非 ASCII、中文路径、Windows/Linux/macOS 差异场景。

遵循以上约定可以确保 QCopilot 内置工具的稳定性、可维护性与统一的用户体验。
