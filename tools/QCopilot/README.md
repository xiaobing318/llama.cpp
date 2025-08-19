# C++ QCopilot

A C++ agent that manages llama-server and provides tool calling capabilities for large language models.

## Features

- **Automatic Server Management**: Automatically starts and manages llama-server process
- **Tool Calling Support**: Intercepts and executes tool calls from the model
- **Built-in Tools**: Includes 5 essential tools ready to use
- **REST API**: Compatible with OpenAI API format
- **Configuration**: JSON-based configuration for easy setup

## Prerequisites

- C++17 compatible compiler
- CMake 3.16 or higher
- llama.cpp built with server support
- OpenSSL (optional, for HTTPS support)

## Building

1. Place the QCopilot folder in `llama.cpp/tools/`
2. Build llama.cpp with the agent:

```bash
cd llama.cpp
mkdir build
cd build
cmake .. -DLLAMA_BUILD_EXAMPLES=ON
cmake --build . --config Release
```

## Configuration

Edit `QCopilotConfig.json` to configure the agent:

```json
{
  "qcopilot_host": "127.0.0.1",
  "qcopilot_port": 8081,
  "base_server_host": "127.0.0.1",
  "base_server_port": 8080,
  "base_server_path": "./llama-server",
  "model_path": "path/to/your/model.gguf",
  "n_ctx": 2048,
  "n_gpu_layers": -1,
  "auto_start_base_server": true,
  "tools": [...]
}
```

## Usage

### Starting the Agent

```bash
./qcopilot [QCopilotConfig.json]
```

### API Endpoints

#### Health Check
```http
GET /health
```

#### List Available Tools
```http
GET /tools
```

#### Chat Completion with Tools
```http
POST /v1/chat/completions
Content-Type: application/json

{
  "messages": [
    {"role": "user", "content": "What time is it?"}
  ],
  "model": "gpt-3.5-turbo"
}
```

#### Direct Tool Execution
```http
POST /execute_tool
Content-Type: application/json

{
  "name": "get_current_time",
  "arguments": {
    "format": "ISO8601"
  }
}
```

## Built-in Tools

### 1. get_current_time
Get the current date and time.

**Parameters:**
- `format` (string, optional): Time format ("ISO8601", "unix", "readable")
- `timezone` (string, optional): Timezone ("local", "UTC")

### 2. calculate
Perform basic mathematical calculations.

**Parameters:**
- `expression` (string, required): Mathematical expression to evaluate

### 3. read_file
Read contents of a file.

**Parameters:**
- `path` (string, required): Path to the file
- `encoding` (string, optional): File encoding (default: "utf-8")

### 4. write_file
Write content to a file.

**Parameters:**
- `path` (string, required): Path to the file
- `content` (string, required): Content to write
- `append` (bool, optional): Append to existing file (default: false)

### 5. list_files
List files in a directory.

**Parameters:**
- `directory` (string, optional): Directory path (default: ".")
- `pattern` (string, optional): File pattern filter (default: "*")
- `recursive` (bool, optional): List recursively (default: false)

## Tool Calling Flow

1. User sends a message that requires tool use
2. QCopilotConfig forwards request to llama-server with tool definitions
3. Model returns tool call request
4. QCopilotConfig executes the requested tool
5. QCopilotConfig sends tool results back to model
6. Model generates final response with tool results
7. QCopilotConfig returns complete response to user

## Example Python Client

```python
import requests
import json

# Configure the agent URL
AGENT_URL = "http://localhost:8081"

# Send a chat completion request
response = requests.post(
    f"{AGENT_URL}/v1/chat/completions",
    json={
        "messages": [
            {"role": "user", "content": "Save the current time to a file called time.txt"}
        ],
        "model": "gpt-3.5-turbo"
    }
)

print(json.dumps(response.json(), indent=2))
```

## Troubleshooting

### QCopilotConfig fails to start
- Check if the ports are available
- Verify model path in config.json
- Ensure llama-server binary exists

### Tool execution fails
- Check file permissions for file operations
- Verify tool parameters are correct
- Check agent logs for error messages

### Connection issues
- Ensure firewall allows the configured ports
- Check if llama-server is running
- Verify network configuration

## Security Considerations

- The agent executes system commands and file operations
- Run in a controlled environment
- Restrict file access paths as needed
- Use authentication in production environments

## License

This project follows the same license as llama.cpp.

## Contributing

Contributions are welcome! Please submit pull requests or open issues for bugs and feature requests.
