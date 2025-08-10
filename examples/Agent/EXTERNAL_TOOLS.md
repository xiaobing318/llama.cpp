# 外部工具使用指南

## 概述

Agent 系统支持两种类型的工具：
1. **内置工具**：用 C++ 实现，直接在 Agent 进程中执行
2. **外部工具**：通过系统调用启动外部可执行文件

## 外部工具定义格式

在配置文件 `config.json` 中，外部工具需要添加 `executable` 字段：

```json
{
  "type": "function",
  "executable": "python3 ./external_tools/my_tool.py",
  "function": {
    "name": "tool_name",
    "description": "工具描述",
    "parameters": {
      "type": "object",
      "properties": {
        "param1": {
          "type": "string",
          "description": "参数1描述"
        }
      },
      "required": ["param1"]
    }
  }
}
```

### 关键字段说明：
- `executable`: 外部工具的可执行命令
- `function`: 标准的 OpenAI Function Calling 格式定义

## 外部工具协议

### 输入协议
外部工具从**标准输入**接收 JSON 格式的参数：
```json
{
  "param1": "value1",
  "param2": "value2"
}
```

### 输出协议
外部工具向**标准输出**输出 JSON 格式的结果：

**成功情况：**
```json
{
  "result": "处理结果",
  "success": true,
  "additional_data": "其他数据"
}
```

**失败情况：**
```json
{
  "error": "错误信息",
  "success": false
}
```

如果输出不是有效的 JSON，Agent 会将其作为纯文本处理：
```json
{
  "output": "原始输出内容",
  "success": true
}
```

## 外部工具示例

### Python 工具示例

```python
#!/usr/bin/env python3
import sys
import json

def main():
    try:
        # 读取输入参数
        input_data = sys.stdin.read()
        args = json.loads(input_data)
        
        # 处理逻辑
        result = process_data(args)
        
        # 输出结果
        response = {
            "result": result,
            "success": True
        }
        print(json.dumps(response))
        
    except Exception as e:
        print(json.dumps({
            "error": str(e),
            "success": False
        }))

if __name__ == "__main__":
    main()
```

### Shell 脚本示例

```bash
#!/bin/bash
# 读取 JSON 输入
input=$(cat)

# 解析参数（需要 jq 工具）
name=$(echo "$input" | jq -r '.name // "World"')

# 处理逻辑
result="Hello, $name!"

# 输出 JSON 结果
echo "{\"result\": \"$result\", \"success\": true}"
```

## 错误处理

外部工具的退出码：
- `0`: 成功执行
- `非0`: 执行失败，Agent 会报告错误

## 安全注意事项

1. **路径验证**：确保外部工具路径安全
2. **参数校验**：验证输入参数防止注入攻击
3. **权限控制**：外部工具应以最小权限运行
4. **资源限制**：考虑添加超时和资源使用限制

## 调试外部工具

可以手动测试外部工具：
```bash
echo '{"name": "Test"}' | python3 ./external_tools/simple_tool.py
```

## 示例文件

- `test_external_tools.json`: 测试配置文件
- `external_tools/simple_tool.py`: 简单 Python 工具示例
- `config_with_external_tools.json`: 完整配置示例