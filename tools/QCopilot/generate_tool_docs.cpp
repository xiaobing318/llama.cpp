#include "qcopilot_executor.h"
#include "qcopilot_utils.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using json = nlohmann::ordered_json;

namespace {

struct CLIOptions {
    std::string config_path = "QCopilotConfig.json";
    std::string output_path = "ToolDefinitions.md";
    bool config_explicit = false;
    bool show_help = false;
};

void print_usage(const char *program) {
    std::cout << "Usage: " << program << " [--config <path>] [--output <path>]" << '\n'
              << "Generate a Markdown document describing registered QCopilot tools." << '\n'
              << '\n'
              << "Options:" << '\n'
              << "  --config, -c <path>   QCopilot config JSON to load external tools (default: QCopilotConfig.json if present)" << '\n'
              << "  --output, -o <path>   Destination Markdown file (default: ToolDefinitions.md)" << '\n'
              << "  --help, -h            Show this help and exit" << std::endl;
}

bool parse_cli(int argc, char **argv, CLIOptions &opts) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            opts.show_help = true;
            return true;
        } else if (arg == "--config" || arg == "-c") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for " << arg << std::endl;
                return false;
            }
            opts.config_path = argv[++i];
            opts.config_explicit = true;
        } else if (arg == "--output" || arg == "-o") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for " << arg << std::endl;
                return false;
            }
            opts.output_path = argv[++i];
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            return false;
        }
    }
    return true;
}

std::string current_time_iso8601() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
#if defined(_WIN32)
    localtime_s(&tm, &time_t);
#else
    localtime_r(&time_t, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string tool_name_from_definition(const json &definition) {
    if (!definition.contains("function")) {
        return std::string("<unknown>");
    }
    const auto &fn = definition["function"];
    if (fn.contains("name") && fn["name"].is_string()) {
        return fn["name"].get<std::string>();
    }
    return std::string("<unknown>");
}

void emit_executable_section(std::ostream &os, const json &definition) {
    std::vector<std::string> lines;
    auto append_line = [&](const char *key) {
        if (definition.contains(key) && definition[key].is_string() && !definition[key].get<std::string>().empty()) {
            lines.emplace_back(std::string("- ") + key + ": `" + definition[key].get<std::string>() + "`");
        }
    };
    append_line("executable_windows");
    append_line("executable_linux");
    append_line("executable_macos");
    append_line("executable_generic");

    if (!lines.empty()) {
        os << "**Executables:**\n";
        for (const auto &line : lines) {
            os << line << '\n';
        }
    } else {
        os << "**Executables:** _(none specified)_\n";
    }

    if (definition.contains("command_template") && definition["command_template"].is_string()) {
        os << "- Command template: `" << definition["command_template"].get<std::string>() << "`\n";
    } else {
        os << "- Command template: _stdin JSON payload (no template provided)_\n";
    }

    if (definition.contains("timeout_ms")) {
        os << "- timeout_ms: `" << definition["timeout_ms"].dump() << "`\n";
    }

    os << '\n';
}

void emit_tool_markdown(std::ostream &os, const json &definition, bool is_builtin) {
    json sanitized = definition;
    if (sanitized.contains("_kind")) {
        sanitized.erase("_kind");
    }

    const auto &fn = sanitized["function"];
    const std::string name = tool_name_from_definition(definition);
    os << "### " << name << " (" << (is_builtin ? "builtin" : "external") << ")\n\n";

    if (fn.contains("description") && fn["description"].is_string()) {
        os << "**Description:** " << fn["description"].get<std::string>() << "\n\n";
    }

    if (!is_builtin) {
        emit_executable_section(os, definition);
    }

    if (fn.contains("parameters")) {
        os << "**Parameters schema:**\n\n```json\n" << fn["parameters"].dump(2) << "\n```\n\n";
    } else {
        os << "_Parameters: none._\n\n";
    }

    os << "**Tool definition JSON:**\n\n```json\n" << sanitized.dump(2) << "\n```\n\n";
}

} // namespace

int main(int argc, char **argv) {
    CLIOptions options;
    if (!parse_cli(argc, argv, options)) {
        print_usage(argv[0]);
        return 1;
    }
    if (options.show_help) {
        print_usage(argv[0]);
        return 0;
    }

    common_init();
    Logger::set_level(LogLevel::WARN);

    ToolExecutor executor;
    std::vector<std::string> registration_errors;

    bool external_tools_loaded = false;
    {
        std::ifstream config_stream(options.config_path);
        if (config_stream.is_open()) {
            try {
                json config_json;
                config_stream >> config_json;
                if (config_json.contains("tools") && config_json["tools"].is_array()) {
                    for (const auto &tool : config_json["tools"]) {
                        if (!executor.registerExternalTools(tool)) {
                            std::string name = tool.contains("function") && tool["function"].contains("name") && tool["function"]["name"].is_string()
                                ? tool["function"]["name"].get<std::string>()
                                : std::string("<unnamed>");
                            registration_errors.emplace_back("Failed to register external tool: " + name);
                        }
                    }
                    external_tools_loaded = true;
                }
            } catch (const std::exception &e) {
                std::cerr << "Failed to parse config '" << options.config_path << "': " << e.what() << std::endl;
                return 1;
            }
        } else if (options.config_explicit) {
            std::cerr << "Unable to open config file: " << options.config_path << std::endl;
            return 1;
        }
    }

    json all_tools = executor.getAllToolsDefinitions();
    std::vector<json> builtin_tools;
    std::vector<json> external_tools;
    builtin_tools.reserve(all_tools.size());
    external_tools.reserve(all_tools.size());

    for (const auto &tool : all_tools) {
        const std::string kind = tool.value("_kind", std::string("builtin"));
        if (kind == "external") {
            external_tools.push_back(tool);
        } else {
            builtin_tools.push_back(tool);
        }
    }

    auto sorter = [](const json &lhs, const json &rhs) {
        return tool_name_from_definition(lhs) < tool_name_from_definition(rhs);
    };
    std::sort(builtin_tools.begin(), builtin_tools.end(), sorter);
    std::sort(external_tools.begin(), external_tools.end(), sorter);

    std::ostringstream markdown;
    markdown << "# QCopilot Tool Definitions\n\n";
    markdown << "Generated at " << current_time_iso8601() << "\n\n";

    if (external_tools_loaded) {
        markdown << "Config source: `" << options.config_path << "`\n\n";
    }

    markdown << "## Built-in Tools\n\n";
    if (builtin_tools.empty()) {
        markdown << "_No built-in tools registered._\n\n";
    } else {
        for (const auto &tool : builtin_tools) {
            emit_tool_markdown(markdown, tool, true);
        }
    }

    markdown << "## External Tools\n\n";
    if (external_tools.empty()) {
        markdown << "_No external tools registered._\n\n";
    } else {
        for (const auto &tool : external_tools) {
            emit_tool_markdown(markdown, tool, false);
        }
    }

    if (!registration_errors.empty()) {
        markdown << "## Registration Warnings\n\n";
        for (const auto &err : registration_errors) {
            markdown << "- " << err << "\n";
        }
        markdown << '\n';
    }

    std::ofstream output_file(options.output_path, std::ios::out | std::ios::trunc);
    if (!output_file.is_open()) {
        std::cerr << "Failed to open output file: " << options.output_path << std::endl;
        return 1;
    }
    output_file << markdown.str();
    output_file.close();

    std::cout << "Tool definitions written to " << options.output_path << std::endl;
    if (!registration_errors.empty()) {
        std::cerr << "Some external tools failed to register. See 'Registration Warnings' section in the output." << std::endl;
        return 2;
    }

    return 0;
}
