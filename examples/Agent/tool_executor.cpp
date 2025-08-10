#include "tool_executor.h"
#include "agent_utils.h"
#include <cmath>
#include <sstream>
#include <iomanip>

ToolExecutor::ToolExecutor() {
    registerBuiltinTools();
}

void ToolExecutor::registerBuiltinTools() {
    // Register get_current_time
    tools["get_current_time"] = [this](const json& args) {
        return executeGetCurrentTime(args);
    };
    tool_definitions["get_current_time"] = {
        {"type", "function"},
        {"function", {
            {"name", "get_current_time"},
            {"description", "获取当前时间"},
            {"parameters", {
                {"type", "object"},
                {"properties", {
                    {"format", {{"type", "string"}, {"description", "时间格式：ISO8601, unix, 或默认格式"}}},
                    {"timezone", {{"type", "string"}, {"description", "时区：local 或 UTC"}}}
                }}
            }}
        }}
    };

    // Register calculate
    tools["calculate"] = [this](const json& args) {
        return executeCalculate(args);
    };
    tool_definitions["calculate"] = {
        {"type", "function"},
        {"function", {
            {"name", "calculate"},
            {"description", "计算数学表达式"},
            {"parameters", {
                {"type", "object"},
                {"properties", {
                    {"expression", {{"type", "string"}, {"description", "要计算的数学表达式"}}}
                }},
                {"required", {"expression"}}
            }}
        }}
    };

    // Register read_file
    tools["read_file"] = [this](const json& args) {
        return executeReadFile(args);
    };
    tool_definitions["read_file"] = {
        {"type", "function"},
        {"function", {
            {"name", "read_file"},
            {"description", "读取文件内容"},
            {"parameters", {
                {"type", "object"},
                {"properties", {
                    {"path", {{"type", "string"}, {"description", "文件路径"}}},
                    {"encoding", {{"type", "string"}, {"description", "文件编码，默认utf-8"}}}
                }},
                {"required", {"path"}}
            }}
        }}
    };

    // Register write_file
    tools["write_file"] = [this](const json& args) {
        return executeWriteFile(args);
    };
    tool_definitions["write_file"] = {
        {"type", "function"},
        {"function", {
            {"name", "write_file"},
            {"description", "写入文件内容"},
            {"parameters", {
                {"type", "object"},
                {"properties", {
                    {"path", {{"type", "string"}, {"description", "文件路径"}}},
                    {"content", {{"type", "string"}, {"description", "要写入的内容"}}},
                    {"append", {{"type", "boolean"}, {"description", "是否追加到文件末尾"}}}
                }},
                {"required", {"path", "content"}}
            }}
        }}
    };

    // Register list_files
    tools["list_files"] = [this](const json& args) {
        return executeListFiles(args);
    };
    tool_definitions["list_files"] = {
        {"type", "function"},
        {"function", {
            {"name", "list_files"},
            {"description", "列出目录中的文件"},
            {"parameters", {
                {"type", "object"},
                {"properties", {
                    {"directory", {{"type", "string"}, {"description", "目录路径，默认为当前目录"}}},
                    {"pattern", {{"type", "string"}, {"description", "文件名匹配模式"}}},
                    {"recursive", {{"type", "boolean"}, {"description", "是否递归搜索子目录"}}}
                }}
            }}
        }}
    };
}

void ToolExecutor::registerTool(const json& tool_definition) {
    // 检查配置文件中的工具定义是否包含必需的字段
    if (!tool_definition.contains("function")) {
        LOG_ERR("工具定义缺失 'function' 字段，每个工具定义中必须存在 'function' 字段，请检查配置文件中对工具定义是否正确。\n");
        return;
    }
    // 获取工具定义中 'function' 字段的属性值并且将其保存在临时的 json 对象中。
    json function = tool_definition["function"];
    // 获取 'function' 字段的属性值中的 "name" 字段值，如果没有设置则默认为空字符串。
    std::string name = function.value("name", "");
    // 如果工具名称为空或者不符合命名规范，则记录错误日志并返回。
    if (name.empty() || !validate_tool_name(name)) {
        LOG_ERR("不是有效的工具名称: %s\n", name.c_str());
        return;
    }

    // TODO：检查是否已经注册了同名的工具

    // 经过上述检查后说明配置文件中的当前工具定义是有效的，将其保存到内存中的工具定义映射中。
    tool_definitions[name] = tool_definition;
    LOG_INF("成功注册工具： %s\n", name.c_str());
}

