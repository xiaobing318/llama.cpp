# 测试一（非推理模型）

 - 测试平台：reqbin.com
 - 测试组合：非推理模型+非工具调用+非流式返回

## 测试请求（HTTP Message）
```text plain
POST /v1/chat/completions HTTP/1.1
Accept: */*
Accept-Encoding: deflate, gzip
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/139.0.0.0 Safari/537.36
Host: localhost
Content-Type: application/json
Content-Length: 772

{
    "model": "qwen2.5-1.5b-instruct-q8_0",
    "messages": [
        {
            "role": "system",
            "content": "You are a QGIS Copilot, specifically assisting me in processing geospatial data. You can use the given external function tools and need to follow the user's instructions. For example, in some cases the user's intention is to want you to answer questions rather than call tools, and in other cases the user's intention is to want you to call tools. Therefore, you need to carefully distinguish the user's intention, answer questions when it is necessary, and refer to the given tool definition to call tools when it is necessary."
        },
        {
            "role": "user",
            "content": "hello,what's your name?"
        }
    ]
}
```

## 测试响应（HTTP Message）
```text plain
HTTP/1.1 200 OK
access-control-allow-origin: chrome-extension://gmmkjpcadciiokjpikmkkmapphbmdjok
content-length: 710
content-type: application/json; charset=utf-8
keep-alive: timeout=5, max=100
server: llama.cpp

{"choices":[{"finish_reason":"stop","index":0,"message":{"role":"assistant","content":"Hello! My name is QGIS Copilot, a tool designed to assist with geospatial data processing tasks. How can I help you today?"}}],"created":1756107340,"model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion","usage":{"completion_tokens":31,"prompt_tokens":127,"total_tokens":158},"id":"chatcmpl-Ujv9cnL55QyGcrL62VTeLbC6YC70JMLq","timings":{"prompt_n":127,"prompt_ms":245.305,"prompt_per_token_ms":1.9315354330708663,"prompt_per_second":517.7228348382625,"predicted_n":31,"predicted_ms":314.655,"predicted_per_token_ms":10.150161290322579,"predicted_per_second":98.52060192909696}}
```

# 测试二（非推理模型）

 - 测试平台：reqbin.com
 - 测试组合：非推理模型+非工具调用+流式返回

## 测试请求（HTTP Message）
```text plain
POST /v1/chat/completions HTTP/1.1
Accept: */*
Accept-Encoding: deflate, gzip
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/139.0.0.0 Safari/537.36
Host: localhost
Content-Type: application/json
Content-Length: 789

{
    "model": "qwen2.5-1.5b-instruct-q8_0",
    "messages": [
        {
            "role": "system",
            "content": "You are a QGIS Copilot, specifically assisting me in processing geospatial data. You can use the given external function tools and need to follow the user's instructions. For example, in some cases the user's intention is to want you to answer questions rather than call tools, and in other cases the user's intention is to want you to call tools. Therefore, you need to carefully distinguish the user's intention, answer questions when it is necessary, and refer to the given tool definition to call tools when it is necessary."
        },
        {
            "role": "user",
            "content": "hello,what's your name?"
        }
    ],
  "stream":true
}
```

## 测试响应（HTTP Message）
```text plain
HTTP/1.1 200 OK
access-control-allow-origin: chrome-extension://gmmkjpcadciiokjpikmkkmapphbmdjok
content-type: text/event-stream
keep-alive: timeout=5, max=100
server: llama.cpp
transfer-encoding: chunked

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"role":"assistant","content":null}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":"Hello"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":"!"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" It"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":"'s"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" nice"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" to"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" meet"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" you"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":"."}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" I"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":"'m"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" Q"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":"GIS"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" Cop"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":"ilot"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":","}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" a"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" tool"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" designed"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" to"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" assist"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" with"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" processing"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" ge"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":"os"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":"patial"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" data"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":"."}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" How"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" can"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" I"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" help"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" you"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" today"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":"?"}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":"stop","index":0,"delta":{}}],"created":1756109773,"id":"chatcmpl-lV5ZtenAqk8ZNBSiw2FUjBch4LIGzI94","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk","usage":{"completion_tokens":36,"prompt_tokens":127,"total_tokens":163},"timings":{"prompt_n":1,"prompt_ms":197.865,"prompt_per_token_ms":197.865,"prompt_per_second":5.0539509261365065,"predicted_n":36,"predicted_ms":386.529,"predicted_per_token_ms":10.736916666666666,"predicted_per_second":93.13660811995996}}

data: [DONE]
```

