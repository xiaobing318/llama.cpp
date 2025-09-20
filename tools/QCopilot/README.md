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

### 3. read_text_lines
Read a UTF-8 text file by line range with BOM/CRLF normalization and optional UTF-8 enforcement.

**Parameters:**
- `path` (string, required): Target file path
- `start_line` (integer, optional): 1-based inclusive start line (default 1)
- `end_line` (integer, optional): inclusive end line, `<=0` means EOF
- `include_line_numbers` (bool, optional): Include `{no,text}` array (default true)
- `enforce_utf8` (bool, optional): Validate UTF-8 (default true)
- `max_file_size_bytes` (integer, optional): Size guard (default 100 MB)

### 4. write_text_file
Write UTF-8 text content to a file with append/overwrite support and safe path validation.

**Parameters:**
- `path` (string, required): Target file path (parent directory must exist)
- `content` (string, required): UTF-8 text content
- `append` (bool, optional): Append instead of overwrite (default false)

### 5. validate_utf8_file
Verify whether a file is valid UTF-8 text.

**Parameters:**
- `path` (string, required): File path for validation

### 6. list_directory
Enumerate directory entries with options for recursion, hidden files, and size reporting.

**Parameters:**
- `path` (string, required): Directory path
- `recursive` (bool, optional): Recurse into subdirectories (default false)
- `show_hidden` (bool, optional): Include hidden/system entries (default false)
- `include_size` (bool, optional): Include file sizes/human_size (default true)
- `max_results` (integer, optional): Soft cap on results (default 50000)

### 7. path_stat
Inspect a path and return metadata such as type, size, timestamps, permissions, and optional text statistics.

**Parameters:**
- `path` (string, required): Path to inspect
- `detailed` (bool, optional): Enable extended metadata (default false)
- `text_analysis` (bool, optional): Count lines/chars for text files (default true)

### 8. grep
Search for patterns in files or directory trees. Supports regex/literal modes, glob filtering, and match limits.

**Parameters:**
- `path` (string, required): File or directory to search
- `pattern` (string, required): Search pattern
- `use_regex` (bool, optional): Treat pattern as ECMAScript regex (default false)
- `case_sensitive` (bool, optional): Case-sensitive search (default true)
- `line_numbers` (bool, optional): Include line numbers (default true)
- `recursive` (bool, optional): Recurse into directories (default false)
- `file_glob` (string, optional): Glob filter for files (default `*` or `**/*` when recursive)
- `follow_symlinks` (bool, optional): Follow directory symlinks (default false)
- `show_hidden` (bool, optional): Include hidden files (default false)
- `max_matches` (integer, optional): Soft cap on total matches (default 10000)

### 9. glob
Expand shell-style patterns (`*`, `?`, `[]`, `**`) under a base directory.

**Parameters:**
- `base_dir` (string, required): Base directory for expansion
- `pattern` (string, required): Glob pattern
- `include_directories` (bool, optional): Include matching directories (default false)
- `follow_symlinks` (bool, optional): Follow directory symlinks (default false)
- `case_sensitive` (bool, optional): Pattern matching sensitivity (default platform-dependent)
- `show_hidden` (bool, optional): Include hidden files/directories (default false)
- `max_results` (integer, optional): Soft cap on expanded entries (default 50)

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