// tool_executor.cpp

json ToolExecutor::execute(const std::string& name, const json& arguments) {
    // 首先检查是否为内置工具
    auto it = tools.find(name);
    if (it != tools.end()) {
        try {
            // Validate arguments if schema exists
            auto defIt = tool_definitions.find(name);
            if (defIt != tool_definitions.end()) {
                const json& definition = defIt->second;
                if (definition.contains("function") &&
                    definition["function"].contains("parameters")) {
                    if (!validate_arguments(arguments, definition["function"]["parameters"])) {
                        return json{
                            {"error", "Invalid arguments"},
                            {"success", false}
                        };
                    }
                }
            }

            // Execute the builtin tool
            return it->second(arguments);

        } catch (const std::exception& e) {
            LOG_ERR("Tool execution failed: %s\n", e.what());
            return json{
                {"error", e.what()},
                {"success", false}
            };
        }
    }
    
    // 检查是否为外部工具（在 tool_definitions 中有定义但没有内置实现）
    auto defIt = tool_definitions.find(name);
    if (defIt != tool_definitions.end()) {
        const json& definition = defIt->second;
        
        // 检查是否为外部工具（包含 executable 字段）
        if (definition.contains("executable")) {
            std::string executable = definition["executable"];
            
            try {
                // Validate arguments if schema exists
                if (definition.contains("function") &&
                    definition["function"].contains("parameters")) {
                    if (!validate_arguments(arguments, definition["function"]["parameters"])) {
                        return json{
                            {"error", "Invalid arguments"},
                            {"success", false}
                        };
                    }
                }
                
                // Execute external tool
                return executeExternalTool(executable, arguments);
                
            } catch (const std::exception& e) {
                LOG_ERR("External tool execution failed: %s\n", e.what());
                return json{
                    {"error", e.what()},
                    {"success", false}
                };
            }
        }
    }
    
    // 工具不存在
    return json{
        {"error", "Tool not found: " + name},
        {"success", false}
    };
}


bool ToolExecutor::hasTool(const std::string& name) const {
    return tools.find(name) != tools.end() || tool_definitions.find(name) != tool_definitions.end();
}

json ToolExecutor::getTools() const {
    json result = json::array();
    for (const auto& [name, definition] : tool_definitions) {
        result.push_back(definition);
    }
    return result;
}

json ToolExecutor::executeGetCurrentTime(const json& args) {
    std::string format = args.value("format", "ISO8601");
    std::string timezone = args.value("timezone", "local");

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    if (format == "ISO8601") {
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    } else if (format == "unix") {
        ss << time_t;
    } else {
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    }

    return json{
        {"time", ss.str()},
        {"format", format},
        {"timezone", timezone},
        {"success", true}
    };
}

json ToolExecutor::executeCalculate(const json& args) {
    std::string expression = args.value("expression", "");

    if (expression.empty()) {
        return json{
            {"error", "Expression is required"},
            {"success", false}
        };
    }

    // Simple calculator implementation (supports +, -, *, /)
    // Note: This is a very basic implementation
    try {
        double result = 0;
        char op = '+';
        std::stringstream ss(expression);
        double num;

        while (ss >> num) {
            switch (op) {
                case '+': result += num; break;
                case '-': result -= num; break;
                case '*': result *= num; break;
                case '/':
                    if (num == 0) {
                        return json{
                            {"error", "Division by zero"},
                            {"success", false}
                        };
                    }
                    result /= num;
                    break;
            }
            ss >> op;
        }

        return json{
            {"expression", expression},
            {"result", result},
            {"success", true}
        };

    } catch (const std::exception& e) {
        return json{
            {"error", "Failed to evaluate expression"},
            {"success", false}
        };
    }
}

