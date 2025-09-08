#include "qcopilot_builtin_tools.h"
#include "qcopilot_utils.h"

namespace BuiltinTools {

// 获取内置工具的定义
std::vector<ToolDefinition> getBuiltinToolDefinitions() {
    std::vector<ToolDefinition> definitions;

    // 时间工具
    definitions.push_back(TimeTools::getGetCurrentTimeDefinition());

    // 数学工具
    definitions.push_back(MathTools::getCalculateDefinition());

    // 文件工具
    definitions.push_back(FileTools::getReadTextFileDefinition());
    definitions.push_back(FileTools::getWriteTextFileDefinition());
    definitions.push_back(FileTools::getValidateUtf8FileDefinition());

    // 系统工具
    definitions.push_back(SystemTools::getListDirectoryDefinition());
    definitions.push_back(SystemTools::getPathStatDefinition());
    definitions.push_back(SystemTools::getGrepDefinition());
    definitions.push_back(SystemTools::getGlobDefinition());

    return definitions;
}

// 获取内置工具的执行器函数映射
std::map<std::string, ToolFunction> getBuiltinToolFunctions() {
    std::map<std::string, ToolFunction> functions;

    // 从工具定义中获取名称，确保一致性
    auto definitions = getBuiltinToolDefinitions();

    for (const auto& def : definitions) {
        const std::string& name = def.name;

        // 根据工具名称映射到对应的执行函数
        if (name == "get_current_time") {
            functions[name] = [](const json& args) {
                return TimeTools::executeGetCurrentTime(args);
            };
        }
        else if (name == "calculate") {
            functions[name] = [](const json& args) {
                return MathTools::executeCalculate(args);
            };
        }
        else if (name == "read_text_file") {
            functions[name] = [](const json& args) {
                return FileTools::executeReadTextFile(args);
            };
        }
        else if (name == "write_text_file") {
            functions[name] = [](const json& args) {
                return FileTools::executeWriteTextFile(args);
            };
        }
        else if (name == "validate_utf8_file") {
            functions[name] = [](const json& args) {
                return FileTools::executeValidateUtf8File(args);
            };
        }
        else if (name == "list_directory") {
            functions[name] = [](const json& args) {
                return SystemTools::executeListDirectory(args);
            };
        }
        else if (name == "path_stat") {
            functions[name] = [](const json& args) {
                return SystemTools::executePathStat(args);
            };
        }
        else if (name == "grep") {
            functions[name] = [](const json& args) {
                return SystemTools::executeGrep(args);
            };
        }
        else if (name == "glob") {
            functions[name] = [](const json& args) {
                return SystemTools::executeGlob(args);
            };
        }
        else {
            LOG_WRN("Unknown builtin tool encountered: %s", name.c_str());
        }
    }

    return functions;
}

} // namespace BuiltinTools
