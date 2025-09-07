// 1. 本模块的头文件必须第一个包含（验证头文件自包含性）
#include "qcopilot_executor.h"

// 2. 相关项目头文件
#include "qcopilot_utils.h"
#include "qcopilot_builtin_tools.h"

// 3. C++标准库头文件
#include <cmath>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cstring>

// 4. 平台特定头文件
#ifdef _WIN32
    #include <windows.h>
#else
    // Linux platform specific headers
    #include <unistd.h>      // pipe, fork, dup2, execlp/execvp, read, write, close, STDIN_FILENO...
    #include <sys/types.h>   // pid_t
    #include <sys/wait.h>    // waitpid, WIFEXITED, WEXITSTATUS, WIFSIGNALED, WTERMSIG
    #include <fcntl.h>       // 可选：pipe2, O_CLOEXEC 等
    #include <errno.h>       // errno
#endif

// 构造函数
ToolExecutor::ToolExecutor() {
    //  在构造 ToolExecutor 实体的时候自动注册内置工具。
    registerBuiltinTools();
}

// 注册内置工具
void ToolExecutor::registerBuiltinTools() {
    // 使用新的内置工具模块获取工具定义和函数
    auto definitions = BuiltinTools::getBuiltinToolDefinitions();
    auto functions = BuiltinTools::getBuiltinToolFunctions();

    // 注册所有内置工具
    for (const auto& definition : definitions) {
        const std::string& name = definition.name;

        // 注册工具定义
        tool_definitions[name] = definition.definition;

        // 注册工具执行函数
        if (functions.find(name) != functions.end()) {
            builtinTools[name] = functions[name];
        }

        // 输出日志
        LOG_INF("成功注册内置工具: %s - %s",
            name.c_str(),
            tool_definitions[name]["function"]["description"].get<std::string>().c_str());
    }
}

// 注册外部工具
bool ToolExecutor::registerExternalTools(const json& tool_definition) {
    // 使用增强的验证函数进行全面的工具定义检查
    std::string error_message;
    if (!validateToolDefinition(tool_definition, error_message)) {
        LOG_ERR("工具定义验证失败: %s", error_message.c_str());
        return false;
    }

    // 获取工具名称（经过验证，我们知道这些字段是存在且有效的）
    const json& function = tool_definition["function"];
    std::string name = function["name"].get<std::string>();

    // 检查是否已经注册了同名的工具，如果已经存在，则直接返回不需要进行注册。
    if (hasTool(name)){
        LOG_WRN("工具注册表中已经存在名称为 %s 的工具，请检查配置表中工具定义是否重复", name.c_str());
        return true;
    }

    // 经过上述检查后说明配置文件中的当前工具定义是有效的，将其保存到内存中的工具定义映射中。
    tool_definitions[name] = tool_definition;
    LOG_INF("成功注册外部工具: %s - %s", name.c_str(), function["description"].get<std::string>().c_str());
    return true;
}