json ToolExecutor::executeReadFile(const json& args) {
    std::string path = args.value("path", "");
    std::string encoding = args.value("encoding", "utf-8");

    if (path.empty()) {
        return json{
            {"error", "Path is required"},
            {"success", false}
        };
    }

    if (!file_exists(path)) {
        return json{
            {"error", "File not found"},
            {"success", false}
        };
    }

    std::string content;
    if (!read_file_content(path, content)) {
        return json{
            {"error", "Failed to read file"},
            {"success", false}
        };
    }

    return json{
        {"path", path},
        {"content", content},
        {"size", content.size()},
        {"success", true}
    };
}

json ToolExecutor::executeWriteFile(const json& args) {
    std::string path = args.value("path", "");
    std::string content = args.value("content", "");
    bool append = args.value("append", false);

    if (path.empty()) {
        return json{
            {"error", "Path is required"},
            {"success", false}
        };
    }

    std::string final_content = content;
    if (append && file_exists(path)) {
        std::string existing;
        if (read_file_content(path, existing)) {
            final_content = existing + content;
        }
    }

    if (!write_file_content(path, final_content)) {
        return json{
            {"error", "Failed to write file"},
            {"success", false}
        };
    }

    return json{
        {"path", path},
        {"bytes_written", final_content.size()},
        {"success", true}
    };
}

json ToolExecutor::executeListFiles(const json& args) {
    std::string directory = args.value("directory", ".");
    std::string pattern = args.value("pattern", "*");
    bool recursive = args.value("recursive", false);

    if (!file_exists(directory)) {
        return json{
            {"error", "Directory not found"},
            {"success", false}
        };
    }

    std::vector<std::string> files = list_directory(directory);

    // Simple pattern matching (only supports * wildcard)
    if (pattern != "*") {
        std::vector<std::string> filtered;
        for (const auto& file : files) {
            if (pattern.front() == '*') {
                std::string suffix = pattern.substr(1);
                if (file.size() >= suffix.size() &&
                    file.substr(file.size() - suffix.size()) == suffix) {
                    filtered.push_back(file);
                }
            } else if (pattern.back() == '*') {
                std::string prefix = pattern.substr(0, pattern.size() - 1);
                if (file.size() >= prefix.size() &&
                    file.substr(0, prefix.size()) == prefix) {
                    filtered.push_back(file);
                }
            } else if (file == pattern) {
                filtered.push_back(file);
            }
        }
        files = filtered;
    }

    return json{
        {"directory", directory},
        {"files", files},
        {"count", files.size()},
        {"success", true}
    };
}

json ToolExecutor::executeExternalTool(const std::string& executable, const json& arguments) {
    try {
        // 构建命令行，将 JSON 参数作为标准输入传递给外部程序
        std::string cmd = executable;
        
        LOG_INF("执行外部工具: %s\n", cmd.c_str());
        
        // 使用跨平台的方式执行外部命令并捕获输出
#ifdef _WIN32
        // Windows implementation
        SECURITY_ATTRIBUTES saAttr;
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        HANDLE hChildStd_IN_Rd = NULL;
        HANDLE hChildStd_IN_Wr = NULL;
        HANDLE hChildStd_OUT_Rd = NULL;
        HANDLE hChildStd_OUT_Wr = NULL;

        // 创建管道用于标准输入输出
        if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0) ||
            !SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0) ||
            !CreatePipe(&hChildStd_IN_Rd, &hChildStd_IN_Wr, &saAttr, 0) ||
            !SetHandleInformation(hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0)) {
            return json{
                {"error", "Failed to create pipes"},
                {"success", false}
            };
        }

        PROCESS_INFORMATION piProcInfo;
        STARTUPINFOA siStartInfo;
        ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
        ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
        siStartInfo.cb = sizeof(STARTUPINFO);
        siStartInfo.hStdError = hChildStd_OUT_Wr;
        siStartInfo.hStdOutput = hChildStd_OUT_Wr;
        siStartInfo.hStdInput = hChildStd_IN_Rd;
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

        BOOL bSuccess = CreateProcessA(NULL,
            const_cast<char*>(cmd.c_str()),
            NULL, NULL, TRUE, 0, NULL, NULL,
            &siStartInfo, &piProcInfo);

        if (!bSuccess) {
            CloseHandle(hChildStd_OUT_Rd);
            CloseHandle(hChildStd_OUT_Wr);
            CloseHandle(hChildStd_IN_Rd);
            CloseHandle(hChildStd_IN_Wr);
            return json{
                {"error", "Failed to create process"},
                {"success", false}
            };
        }

        // 关闭子进程端的管道句柄
        CloseHandle(hChildStd_OUT_Wr);
        CloseHandle(hChildStd_IN_Rd);

        // 向子进程发送 JSON 参数
        std::string json_input = arguments.dump();
        DWORD dwWritten;
        WriteFile(hChildStd_IN_Wr, json_input.c_str(), json_input.length(), &dwWritten, NULL);
        CloseHandle(hChildStd_IN_Wr);

        // 读取子进程输出
        std::string output;
        DWORD dwRead;
        CHAR chBuf[4096];
        while (ReadFile(hChildStd_OUT_Rd, chBuf, 4095, &dwRead, NULL) && dwRead > 0) {
            chBuf[dwRead] = '\0';
            output += chBuf;
        }

        // 等待进程结束
        WaitForSingleObject(piProcInfo.hProcess, INFINITE);
        DWORD exitCode;
        GetExitCodeProcess(piProcInfo.hProcess, &exitCode);

        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);
        CloseHandle(hChildStd_OUT_Rd);

        if (exitCode != 0) {
            return json{
                {"error", "External tool exited with code " + std::to_string(exitCode)},
                {"output", output},
                {"success", false}
            };
        }

        // 尝试解析输出为 JSON，如果失败则作为文本返回
        try {
            json result = json::parse(output);
            result["success"] = true;
            return result;
        } catch (const std::exception&) {
            return json{
                {"output", output},
                {"success", true}
            };
        }

