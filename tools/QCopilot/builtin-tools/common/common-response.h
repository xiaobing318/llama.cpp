/**
 * @file common-response.h
 * @brief 文件介绍：提供构建内置工具响应的辅助接口。
 *
 * 文件概述：本文件集中声明了若干用于生成与维护内置工具响应 JSON 的接口，
 * 借助这些接口，调用方可以轻松构造出统一格式的成功或失败响应，并在执行过程中
 * 动态追加日志消息或标记结果是否被截断。
 *
 * 使用方式：
 * - 在需要创建标准化工具响应结构时，包含本头文件即可调用这些接口。
 * - 使用 `BuiltinTools::Common::make_success` / `BuiltinTools::Common::make_error` 生成初始响应骨架。
 * - 借助 `BuiltinTools::Common::append_message` 与 `BuiltinTools::Common::set_truncated` 在执行期间补充上下文信息。
 *
 * 使用场景：
 * - 在内置工具的运行流程中统一返回 JSON 响应，便于前后端解析。
 * - 为调用者提供结构化的执行日志与错误说明。
 * - 在输出可能过长时显式标记截断状态，方便上游组件做出处理决策。
 */

#pragma once

#include "json.hpp"

#include <string>
#include <vector>

namespace BuiltinTools {
namespace Common {

using json = nlohmann::ordered_json;

/**
 * @brief 生成成功状态的工具响应。
 *
 * 构造一个包含 `tool`、`success`、`messages` 与 `truncated` 字段的 JSON 对象，
 * 其中 `success` 被设置为 `true`，消息数组默认置空，截断标记设为 `false`。
 *
 * @param tool_name 工具名称，用于标识响应来源。
 * @return 标准化的成功响应 JSON，供调用者直接返回或进一步补充信息。
 */
json make_success(const std::string& tool_name);

/**
 * @brief 生成失败状态的工具响应并附带初始错误信息。
 *
 * 在基础响应模板的基础上，将 `success` 置为 `false`，并将错误信息写入
 * `error` 字段，同时附加到 `messages` 数组中，便于调用方获取第一条错误上下文。
 *
 * @param tool_name 工具名称，用于标识响应来源。
 * @param message 首条错误提示信息，描述失败原因。
 * @return 标准化的失败响应 JSON，可继续追加更多诊断信息。
 */
json make_error(const std::string& tool_name, const std::string& message);

/**
 * @brief 向响应对象追加可读消息。
 *
 * 确保响应中存在 `messages` 数组，若缺失则以空数组初始化，然后将新消息
 * 追加到数组尾部，为调用者提供更多执行细节或提示。
 *
 * @param response 待更新的响应 JSON，对其进行原地修改。
 * @param message 需要追加的文本消息。
 */
void append_message(json& response, const std::string& message);

/**
 * @brief 更新响应的截断状态标记。
 *
 * 将 `truncated` 字段设置为对应的布尔值，用于告知调用者当前响应内容是否
 * 因长度或其他限制而被截断。
 *
 * @param response 待更新的响应 JSON，对其进行原地修改。
 * @param truncated 截断标记，`true` 表示内容被截断。
 */
void set_truncated(json& response, bool truncated);

} // namespace Common
} // namespace BuiltinTools