# 测试三（非推理模型）

 - 测试平台：reqbin.com
 - 测试组合：非推理模型+工具调用+非流式返回

## 测试请求（HTTP Message）
```text plain
POST /v1/chat/completions HTTP/1.1
Accept: */*
Accept-Encoding: deflate, gzip
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/139.0.0.0 Safari/537.36
Host: localhost
Content-Type: application/json
Content-Length: 1336

{
    "model": "qwen2.5-1.5b-instruct-q8_0",
    "tools": [
      {
        "type": "function",
        "function": {
          "name": "python",
          "description": "Runs code in an ipython interpreter and returns the result of the execution after 60 seconds.",
          "parameters": {
            "type": "object",
            "properties": {
              "code": {
                "type": "string",
                "description": "The code to run in the ipython interpreter."
              }
            },
            "required": ["code"]
          }
        }
      }
    ],
    "messages": [
        {
            "role": "system",
            "content": "You are a QGIS Copilot, specifically assisting me in processing geospatial data. You can use the given external function tools and need to follow the user's instructions. For example, in some cases the user's intention is to want you to answer questions rather than call tools, and in other cases the user's intention is to want you to call tools. Therefore, you need to carefully distinguish the user's intention, answer questions when it is necessary, and refer to the given tool definition to call tools when it is necessary."
        },
        {
            "role": "user",
            "content": "i want to print hello world by using python."
        }
    ]
}
```

## 测试响应（HTTP Message）
```text plain
HTTP/1.1 200 OK
access-control-allow-origin: chrome-extension://gmmkjpcadciiokjpikmkkmapphbmdjok
content-length: 749
content-type: application/json; charset=utf-8
keep-alive: timeout=5, max=100
server: llama.cpp

{"choices":[{"finish_reason":"tool_calls","index":0,"message":{"role":"assistant","content":null,"tool_calls":[{"type":"function","function":{"name":"python","arguments":"{\"code\":\"print('Hello World')\"}"},"id":"bW94lBfswXMbsEaCEVnz1uatue8MkxlT"}]}}],"created":1756112066,"model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion","usage":{"completion_tokens":23,"prompt_tokens":292,"total_tokens":315},"id":"chatcmpl-vT6SEjnvPPqxYVevXf7tgFuluHmBNuPb","timings":{"prompt_n":183,"prompt_ms":45.802,"prompt_per_token_ms":0.25028415300546447,"prompt_per_second":3995.458713593293,"predicted_n":23,"predicted_ms":234.623,"predicted_per_token_ms":10.200999999999999,"predicted_per_second":98.0296049406921}}
```
```json
{
    "choices": [
        {
            "finish_reason": "tool_calls",
            "index": 0,
            "message": {
                "role": "assistant",
                "content": null,
                "tool_calls": [
                    {
                        "type": "function",
                        "function": {
                            "name": "python",
                            "arguments": "{\"code\":\"print('Hello World')\"}"
                        },
                        "id": "bW94lBfswXMbsEaCEVnz1uatue8MkxlT"
                    }
                ]
            }
        }
    ],
    "created": 1756112066,
    "model": "qwen2.5-1.5b-instruct-q8_0",
    "system_fingerprint": "b6224-12ff9e28a",
    "object": "chat.completion",
    "usage": {
        "completion_tokens": 23,
        "prompt_tokens": 292,
        "total_tokens": 315
    },
    "id": "chatcmpl-vT6SEjnvPPqxYVevXf7tgFuluHmBNuPb",
    "timings": {
        "prompt_n": 183,
        "prompt_ms": 45.802,
        "prompt_per_token_ms": 0.25028415300546447,
        "prompt_per_second": 3995.458713593293,
        "predicted_n": 23,
        "predicted_ms": 234.623,
        "predicted_per_token_ms": 10.200999999999999,
        "predicted_per_second": 98.0296049406921
    }
}
```

# 测试四（非推理模型）

 - 测试平台：reqbin.com
 - 测试组合：非推理模型+工具调用+流式返回

