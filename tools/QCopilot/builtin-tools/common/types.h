/**
 * @file types.h
 * @brief 文件介绍：提供内置工具调用相关的类型定义。
 *
 * 文件概述：本文件的作用是为内置工具调用定义通用的数据结构与类型别名，便于开发者在不同模块中统一管理和调用工具逻辑。它为 JSON 数据交互、
 * 工具函数封装以及工具定义描述提供了基础类型。
 *
 * 使用方式：
 * - 在需要声明或实现内置工具时，包含本头文件即可使用其中的类型。
 * - 使用 `BuiltinTools::Types::ToolFunction` 封装工具的执行逻辑，确保所有工具都具备一致的输入输出签名：输入为 JSON，输出也为 JSON。
 * - 使用 `BuiltinTools::Types::ToolDefinition` 定义工具的元信息（名称与定义），便于在工具注册与调用框架中统一管理。
 *
 * 使用场景：
 * - 在实现新的内置工具时，通过 `ToolFunction` 定义工具的执行逻辑，并结合 `ToolDefinition` 进行注册。
 * - 在构建工具调用系统时，统一使用本文件中的类型来组织工具描述与结果，以便 LLM、前端或其他系统模块进行调用与交互。
 * - 在需要扩展工具集或进行跨平台调用时，作为工具接口层，保证输入输出格式一致。
 */

#pragma once

#include "json.hpp"
#include <string>
#include <functional>

// 设置 JSON 类型别名，这里使用有序 JSON 以保证键值对顺序
using json = nlohmann::ordered_json;

namespace BuiltinTools {
namespace Types{

// 设置内置工具执行器函数类型别名
using ToolFunction = std::function<json(const json&)>;

// 内置工具调用定义结构
struct ToolDefinition {
    std::string name;
    json definition;
};

} // namespace Types
} // namespace BuiltinTools
