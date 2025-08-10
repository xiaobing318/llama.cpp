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

    // Register calculate
    tools["calculate"] = [this](const json& args) {
        return executeCalculate(args);
    };

    // Register read_file
    tools["read_file"] = [this](const json& args) {
        return executeReadFile(args);
    };

    // Register write_file
    tools["write_file"] = [this](const json& args) {
        return executeWriteFile(args);
    };

    // Register list_files
    tools["list_files"] = [this](const json& args) {
        return executeListFiles(args);
    };
}

void ToolExecutor::registerTool(const json& tool_definition) {
    if (!tool_definition.contains("function")) {
        LOG_ERR("Tool definition missing 'function' field\n");
        return;
    }

    json function = tool_definition["function"];
    std::string name = function.value("name", "");

    if (name.empty() || !validate_tool_name(name)) {
        LOG_ERR("Invalid tool name: %s\n", name.c_str());
        return;
    }

    tool_definitions[name] = tool_definition;
    LOG_INF("Registered tool: %s\n", name.c_str());
}

// tool_executor.cpp

json ToolExecutor::execute(const std::string& name, const json& arguments) {
    auto it = tools.find(name);
    if (it == tools.end()) {
        return json{
            {"error", "Tool not found: " + name},
            {"success", false}
        };
    }

    try {
        // Validate arguments if schema exists
        // 原来：if (tool_definitions.contains(name)) {
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

        // Execute the tool
        return it->second(arguments);

    } catch (const std::exception& e) {
        LOG_ERR("Tool execution failed: %s\n", e.what());
        return json{
            {"error", e.what()},
            {"success", false}
        };
    }
}


bool ToolExecutor::hasTool(const std::string& name) const {
    return tools.find(name) != tools.end();
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