## 测试请求（HTTP Message）
```text plain
POST /v1/chat/completions HTTP/1.1
Accept: */*
Accept-Encoding: deflate, gzip
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/139.0.0.0 Safari/537.36
Host: localhost
Content-Type: application/json
Content-Length: 1353

{
    "model": "qwen2.5-1.5b-instruct-q8_0",
    "tools": [
      {
        "type": "function",
        "function": {
          "name": "python",
          "description": "Runs code in an ipython interpreter and returns the result of the execution after 60 seconds.",
          "parameters": {
            "type": "object",
            "properties": {
              "code": {
                "type": "string",
                "description": "The code to run in the ipython interpreter."
              }
            },
            "required": ["code"]
          }
        }
      }
    ],
    "messages": [
        {
            "role": "system",
            "content": "You are a QGIS Copilot, specifically assisting me in processing geospatial data. You can use the given external function tools and need to follow the user's instructions. For example, in some cases the user's intention is to want you to answer questions rather than call tools, and in other cases the user's intention is to want you to call tools. Therefore, you need to carefully distinguish the user's intention, answer questions when it is necessary, and refer to the given tool definition to call tools when it is necessary."
        },
        {
            "role": "user",
            "content": "i want to print hello world by using python."
        }
    ],
  "stream":true
}
```

## 测试响应（HTTP Message）
```text plain
HTTP/1.1 200 OK
access-control-allow-origin: chrome-extension://gmmkjpcadciiokjpikmkkmapphbmdjok
content-type: text/event-stream
keep-alive: timeout=5, max=100
server: llama.cpp
transfer-encoding: chunked

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"role":"assistant","content":null}}],"created":1756112496,"id":"chatcmpl-tUi7npLtiUSucaPFXHLxosPHnd4oHaEL","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"tool_calls":[{"index":0,"id":"ZJ2xV7o3tT5wzSrRLrqRnsFF9fATW899","type":"function","function":{"name":"python","arguments":""}}]}}],"created":1756112496,"id":"chatcmpl-tUi7npLtiUSucaPFXHLxosPHnd4oHaEL","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"tool_calls":[{"index":0,"function":{"arguments":"{\""}}]}}],"created":1756112497,"id":"chatcmpl-tUi7npLtiUSucaPFXHLxosPHnd4oHaEL","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"tool_calls":[{"index":0,"function":{"arguments":"code"}}]}}],"created":1756112497,"id":"chatcmpl-tUi7npLtiUSucaPFXHLxosPHnd4oHaEL","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"tool_calls":[{"index":0,"function":{"arguments":"\":"}}]}}],"created":1756112497,"id":"chatcmpl-tUi7npLtiUSucaPFXHLxosPHnd4oHaEL","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"tool_calls":[{"index":0,"function":{"arguments":"\""}}]}}],"created":1756112497,"id":"chatcmpl-tUi7npLtiUSucaPFXHLxosPHnd4oHaEL","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"tool_calls":[{"index":0,"function":{"arguments":"print"}}]}}],"created":1756112497,"id":"chatcmpl-tUi7npLtiUSucaPFXHLxosPHnd4oHaEL","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"tool_calls":[{"index":0,"function":{"arguments":"('"}}]}}],"created":1756112497,"id":"chatcmpl-tUi7npLtiUSucaPFXHLxosPHnd4oHaEL","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"tool_calls":[{"index":0,"function":{"arguments":"Hello"}}]}}],"created":1756112497,"id":"chatcmpl-tUi7npLtiUSucaPFXHLxosPHnd4oHaEL","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"tool_calls":[{"index":0,"function":{"arguments":","}}]}}],"created":1756112497,"id":"chatcmpl-tUi7npLtiUSucaPFXHLxosPHnd4oHaEL","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"tool_calls":[{"index":0,"function":{"arguments":" World"}}]}}],"created":1756112497,"id":"chatcmpl-tUi7npLtiUSucaPFXHLxosPHnd4oHaEL","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"tool_calls":[{"index":0,"function":{"arguments":"!"}}]}}],"created":1756112497,"id":"chatcmpl-tUi7npLtiUSucaPFXHLxosPHnd4oHaEL","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"tool_calls":[{"index":0,"function":{"arguments":"')"}}]}}],"created":1756112497,"id":"chatcmpl-tUi7npLtiUSucaPFXHLxosPHnd4oHaEL","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"tool_calls":[{"index":0,"function":{"arguments":"\"}"}}]}}],"created":1756112497,"id":"chatcmpl-tUi7npLtiUSucaPFXHLxosPHnd4oHaEL","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":"tool_calls","index":0,"delta":{}}],"created":1756112504,"id":"chatcmpl-tUi7npLtiUSucaPFXHLxosPHnd4oHaEL","model":"qwen2.5-1.5b-instruct-q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk","usage":{"completion_tokens":25,"prompt_tokens":292,"total_tokens":317},"timings":{"prompt_n":1,"prompt_ms":129.526,"prompt_per_token_ms":129.526,"prompt_per_second":7.720457668730602,"predicted_n":25,"predicted_ms":511.755,"predicted_per_token_ms":20.4702,"predicted_per_second":48.85150120663208}}

data: [DONE]
```









