// 执行工具
json ToolExecutor::execute(const std::string& name, const json& arguments) {
    // 首先检查是否为内置工具
    auto it = builtinTools.find(name);
    if (it != builtinTools.end()) {
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
            LOG_ERR("Tool execution failed: %s", e.what());
            return json{
                {"error", e.what()},
                {"success", false}
            };
        }
    }

    // 不是内置工具的情况下检查是否为外部工具
    auto defIt = tool_definitions.find(name);
    if (defIt != tool_definitions.end()) {
        const json& definition = defIt->second;

        // 检查是否为外部工具（包含 executable 字段）
        if (definition.contains("executable")) {
            std::string executable = definition["executable"];

            // 根据平台选择可执行文件路径
#ifdef _WIN32
            if (definition.contains("executable_windows")) {
                executable = definition["executable_windows"];
            }
#else
            if (definition.contains("executable_linux")) {
                executable = definition["executable_linux"];
            }
#endif

            std::string command_template;
            // 检查是否有命令模板
            if (definition.contains("command_template")) {
                command_template = definition["command_template"];
            }

            try {
                // Validate arguments if schema exists
                if (definition.contains("function") &&
                    definition["function"].contains("parameters")) {
                    if (!validate_arguments(arguments, definition["function"]["parameters"])) {
                        return json{
                            {"error", "The tool calling's parameters requested by the model to the agent are invalid"},
                            {"success", false}
                        };
                    }
                }

                // Execute external tool with command template
                return executeExternalTool(executable, arguments, command_template);

            } catch (const std::exception& e) {
                LOG_ERR("External tool execution failed: %s", e.what());
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

// 检查工具是否存在
bool ToolExecutor::hasTool(const std::string& name) const {
    return builtinTools.find(name) != builtinTools.end() || tool_definitions.find(name) != tool_definitions.end();
}

// 获取所有注册的工具定义
json ToolExecutor::getTools() const {
    json result = json::array();
    for (const auto& [name, definition] : tool_definitions) {
        result.push_back(definition);
    }
    return result;
}

// 使用默认命令模板执行外部工具（向外部工具的标准输入传递JSON参数）
json ToolExecutor::executeExternalTool(const std::string& executable, const json& arguments) {
    return executeExternalTool(executable, arguments, "");
}

// 使用命令模板执行外部工具
json ToolExecutor::executeExternalTool(const std::string& executable, const json& arguments, const std::string& command_template) {
    try {
        // 构建命令行
        std::string cmd;
        if (!command_template.empty()) {
            // 使用命令模板构建命令，传入可执行文件路径用于替换模板中的占位符
            cmd = buildCommandFromTemplate(command_template, arguments, executable);
        } else {
            // 默认方式：仅使用可执行文件名，参数通过标准输入传递
            cmd = executable;
        }

        LOG_INF("QCopilot 执行外部工具: %s", cmd.c_str());

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

        // 向子进程发送 JSON 参数（仅在没有使用命令模板时）
        if (command_template.empty()) {
            std::string json_input = arguments.dump();
            DWORD dwWritten;
            WriteFile(hChildStd_IN_Wr, json_input.c_str(), json_input.length(), &dwWritten, NULL);
        }
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

            // 向子进程发送 JSON 参数（仅在没有使用命令模板时）
            if (command_template.empty()) {
                std::string json_input = arguments.dump();
                write(stdin_pipe[1], json_input.c_str(), json_input.length());
            }
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
            
            int exit_code = 0;
            if (WIFEXITED(status)) {
                exit_code = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                // 进程被信号终止
                exit_code = 128 + WTERMSIG(status);
            } else {
                // 其他异常情况
                exit_code = -1;
            }

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
        LOG_ERR("External tool execution error: %s", e.what());
        return json{
            {"error", e.what()},
            {"success", false}
        };
    }
}

// 辅助函数：安全地转义命令行参数
std::string escapeShellArgument(const std::string& arg) {
    // 如果参数包含空格或特殊字符，需要用引号包围
    if (arg.find(' ') != std::string::npos ||
        arg.find('\t') != std::string::npos ||
        arg.find('\"') != std::string::npos ||
        arg.find('\'') != std::string::npos ||
        arg.find('$') != std::string::npos ||
        arg.find('`') != std::string::npos ||
        arg.find(';') != std::string::npos ||
        arg.find('&') != std::string::npos ||
        arg.find('|') != std::string::npos) {

        std::string escaped = arg;
        // 转义双引号
        size_t pos = 0;
        while ((pos = escaped.find('\"', pos)) != std::string::npos) {
            escaped.replace(pos, 1, "\\\"");
            pos += 2;
        }
        return "\"" + escaped + "\"";
    }
    return arg;
}

/*
一、根据命令模板和参数构建完整的命令行字符串
    @param command_template 命令模板字符串
    @param arguments JSON格式的参数对象
    @param executable 可执行文件路径，用于替换模板开头的硬编码可执行文件名
    @return 构建好的完整命令行字符串

二、支持的模板语法：
    1. 基本参数替换：{param_name}
       - 直接将参数值替换到占位符位置
       - 示例：模板 "tool -f {input_file}"，参数 {"input_file": "data.txt"}
       - 结果："tool -f data.txt"

    2. 条件替换：{param_name:?text}
       - 如果参数存在且非空，则替换为指定的text，否则为空字符串
       - text中可以再次引用同一参数：{param_name:?-option {param_name}}
       - 示例：模板 "tool {verbose:?-v} {output:?-o {output}} input.txt"
         参数 {"verbose": true, "output": "result.txt"}
       - 结果："tool -v -o result.txt input.txt"

    3. 默认值比较：{param_name:!default_value?text}
       - 如果参数值不等于默认值，则替换为text，否则为空
       - 示例：模板 "tool {format:!auto?-f {format}} input.txt"
         参数 {"format": "json"}
       - 结果："tool -f json input.txt" (因为"json" != "auto")

    4. 数组连接：{param_name:join:separator}
       - 将数组参数用指定分隔符连接
       - 示例：模板 "tool {files:join: } {options:join:,}"
         参数 {"files": ["a.txt", "b.txt"], "options": ["opt1", "opt2"]}
       - 结果："tool "a.txt" "b.txt" "opt1","opt2""
 */
std::string ToolExecutor::buildCommandFromTemplate(const std::string& command_template, const json& arguments, const std::string& executable) {
    std::string result = command_template;

    // 首先替换可执行文件占位符（将模板开头的硬编码可执行文件名替换为平台特定的路径）
    if (!executable.empty()) {
        // 查找模板开头的可执行文件名并替换
        size_t space_pos = result.find(' ');
        if (space_pos != std::string::npos) {
            std::string template_executable = result.substr(0, space_pos);
            // 检查是否为可执行文件名（不含路径分隔符）
            if (template_executable.find('/') == std::string::npos && template_executable.find('\\') == std::string::npos) {
                result = escapeShellArgument(executable) + result.substr(space_pos);
            }
        } else {
            // 整个模板就是可执行文件名
            if (result.find('/') == std::string::npos && result.find('\\') == std::string::npos) {
                result = escapeShellArgument(executable);
            }
        }
    }

    // 开始处理模板中的参数占位符
    size_t pos = 0;
    while ((pos = result.find('{', pos)) != std::string::npos) {
        size_t end_pos = result.find('}', pos);
        if (end_pos == std::string::npos) break;

        std::string placeholder = result.substr(pos + 1, end_pos - pos - 1);
        std::string replacement;

        // 解析占位符
        size_t colon_pos = placeholder.find(':');
        std::string param_name = placeholder;
        std::string modifier;

        if (colon_pos != std::string::npos) {
            param_name = placeholder.substr(0, colon_pos);
            modifier = placeholder.substr(colon_pos + 1);
        }

        // 检查参数是否存在
        bool param_exists = arguments.contains(param_name);
        auto param_value = param_exists ? arguments[param_name] : json();

        if (modifier.empty()) {
            // 简单替换：{param_name}
            if (param_exists && !param_value.is_null()) {
                if (param_value.is_string()) {
                    replacement = escapeShellArgument(param_value.get<std::string>());
                } else {
                    replacement = param_value.dump();
                }
            }
        } else if (modifier[0] == '?') {
            // 条件替换：{param_name:?text}
            if (param_exists && !param_value.is_null()) {
                std::string text = modifier.substr(1);
                // 支持在条件文本中再次引用参数值，如 {encoding:?-lco ENCODING={encoding}}
                size_t param_ref_pos = 0;
                while ((param_ref_pos = text.find('{' + param_name + '}', param_ref_pos)) != std::string::npos) {
                    std::string param_str;
                    if (param_value.is_string()) {
                        param_str = param_value.get<std::string>();
                    } else {
                        param_str = param_value.dump();
                    }
                    text.replace(param_ref_pos, param_name.length() + 2, param_str);
                    param_ref_pos += param_str.length();
                }
                replacement = text;
            }
        } else if (modifier[0] == '!') {
            // 默认值比较：{param_name:!default_value?text}
            size_t question_pos = modifier.find('?');
            if (question_pos != std::string::npos) {
                std::string default_value = modifier.substr(1, question_pos - 1);
                std::string text = modifier.substr(question_pos + 1);

                if (param_exists && !param_value.is_null()) {
                    std::string current_value;
                    if (param_value.is_string()) {
                        current_value = param_value.get<std::string>();
                    } else {
                        current_value = param_value.dump();
                    }

                    if (current_value != default_value) {
                        replacement = text;
                    }
                }
            }
        } else if (modifier.find("join:") == 0) {
            // 数组连接：{param_name:join:separator}
            std::string separator = modifier.substr(5); // 移除"join:"
            if (param_exists && param_value.is_array()) {
                std::vector<std::string> items;
                for (const auto& item : param_value) {
                    if (item.is_string()) {
                        items.push_back(escapeShellArgument(item.get<std::string>()));
                    } else {
                        items.push_back(item.dump());
                    }
                }
                // 连接数组元素
                for (size_t i = 0; i < items.size(); ++i) {
                    if (i > 0) replacement += separator;
                    replacement += items[i];
                }
            }
        }

        // 替换占位符
        result.replace(pos, end_pos - pos + 1, replacement);
        pos += replacement.length();
    }

    return result;
}

// 辅助函数：验证工具名称格式
bool ToolExecutor::validateToolDefinition(const json& tool_definition, std::string& error_message) const {
    // 1. 检查顶层结构
    if (!tool_definition.is_object()) {
        error_message = "工具定义必须是一个JSON对象";
        return false;
    }

    // 2. 检查必需的'type'字段
    if (!tool_definition.contains("type")) {
        error_message = "工具定义缺失必需的'type'字段";
        return false;
    }

    if (!tool_definition["type"].is_string() || tool_definition["type"].get<std::string>() != "function") {
        error_message = "工具定义的'type'字段必须字符串类型且只能为'function'";
        return false;
    }

    // 3. 检查必需的'function'字段
    if (!tool_definition.contains("function")) {
        error_message = "工具定义缺失必需的'function'字段";
        return false;
    }

    const json& function = tool_definition["function"];
    if (!function.is_object()) {
        error_message = "'function'字段必须是一个JSON对象";
        return false;
    }

    // 4. 检查function中的必需字段

    // 4.1 检查'name'字段
    if (!function.contains("name")) {
        error_message = "function定义缺失必需的'name'字段";
        return false;
    }
    // 检查'name'字段是否为字符串且非空
    if (!function["name"].is_string() || function["name"].get<std::string>().empty()) {
        error_message = "function的'name'字段必须是字符串且非空";
        return false;
    }
    // 检查'name'字段格式是否有效
    if (!validate_tool_name(function["name"].get<std::string>())) {
        error_message = "工具名称格式无效: '" + function["name"].get<std::string>() + "' (必须以字母开头，只能包含字母、数字和下划线)";
        return false;
    }

    // 4.2 检查'description'字段
    if (!function.contains("description")) {
        error_message = "function定义缺失必需的'description'字段";
        return false;
    }

    if (!function["description"].is_string()) {
        error_message = "function的'description'字段必须是字符串";
        return false;
    }

    if (function["description"].get<std::string>().empty()) {
        error_message = "function的'description'字段不能为空";
        return false;
    }

    // 4.3 检查'parameters'字段
    if (function.contains("parameters")) {
        const json& parameters = function["parameters"];
        // 检查parameters是否存在且是一个对象
        if (!parameters.is_object()) {
            error_message = "function的'parameters'字段必须是一个JSON对象";
            return false;
        }
        // 检查parameters是否包含必需的'type'字段
        if (!parameters.contains("type")) {
            error_message = "function的'parameters'字段缺失必需的'type'字段";
            return false;
        }
        // 检查'type'字段是否为字符串且值为'object'
        if (!parameters["type"].is_string() || parameters["type"].get<std::string>() != "object") {
            error_message = "parameters的'type'字段必须为'object'";
            return false;
        }
        // 检查properties字段是否存在
        if (!parameters.contains("properties"))
        {
            error_message = "'parameters'字段缺失必需的'properties'字段";
            return false;
        }
        // 检查properties字段是否为一个对象
        if (!parameters["properties"].is_object()) {
            error_message = "parameters的'properties'字段必须是一个JSON对象";
            return false;
        }
        // 检查parameters是否包含'required'字段
        if (!parameters.contains("required")) {
            error_message = "'parameters'字段缺失必须的'required'字段";
            return false;
        }
        // 检查required字段
        if (!parameters["required"].is_array()) {
            error_message = "parameters的'required'字段必须是一个数组";
            return false;
        }
        // 检查required数组中的每个元素都是字符串
        for (const auto& req : parameters["required"]) {
            if (!req.is_string()) {
                error_message = "parameters的'required'数组中的元素必须是字符串";
                return false;
            }
        }
    }

    // 5. 对于外部工具，检查executable字段
    if (tool_definition.contains("executable")) {
        if (!tool_definition["executable"].is_string()) {
            error_message = "'executable'字段必须是字符串";
            return false;
        }

        std::string executable = tool_definition["executable"].get<std::string>();
        if (executable.empty()) {
            error_message = "'executable'字段不能为空";
            return false;
        }
    }

    // 6. 检查平台特定的executable字段
    if (tool_definition.contains("executable_windows")) {
        if (!tool_definition["executable_windows"].is_string()) {
            error_message = "'executable_windows'字段必须是字符串";
            return false;
        }
    }

    if (tool_definition.contains("executable_linux")) {
        if (!tool_definition["executable_linux"].is_string()) {
            error_message = "'executable_linux'字段必须是字符串";
            return false;
        }
    }

    // 7. 检查command_template字段
    if (tool_definition.contains("command_template")) {
        if (!tool_definition["command_template"].is_string()) {
            error_message = "'command_template'字段必须是字符串";
            return false;
        }
    }

    return true;
}