#else
        // Linux/Unix implementation using fork and pipes
        int stdin_pipe[2];
        int stdout_pipe[2];
        
        if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1) {
            return json{
                {"error", "Failed to create pipes"},
                {"success", false}
            };
        }

        pid_t pid = fork();
        if (pid == -1) {
            close(stdin_pipe[0]);
            close(stdin_pipe[1]);
            close(stdout_pipe[0]);
            close(stdout_pipe[1]);
            return json{
                {"error", "Failed to fork process"},
                {"success", false}
            };
        }

        if (pid == 0) {
            // 子进程
            close(stdin_pipe[1]);   // 关闭写端
            close(stdout_pipe[0]);  // 关闭读端
            
            // 重定向标准输入和输出
            dup2(stdin_pipe[0], STDIN_FILENO);
            dup2(stdout_pipe[1], STDOUT_FILENO);
            dup2(stdout_pipe[1], STDERR_FILENO);
            
            close(stdin_pipe[0]);
            close(stdout_pipe[1]);
            
            // 执行外部命令
            execlp("/bin/sh", "sh", "-c", cmd.c_str(), (char*)NULL);
            exit(127); // 如果 exec 失败
        } else {
            // 父进程
            close(stdin_pipe[0]);   // 关闭读端
            close(stdout_pipe[1]);  // 关闭写端
            
            // 向子进程发送 JSON 参数
            std::string json_input = arguments.dump();
            write(stdin_pipe[1], json_input.c_str(), json_input.length());
            close(stdin_pipe[1]);
            
            // 读取子进程输出
            std::string output;
            char buffer[4096];
            ssize_t bytes_read;
            while ((bytes_read = read(stdout_pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
                buffer[bytes_read] = '\0';
                output += buffer;
            }
            close(stdout_pipe[0]);
            
            // 等待子进程结束
            int status;
            waitpid(pid, &status, 0);
            int exit_code = WEXITSTATUS(status);
            
            if (exit_code != 0) {
                return json{
                    {"error", "External tool exited with code " + std::to_string(exit_code)},
                    {"output", output},
                    {"success", false}
                };
            }
            
            // 尝试解析输出为 JSON，如果失败则作为文本返回
            try {
                json result = json::parse(output);
                result["success"] = true;
                return result;
            } catch (const std::exception&) {
                return json{
                    {"output", output},
                    {"success", true}
                };
            }
        }
#endif

    } catch (const std::exception& e) {
        LOG_ERR("External tool execution error: %s\n", e.what());
        return json{
            {"error", e.what()},
            {"success", false}
        };
    }
}