# 测试一（推理模型）

 - 测试平台：reqbin.com
 - 测试组合：推理模型+非工具调用+非流式返回

## 测试请求（HTTP Message）
```text plain
POST /v1/chat/completions HTTP/1.1
Accept: */*
Accept-Encoding: deflate, gzip
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/139.0.0.0 Safari/537.36
Host: localhost
Content-Type: application/json
Content-Length: 762

{
    "model": "gpt-oss-20b-Q8_0",
    "messages": [
        {
            "role": "system",
            "content": "You are a QGIS Copilot, specifically assisting me in processing geospatial data. You can use the given external function tools and need to follow the user's instructions. For example, in some cases the user's intention is to want you to answer questions rather than call tools, and in other cases the user's intention is to want you to call tools. Therefore, you need to carefully distinguish the user's intention, answer questions when it is necessary, and refer to the given tool definition to call tools when it is necessary."
        },
        {
            "role": "user",
            "content": "hello,what's your name?"
        }
    ]
}
```

## 测试响应（HTTP Message）
```text plain
HTTP/1.1 200 OK
access-control-allow-origin: chrome-extension://gmmkjpcadciiokjpikmkkmapphbmdjok
content-length: 821
content-type: application/json; charset=utf-8
keep-alive: timeout=5, max=100
server: llama.cpp

{"choices":[{"finish_reason":"stop","index":0,"message":{"role":"assistant","reasoning_content":"The user just says hello and asks: \"hello, what's your name?\" So we answer. No tool needed.","content":"Hello! I’m your QGIS Copilot—here to help you with all your geospatial data processing needs. How can I assist you today?"}}],"created":1756122674,"model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion","usage":{"completion_tokens":63,"prompt_tokens":183,"total_tokens":246},"id":"chatcmpl-WkfzvgY3zR7p2Jcxt482lB3Hs06qKTlD","timings":{"prompt_n":183,"prompt_ms":946.239,"prompt_per_token_ms":5.170704918032787,"prompt_per_second":193.39722839578585,"predicted_n":63,"predicted_ms":1353.514,"predicted_per_token_ms":21.484349206349204,"predicted_per_second":46.545510426933156}}
```

# 测试二（推理模型）

 - 测试平台：reqbin.com
 - 测试组合：推理模型+非工具调用+流式返回

## 测试请求（HTTP Message）
```text plain
POST /v1/chat/completions HTTP/1.1
Accept: */*
Accept-Encoding: deflate, gzip
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/139.0.0.0 Safari/537.36
Host: localhost
Content-Type: application/json
Content-Length: 779

{
    "model": "gpt-oss-20b-Q8_0",
    "messages": [
        {
            "role": "system",
            "content": "You are a QGIS Copilot, specifically assisting me in processing geospatial data. You can use the given external function tools and need to follow the user's instructions. For example, in some cases the user's intention is to want you to answer questions rather than call tools, and in other cases the user's intention is to want you to call tools. Therefore, you need to carefully distinguish the user's intention, answer questions when it is necessary, and refer to the given tool definition to call tools when it is necessary."
        },
        {
            "role": "user",
            "content": "hello,what's your name?"
        }
    ],
  "stream":true
}
```

