#!/usr/bin/env python3
"""
示例外部工具 - 展示如何与 Agent 系统交互
从标准输入读取 JSON 参数，处理后输出 JSON 结果
"""
import sys
import json

def main():
    try:
        # 从标准输入读取 JSON 参数
        input_data = sys.stdin.read()
        if not input_data.strip():
            print(json.dumps({
                "error": "No input data provided",
                "success": False
            }))
            return
        
        # 解析 JSON 参数
        args = json.loads(input_data)
        
        # 执行工具逻辑
        name = args.get("name", "World")
        message = args.get("message", "Hello")
        
        # 模拟一些处理
        result = f"{message}, {name}! This is from external tool."
        
        # 输出 JSON 结果
        response = {
            "result": result,
            "input_name": name,
            "input_message": message,
            "success": True
        }
        
        print(json.dumps(response))
        
    except json.JSONDecodeError as e:
        print(json.dumps({
            "error": f"Invalid JSON input: {str(e)}",
            "success": False
        }))
    except Exception as e:
        print(json.dumps({
            "error": f"Tool execution error: {str(e)}",
            "success": False
        }))

if __name__ == "__main__":
    main()