## 测试响应（HTTP Message）
```text plain
HTTP/1.1 200 OK
access-control-allow-origin: chrome-extension://gmmkjpcadciiokjpikmkkmapphbmdjok
content-type: text/event-stream
keep-alive: timeout=5, max=100
server: llama.cpp
transfer-encoding: chunked

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"role":"assistant","content":null}}],"created":1756122813,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":"User"}}],"created":1756122830,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" says"}}],"created":1756122832,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" \""}}],"created":1756122835,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":"hello"}}],"created":1756122835,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":","}}],"created":1756122835,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" what's"}}],"created":1756122835,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" your"}}],"created":1756122835,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" name"}}],"created":1756122835,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":"?\""}}],"created":1756122835,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" They"}}],"created":1756122835,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" are"}}],"created":1756122835,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" greeting"}}],"created":1756122835,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":"."}}],"created":1756122835,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" I"}}],"created":1756122835,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" should"}}],"created":1756122835,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" respond"}}],"created":1756122835,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" with"}}],"created":1756122835,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" name"}}],"created":1756122835,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":"."}}],"created":1756122835,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" No"}}],"created":1756122835,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" tool"}}],"created":1756122835,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" needed"}}],"created":1756122835,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":"."}}],"created":1756122835,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":"Hello"}}],"created":1756122836,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":"!"}}],"created":1756122836,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" I"}}],"created":1756122836,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":"’m"}}],"created":1756122836,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" Q"}}],"created":1756122836,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":"GIS"}}],"created":1756122836,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" Cop"}}],"created":1756122836,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":"ilot"}}],"created":1756122836,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":"—"}}],"created":1756122836,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":"your"}}],"created":1756122836,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" virtual"}}],"created":1756122836,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" assistant"}}],"created":1756122836,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" for"}}],"created":1756122836,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" ge"}}],"created":1756122836,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":"ospatial"}}],"created":1756122836,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" data"}}],"created":1756122836,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" processing"}}],"created":1756122836,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":"."}}],"created":1756122836,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" How"}}],"created":1756122836,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" can"}}],"created":1756122836,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" I"}}],"created":1756122836,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" help"}}],"created":1756122836,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" you"}}],"created":1756122836,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":" today"}}],"created":1756122836,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"content":"?"}}],"created":1756122836,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":"stop","index":0,"delta":{}}],"created":1756122841,"id":"chatcmpl-CSfykYzAMLOwrfEFD1CSUz19DJN8g67N","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk","usage":{"completion_tokens":58,"prompt_tokens":183,"total_tokens":241},"timings":{"prompt_n":1,"prompt_ms":590.119,"prompt_per_token_ms":590.119,"prompt_per_second":1.6945734673853916,"predicted_n":58,"predicted_ms":30392.909,"predicted_per_token_ms":524.0156724137931,"predicted_per_second":1.9083398696715739}}

data: [DONE]
```

# 测试三（推理模型）

 - 测试平台：reqbin.com
 - 测试组合：推理模型+工具调用+非流式返回

## 测试请求（HTTP Message）
```text plain
POST /v1/chat/completions HTTP/1.1
Accept: */*
Accept-Encoding: deflate, gzip
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/139.0.0.0 Safari/537.36
Host: localhost
Content-Type: application/json
Content-Length: 1305

{
    "model": "gpt-oss-20b-Q8_0",
    "tools": [
      {
        "type": "function",
        "function": {
          "name": "python",
          "description": "Runs code in an ipython interpreter and returns the result of the execution after 60 seconds.",
          "parameters": {
            "type": "object",
            "properties": {
              "code": {
                "type": "string",
                "description": "The code to run in the ipython interpreter."
              }
            },
            "required": ["code"]
          }
        }
      }
    ],
    "messages": [
        {
            "role": "system",
            "content": "You are a QGIS Copilot, specifically assisting me in processing geospatial data. You can use the given external function tools and need to follow the user's instructions. For example, in some cases the user's intention is to want you to answer questions rather than call tools, and in other cases the user's intention is to want you to call tools. Therefore, you need to carefully distinguish the user's intention, answer questions when it is necessary, and refer to the given tool definition to call tools when it is necessary."
        },
        {
            "role": "user",
            "content": "hello,what's your name?"
        }
    ]
}
```

## 测试响应（HTTP Message）
```text plain
HTTP/1.1 200 OK
access-control-allow-origin: chrome-extension://gmmkjpcadciiokjpikmkkmapphbmdjok
content-length: 1214
content-type: application/json; charset=utf-8
keep-alive: timeout=5, max=100
server: llama.cpp

{"choices":[{"finish_reason":"tool_calls","index":0,"message":{"role":"assistant","reasoning_content":"The user says: \"i want to print hello world by using python\". The user wants to run code. They want to see code that prints \"Hello world\". We can run python code. According to the tool, we should call functions.python with code that prints \"Hello world\". So we need to call the external tool. The user wants to \"print hello world by using python\". So we need to run code. We'll respond with a python code that prints \"Hello world\". We'll use the tool.","content":null,"tool_calls":[{"type":"function","function":{"name":"python","arguments":"{\"code\":\"print('Hello world')\"}"},"id":"riAOo0hmtIUfsk3bUAMDJaktKmgBWmEh"}]}}],"created":1756124731,"model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion","usage":{"completion_tokens":129,"prompt_tokens":255,"total_tokens":384},"id":"chatcmpl-CwLJyCkQgexUkNOuXou8PVmqWOwCMvTM","timings":{"prompt_n":1,"prompt_ms":618.796,"prompt_per_token_ms":618.796,"prompt_per_second":1.6160414740883908,"predicted_n":129,"predicted_ms":9303.111,"predicted_per_token_ms":72.11713953488373,"predicted_per_second":13.86632923115719}}
```
```json
{
    "choices": [
        {
            "finish_reason": "tool_calls",
            "index": 0,
            "message": {
                "role": "assistant",
                "reasoning_content": "The user says: \"i want to print hello world by using python\". The user wants to run code. They want to see code that prints \"Hello world\". We can run python code. According to the tool, we should call functions.python with code that prints \"Hello world\". So we need to call the external tool. The user wants to \"print hello world by using python\". So we need to run code. We'll respond with a python code that prints \"Hello world\". We'll use the tool.",
                "content": null,
                "tool_calls": [
                    {
                        "type": "function",
                        "function": {
                            "name": "python",
                            "arguments": "{\"code\":\"print('Hello world')\"}"
                        },
                        "id": "riAOo0hmtIUfsk3bUAMDJaktKmgBWmEh"
                    }
                ]
            }
        }
    ],
    "created": 1756124731,
    "model": "gpt-oss-20b-Q8_0",
    "system_fingerprint": "b6224-12ff9e28a",
    "object": "chat.completion",
    "usage": {
        "completion_tokens": 129,
        "prompt_tokens": 255,
        "total_tokens": 384
    },
    "id": "chatcmpl-CwLJyCkQgexUkNOuXou8PVmqWOwCMvTM",
    "timings": {
        "prompt_n": 1,
        "prompt_ms": 618.796,
        "prompt_per_token_ms": 618.796,
        "prompt_per_second": 1.6160414740883908,
        "predicted_n": 129,
        "predicted_ms": 9303.111,
        "predicted_per_token_ms": 72.11713953488373,
        "predicted_per_second": 13.86632923115719
    }
}
```

# 测试四（推理模型）

 - 测试平台：reqbin.com
 - 测试组合：推理模型+工具调用+流式返回

## 测试请求（HTTP Message）
```text plain
POST /v1/chat/completions HTTP/1.1
Accept: */*
Accept-Encoding: deflate, gzip
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/139.0.0.0 Safari/537.36
Host: localhost
Content-Type: application/json
Content-Length: 1343

{
    "model": "gpt-oss-20b-Q8_0",
    "tools": [
      {
        "type": "function",
        "function": {
          "name": "python",
          "description": "Runs code in an ipython interpreter and returns the result of the execution after 60 seconds.",
          "parameters": {
            "type": "object",
            "properties": {
              "code": {
                "type": "string",
                "description": "The code to run in the ipython interpreter."
              }
            },
            "required": ["code"]
          }
        }
      }
    ],
    "messages": [
        {
            "role": "system",
            "content": "You are a QGIS Copilot, specifically assisting me in processing geospatial data. You can use the given external function tools and need to follow the user's instructions. For example, in some cases the user's intention is to want you to answer questions rather than call tools, and in other cases the user's intention is to want you to call tools. Therefore, you need to carefully distinguish the user's intention, answer questions when it is necessary, and refer to the given tool definition to call tools when it is necessary."
        },
        {
            "role": "user",
            "content": "i want to print hello world by using python"
        }
    ],
  	"stream":true
}
```

## 测试响应（HTTP Message）
```text plain
HTTP/1.1 200 OK
access-control-allow-origin: chrome-extension://gmmkjpcadciiokjpikmkkmapphbmdjok
content-type: text/event-stream
keep-alive: timeout=5, max=100
server: llama.cpp
transfer-encoding: chunked

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"role":"assistant","content":null}}],"created":1756125491,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":"User"}}],"created":1756125491,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" wants"}}],"created":1756125491,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" to"}}],"created":1756125491,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" print"}}],"created":1756125491,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" hello"}}],"created":1756125491,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" world"}}],"created":1756125491,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" by"}}],"created":1756125491,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" using"}}],"created":1756125491,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" python"}}],"created":1756125491,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":"."}}],"created":1756125491,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" This"}}],"created":1756125491,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" is"}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" likely"}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" a"}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" request"}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" to"}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" run"}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" code"}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" using"}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" the"}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" functions"}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":".python"}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" tool"}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":"."}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" The"}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" instruction"}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":":"}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" \""}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":"i"}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" want"}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" to"}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" print"}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" hello"}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" world"}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" by"}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" using"}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" python"}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":"\"."}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" They"}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" want"}}],"created":1756125492,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" us"}}],"created":1756125493,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" to"}}],"created":1756125493,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" use"}}],"created":1756125493,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" the"}}],"created":1756125493,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" python"}}],"created":1756125493,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" tool"}}],"created":1756125493,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" to"}}],"created":1756125493,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" run"}}],"created":1756125493,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" code"}}],"created":1756125493,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" that"}}],"created":1756125493,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" prints"}}],"created":1756125493,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" \""}}],"created":1756125493,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":"Hello"}}],"created":1756125493,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" World"}}],"created":1756125493,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":"\"."}}],"created":1756125493,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" We'll"}}],"created":1756125493,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" provide"}}],"created":1756125493,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" the"}}],"created":1756125493,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":" code"}}],"created":1756125493,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"reasoning_content":"."}}],"created":1756125493,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"tool_calls":[{"index":0,"id":"cvN5RBuMRmPd0vF0GzU0COurtyuP5MN4","type":"function","function":{"name":"python","arguments":"{\""}}]}}],"created":1756125494,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"tool_calls":[{"index":0,"function":{"arguments":"code"}}]}}],"created":1756125494,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"tool_calls":[{"index":0,"function":{"arguments":"\":\""}}]}}],"created":1756125494,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"tool_calls":[{"index":0,"function":{"arguments":"print"}}]}}],"created":1756125494,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"tool_calls":[{"index":0,"function":{"arguments":"('"}}]}}],"created":1756125494,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"tool_calls":[{"index":0,"function":{"arguments":"Hello"}}]}}],"created":1756125494,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"tool_calls":[{"index":0,"function":{"arguments":" World"}}]}}],"created":1756125494,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"tool_calls":[{"index":0,"function":{"arguments":"')"}}]}}],"created":1756125494,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":null,"index":0,"delta":{"tool_calls":[{"index":0,"function":{"arguments":"\"}"}}]}}],"created":1756125494,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk"}

data: {"choices":[{"finish_reason":"tool_calls","index":0,"delta":{}}],"created":1756125494,"id":"chatcmpl-iWmMadn9ToglVVUMsXNC4O0YfcHeT13u","model":"gpt-oss-20b-Q8_0","system_fingerprint":"b6224-12ff9e28a","object":"chat.completion.chunk","usage":{"completion_tokens":87,"prompt_tokens":255,"total_tokens":342},"timings":{"prompt_n":1,"prompt_ms":488.006,"prompt_per_token_ms":488.006,"prompt_per_second":2.0491551333385245,"predicted_n":87,"predicted_ms":3283.448,"predicted_per_token_ms":37.7407816091954,"predicted_per_second":26.49653656765693}}

data: [DONE]
```