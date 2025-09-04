// 1. 本模块的头文件必须第一个包含（验证头文件自包含性）
#include "qcopilot_builtin_tools.h"

// 2. 相关项目头文件
#include "qcopilot_executor.h"

// 3. C++标准库头文件
#include <cmath>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <functional>
#include <map>
#include <set>
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace BuiltinTools {

// 获取内置工具的定义
std::vector<ToolDefinition> getBuiltinToolDefinitions() {
    std::vector<ToolDefinition> definitions;
    
    // get_current_time tool
    definitions.push_back({
        "get_current_time",
        {
            {"type", "function"},
            {"function", {
                {"name", "get_current_time"},
                {"description", "Retrieves the current system time in various formats for timestamping, logging, and temporal data processing. Essential for applications requiring precise time tracking, data synchronization, and audit trails. Use cases include: 1) Adding creation timestamps to log entries and data records 2) Generating time-based unique identifiers for files and database entries 3) Calculating elapsed time intervals and performance metrics 4) Synchronizing distributed systems and coordinating batch processing jobs 5) Creating temporal metadata for GIS datasets and scientific measurements. The tool supports multiple output formats to accommodate different systems and use cases."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"format", {{"type", "string"}, {"description", "Time format specification: 'ISO8601' for standard international format (YYYY-MM-DDTHH:MM:SS, ideal for databases and APIs), 'unix' for Unix timestamp (seconds since epoch, optimal for calculations and system interoperability), or 'default' for human-readable format (YYYY-MM-DD HH:MM:SS, suitable for user interfaces and reports). Defaults to ISO8601."}}},
                        {"timezone", {{"type", "string"}, {"description", "Timezone specification: 'local' for system local timezone (appropriate for user-facing applications and local file processing), or 'UTC' for Coordinated Universal Time (recommended for distributed systems, logging, and international data exchange). Defaults to local timezone."}}}
                    }}
                }}
            }}
        }
    });
    
    // calculate tool
    definitions.push_back({
        "calculate",
        {
            {"type", "function"},
            {"function", {
                {"name", "calculate"},
                {"description", "Advanced mathematical expression evaluator supporting comprehensive arithmetic operations, mathematical functions, and constants. Designed for scientific computing, engineering calculations, statistical analysis, and geometric computations. Capabilities include: 1) Basic arithmetic operators (+, -, *, /, %) with proper precedence handling 2) Comprehensive mathematical functions: trigonometric (sin, cos, tan, asin, acos, atan), hyperbolic (sinh, cosh, tanh), logarithmic (log, ln), exponential (exp), power (pow), and utility functions (abs, sqrt, floor, ceil, round) 3) Mathematical constants (pi ≈ 3.14159, e ≈ 2.71828) 4) Parentheses for expression grouping and precedence control 5) Scientific notation support (e.g., 1.5e-3) 6) Error handling for domain violations (sqrt of negative, division by zero). Perfect for coordinate transformations, area calculations, statistical computations, financial modeling, and physics simulations."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"expression", {{"type", "string"}, {"description", "Mathematical expression string supporting operators, functions, constants, and parentheses. Examples: 'sin(pi/4)' (trigonometry), 'sqrt(pow(3,2) + pow(4,2))' (Pythagorean theorem), '2*pi*radius' (circumference), 'log(100)/ln(10)' (logarithms), 'abs(-5) + floor(3.7)' (utility functions), '(1+0.05)^12' (compound interest using pow(1+0.05,12))"}}}
                    }},
                    {"required", {"expression"}}
                }}
            }}
        }
    });
    
    // read_file tool
    definitions.push_back({
        "read_file",
        {
            {"type", "function"},
            {"function", {
                {"name", "read_file"},
                {"description", "Comprehensive file reading utility for loading and processing various file types including text documents, configuration files, data files, and source code. Essential for data analysis workflows, configuration management, log analysis, and content processing. Primary use cases: 1) Loading configuration files (.ini, .conf, .json, .yaml) for application settings 2) Reading data files (.csv, .tsv, .txt) for analysis and processing 3) Accessing log files (.log) for debugging and monitoring 4) Loading source code (.py, .js, .cpp, .h) for analysis and documentation 5) Reading project files (.qgs, .qgz, .xml) for metadata extraction 6) Processing documentation (.md, .rst, .txt) for content management. Supports multiple text encodings and provides robust error handling for file system operations."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"path", {{"type", "string"}, {"description", "Complete file path (absolute or relative) to the target file. Supports various formats including './data/config.json', '/home/user/logs/app.log', 'C:\\\\Users\\\\Name\\\\Documents\\\\file.txt', '../project/src/main.py'. Path separators are automatically handled across platforms."}}},
                        {"encoding", {{"type", "string"}, {"description", "Text encoding specification for proper character interpretation: 'utf-8' (default, recommended for modern applications and international text), 'ascii' (for legacy English-only files), 'gbk' (for Chinese Windows systems), 'iso-8859-1' (for Western European text), 'cp1252' (Windows Western encoding). Auto-detects if not specified."}}}
                    }},
                    {"required", {"path"}}
                }}
            }}
        }
    });
    
    // write_file tool
    definitions.push_back({
        "write_file",
        {
            {"type", "function"},
            {"function", {
                {"name", "write_file"},
                {"description", "Versatile file writing utility for creating and modifying various file types with support for both overwrite and append modes. Critical for code generation, data export, configuration management, and automated report generation. Key applications include: 1) Generating source code files (.py, .js, .cpp, .sql) for automated development workflows 2) Creating and updating configuration files (.ini, .conf, .json, .yaml) for application settings 3) Exporting processed data (.csv, .tsv, .txt, .xml) from analysis pipelines 4) Writing batch scripts (.sh, .bat, .ps1) for system automation 5) Maintaining log files (.log) with append mode for continuous monitoring 6) Generating documentation (.md, .html, .rst) and reports 7) Creating temporary files for inter-process communication. Features automatic directory creation and robust error handling."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"path", {{"type", "string"}, {"description", "Target file path (absolute or relative) where content will be written. Automatically creates parent directories if they don't exist. Examples: './output/results.csv', '/var/log/application.log', 'C:\\\\Projects\\\\scripts\\\\automation.py', '../config/settings.json'. Cross-platform path handling included."}}},
                        {"content", {{"type", "string"}, {"description", "Content to write to the file. Supports various formats including plain text, structured data (JSON, CSV, XML), source code, configuration syntax, and binary data encoded as text. Handles newlines and special characters appropriately."}}},
                        {"append", {{"type", "boolean"}, {"description", "Write mode selection: false (overwrite mode, default) completely replaces existing file content, ideal for generating new files and configuration updates; true (append mode) adds content to existing file end, perfect for log files, data collection, and incremental updates."}}}
                    }},
                    {"required", {"path", "content"}}
                }}
            }}
        }
    });
    
    
    // Claude Code风格工具
    
    // glob tool
    definitions.push_back({
        "glob",
        {
            {"type", "function"},
            {"function", {
                {"name", "glob"},
                {"description", "Advanced file system pattern matching tool for efficient file discovery and batch operations. Implements powerful glob patterns with support for wildcards, recursive directory traversal, and complex matching rules. Essential for DevOps automation, code analysis, build systems, and data processing pipelines. Primary use cases: 1) Bulk file operations and batch processing (find all .jpg files for image processing) 2) Code repository analysis and metrics collection (locate all source files matching patterns) 3) Build system file discovery (find compilation targets, test files, documentation) 4) Cleanup and maintenance tasks (identify temporary files, old backups, unused assets) 5) Deployment and packaging (collect files for distribution, exclude patterns) 6) Security scanning (find files with specific extensions or naming patterns). Supports both simple wildcards and advanced recursive patterns for comprehensive file system exploration."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"pattern", {{"type", "string"}, {"description", "Glob pattern string supporting wildcards and recursive matching. Examples: '*.txt' (all text files in current directory), '**/*.py' (all Python files recursively), 'test_*.js' (test files starting with 'test_'), 'src/**/*.{cpp,h}' (C++ source and headers in src tree), '*.log' (log files), 'backup_*_2023.*' (specific backup files), '**/Dockerfile' (Docker files anywhere in tree). Patterns are case-sensitive by default."}}},
                        {"path", {{"type", "string"}, {"description", "Starting directory path for the search operation. Can be absolute ('/home/user/project') or relative ('./src', '../data'). Defaults to current working directory ('.') if not specified. The search will begin from this location and follow the pattern's directory traversal rules."}}}
                    }},
                    {"required", {"pattern"}}
                }}
            }}
        }
    });
    
    // grep tool
    definitions.push_back({
        "grep",
        {
            {"type", "function"},
            {"function", {
                {"name", "grep"},
                {"description", "Powerful text search and content analysis tool implementing regex-based pattern matching across files and directories. Inspired by the Unix grep utility but enhanced for modern development workflows. Essential for code analysis, log mining, documentation search, and data extraction tasks. Key applications: 1) Code navigation and refactoring (find function definitions, variable usage, API calls) 2) Log analysis and debugging (search error patterns, trace execution flows) 3) Configuration auditing (locate settings, validate parameters) 4) Documentation search (find examples, API references, explanations) 5) Data mining and extraction (parse structured text, extract metrics) 6) Security analysis (search for sensitive patterns, credential leaks) 7) Code quality assessment (find TODO comments, deprecated usage). Supports advanced regex patterns, case sensitivity control, and flexible output formatting."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"pattern", {{"type", "string"}, {"description", "Search pattern supporting both literal text and regular expressions. Examples: 'function main' (literal text), 'ERROR|FATAL' (regex alternation), '^class\\s+\\w+' (regex for class definitions), '\\b\\d{3}-\\d{3}-\\d{4}\\b' (phone numbers), 'TODO.*' (comments), 'import\\s+['\\\"].*['\\\"]' (import statements). Use regex metacharacters for advanced pattern matching."}}},
                        {"path", {{"type", "string"}, {"description", "Target path for search operation - can be a specific file ('/path/to/file.txt'), directory ('/path/to/project'), or use current directory if omitted ('.'). When targeting directories, recursively searches all contained files."}}},
                        {"include", {{"type", "string"}, {"description", "File filter pattern to limit search scope to specific file types. Examples: '*.cpp' (C++ files only), '*.{js,ts}' (JavaScript/TypeScript), '*.log' (log files), '*.py' (Python files), 'test_*.py' (test files). Helps focus search and improve performance."}}},
                        {"case_sensitive", {{"type", "boolean"}, {"description", "Case sensitivity control: false (default, case-insensitive search, finds 'Error', 'ERROR', 'error') or true (exact case matching required). Case-insensitive is often preferred for general searches."}}},
                        {"line_numbers", {{"type", "boolean"}, {"description", "Include line numbers in results: true (default, shows file:line format for easy navigation) or false (shows only matching content). Line numbers are essential for code navigation and debugging."}}}
                    }},
                    {"required", {"pattern"}}
                }}
            }}
        }
    });
    
    // multiedit tool
    definitions.push_back({
        "multiedit",
        {
            {"type", "function"},
            {"function", {
                {"name", "multiedit"},
                {"description", "Advanced batch text editing tool for performing multiple find-and-replace operations atomically on a single file. Designed for complex refactoring tasks, configuration updates, and systematic code transformations. Processes all edits sequentially within a single transaction, ensuring consistency and enabling rollback on errors. Critical for: 1) Large-scale refactoring operations (rename variables, update function signatures, modify API calls) 2) Configuration file updates (change multiple settings, update connection strings, modify parameters) 3) Code modernization (update deprecated syntax, migrate to new APIs, standardize formatting) 4) Data transformation (normalize values, update formats, standardize conventions) 5) Template processing (replace placeholders, customize configurations, generate variants) 6) Systematic code reviews (fix consistent issues, apply style guidelines). All edits are validated and applied atomically to prevent partial updates and maintain file integrity."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"file_path", {{"type", "string"}, {"description", "Target file path for editing operations. Must be an existing readable file. Examples: './src/main.py', '/etc/nginx/nginx.conf', 'C:\\\\Projects\\\\config\\\\app.json'. File is locked during editing to prevent concurrent modifications."}}},
                        {"edits", {{"type", "array"}, {"description", "Array of edit operations to perform sequentially. Each operation contains old_string, new_string, and optional replace_all fields."}}}
                    }},
                    {"required", {"file_path", "edits"}}
                }}
            }}
        }
    });
    
    // edit tool
    definitions.push_back({
        "edit",
        {
            {"type", "function"},
            {"function", {
                {"name", "edit"},
                {"description", "Streamlined single-operation text replacement tool for quick file modifications and targeted content updates. Optimized for simple, focused edits where precision and speed are essential. Ideal for: 1) Bug fixes and quick corrections (fix typos, correct values, update single parameters) 2) Simple refactoring tasks (rename single variable, update function name, change import path) 3) Configuration adjustments (modify single setting, update URL, change version number) 4) Content updates (fix documentation, update comments, modify strings) 5) Template customization (replace placeholder values, update titles) 6) Quick patches (emergency fixes, temporary modifications). Provides precise control over replacement scope and maintains file integrity with atomic operations. Complementary to multiedit for scenarios requiring single, targeted changes."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"file_path", {{"type", "string"}, {"description", "Path to the target file for editing. Must be an existing, writable file. Supports both absolute ('/home/user/project/file.py') and relative paths ('./src/config.js', '../docs/readme.md'). Cross-platform path handling included."}}},
                        {"old_string", {{"type", "string"}, {"description", "Exact text content to locate and replace. Case-sensitive matching. Must be unique enough to avoid unintended replacements. Examples: 'DEBUG = True', 'localhost:3000', 'version 1.0.0', 'TODO: implement this'. Include sufficient context for precision."}}},
                        {"new_string", {{"type", "string"}, {"description", "Replacement text to substitute for the matched content. Can be empty string for text removal. Examples: 'DEBUG = False', 'api.production.com', 'version 2.1.0', 'COMPLETED: feature implemented'. Preserves surrounding formatting."}}},
                        {"replace_all", {{"type", "boolean"}, {"description", "Replacement scope control: false (default, replaces only the first match, safer for unique content) or true (replaces all matching occurrences, useful for global text changes like renaming throughout file). Choose based on intended scope."}}}
                    }},
                    {"required", {"file_path", "old_string", "new_string"}}
                }}
            }}
        }
    });
    
    // bash tool
    definitions.push_back({
        "bash",
        {
            {"type", "function"},
            {"function", {
                {"name", "bash"},
                {"description", "Comprehensive system command execution interface providing access to shell/command-line operations across platforms. Enables automation, system administration, development workflow integration, and infrastructure management. Essential for DevOps pipelines, build automation, system monitoring, and development tool integration. Primary applications: 1) File system operations (create directories, move files, set permissions, archive data) 2) Process management (start/stop services, monitor resource usage, manage background tasks) 3) Development workflow automation (run build scripts, execute tests, deploy applications, manage dependencies) 4) System information gathering (check disk space, monitor performance, query system status) 5) Version control operations (git commands, repository management, branch operations) 6) Network operations (connectivity tests, file transfers, API calls) 7) Database operations (backups, imports, maintenance scripts). Includes safety measures and timeout controls for reliable automation. Cross-platform compatibility with Windows cmd/PowerShell and Unix shell environments."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"command", {{"type", "string"}, {"description", "Shell command to execute with full argument list. Examples: 'ls -la /home/user' (list directory), 'git status --porcelain' (check git status), 'npm install --production' (install dependencies), 'docker ps -a' (list containers), 'python -m pytest tests/' (run tests), 'curl -s https://api.example.com/health' (health check). Use appropriate syntax for target platform (Unix: ls, ps, grep; Windows: dir, tasklist, findstr)."}}},
                        {"timeout", {{"type", "number"}, {"description", "Maximum execution time in milliseconds before command termination. Defaults to 30000 (30 seconds). Use higher values for long-running operations like builds or data processing. Examples: 60000 for compile operations, 300000 for large file transfers, 10000 for quick queries."}}},
                        {"description", {{"type", "string"}, {"description", "Human-readable description of the command's purpose for logging and audit trails. Examples: 'Build production assets', 'Check system disk usage', 'Deploy to staging server', 'Run integration tests'. Helps with debugging and operation tracking."}}}
                    }},
                    {"required", {"command"}}
                }}
            }}
        }
    });
    
    // list_directory tool (enhanced replacement for removed list_files)
    definitions.push_back({
        "list_directory", 
        {
            {"type", "function"},
            {"function", {
                {"name", "list_directory"},
                {"description", "Enhanced directory listing and file system exploration tool providing detailed file and directory information with filtering capabilities. More comprehensive than basic glob patterns, designed for file system analysis, cleanup operations, and inventory management. Essential for: 1) Project structure analysis and documentation 2) File system auditing and cleanup (find large files, old files, duplicates) 3) Build system preparation (identify source files, check dependencies) 4) Backup and archival operations (catalog files, verify completeness) 5) Security scanning (check permissions, identify sensitive files) 6) Development environment setup (verify installations, check configurations). Provides rich metadata including sizes, timestamps, permissions, and file types with flexible filtering options."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"path", {{"type", "string"}, {"description", "Directory path to list. Supports both absolute ('/home/user/project') and relative paths ('./src', '../data'). Defaults to current directory ('.') if not specified."}}},
                        {"recursive", {{"type", "boolean"}, {"description", "Traversal depth: false (current directory only, faster for shallow inspection) or true (include all subdirectories, comprehensive for full analysis). Default false."}}},
                        {"show_hidden", {{"type", "boolean"}, {"description", "Hidden file visibility: false (skip files starting with '.', cleaner output) or true (include hidden files and system files, complete inventory). Default false."}}},
                        {"file_types", {{"type", "string"}, {"description", "File type filter using extensions: 'all' (no filtering), 'source' (code files: .py, .js, .cpp, .h), 'data' (data files: .csv, .json, .xml), 'docs' (documentation: .md, .txt, .pdf), or specific extensions like 'py,js,cpp'. Helps focus on relevant files."}}},
                        {"size_info", {{"type", "boolean"}, {"description", "Include file size information: true (show file sizes, useful for cleanup and analysis) or false (names only, faster for simple listings). Default true."}}}
                    }}
                }}
            }}
        }
    });
    
    // file_stats tool  
    definitions.push_back({
        "file_stats",
        {
            {"type", "function"},
            {"function", {
                {"name", "file_stats"},
                {"description", "Comprehensive file and directory metadata analysis tool providing detailed information about file system objects. Essential for system administration, security auditing, backup verification, and development workflows. Retrieves complete file attributes including size, timestamps, permissions, ownership, and content type detection. Primary use cases: 1) Security auditing (check permissions, ownership, access patterns) 2) Backup and synchronization (verify file integrity, detect changes) 3) Performance analysis (identify large files, analyze disk usage) 4) Development debugging (check file modifications, verify builds) 5) System monitoring (track file system changes, detect anomalies) 6) Compliance reporting (document file attributes, access controls). Provides both human-readable and machine-parseable output formats."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"path", {{"type", "string"}, {"description", "Path to file or directory for analysis. Can be absolute ('/var/log/app.log') or relative ('./config.json'). For directories, provides summary statistics of contained files."}}},
                        {"detailed", {{"type", "boolean"}, {"description", "Information depth: false (basic info - size, modified time, type) or true (comprehensive - permissions, ownership, checksums, content analysis). Default false for performance."}}},
                        {"checksum", {{"type", "boolean"}, {"description", "Include file integrity checksums (MD5/SHA) for verification and change detection. Useful for security auditing and backup verification. Default false due to computation overhead."}}}
                    }},
                    {"required", {"path"}}
                }}
            }}
        }
    });
    
    return definitions;
}

// 获取内置工具的执行器函数映射
std::map<std::string, ToolFunction> getBuiltinToolFunctions() {
    std::map<std::string, ToolFunction> functions;

    functions["get_current_time"] = [](const json& args) {
        return executeGetCurrentTime(args);
    };
    
    functions["calculate"] = [](const json& args) {
        return executeCalculate(args);
    };
    
    functions["read_file"] = [](const json& args) {
        return executeReadFile(args);
    };
    
    functions["write_file"] = [](const json& args) {
        return executeWriteFile(args);
    };
    
    functions["glob"] = [](const json& args) {
        return executeGlob(args);
    };
    
    functions["grep"] = [](const json& args) {
        return executeGrep(args);
    };
    
    functions["multiedit"] = [](const json& args) {
        return executeMultiEdit(args);
    };
    
    functions["edit"] = [](const json& args) {
        return executeEdit(args);
    };
    
    functions["bash"] = [](const json& args) {
        return executeBash(args);
    };
    
    functions["list_directory"] = [](const json& args) {
        return executeListDirectory(args);
    };
    
    functions["file_stats"] = [](const json& args) {
        return executeFileStats(args);
    };
    
    return functions;
}

/********各个内置工具使用到的函数********/

// 前向声明内部函数 
double evaluateExpression(const std::string& expr);
double parseExpression(const std::string& expr, size_t& pos);
double parseTerm(const std::string& expr, size_t& pos);
double parseFactor(const std::string& expr, size_t& pos);
double parseFunction(const std::string& funcName, const std::string& expr, size_t& pos);
void skipWhitespace(const std::string& expr, size_t& pos);
bool isFunction(const std::string& name);

// 计算工具将会用到的常量
const double PI = 3.14159265358979323846;
const double E = 2.71828182845904523536;

// 计算表达式的主函数
double evaluateExpression(const std::string& expr) {
    if (expr.empty()) {
        throw std::runtime_error("Empty expression");
    }
    
    size_t pos = 0;
    double result = parseExpression(expr, pos);
    
    // 检查是否还有未处理的字符
    skipWhitespace(expr, pos);
    if (pos < expr.length()) {
        throw std::runtime_error("Unexpected characters at end of expression: " + expr.substr(pos));
    }
    
    return result;
}
// 解析表达式（处理 +, - 运算符）
double parseExpression(const std::string& expr, size_t& pos) {
    double result = parseTerm(expr, pos);
    
    while (pos < expr.length()) {
        skipWhitespace(expr, pos);
        
        if (pos < expr.length() && (expr[pos] == '+' || expr[pos] == '-')) {
            char op = expr[pos];
            pos++;
            double right = parseTerm(expr, pos);
            
            if (op == '+') {
                result += right;
            } else if (op == '-') {
                result -= right;
            }
        } else {
            break;
        }
    }
    
    return result;
}
// 解析项（处理 *, /, % 运算符）
double parseTerm(const std::string& expr, size_t& pos) {
    double result = parseFactor(expr, pos);
    
    while (pos < expr.length()) {
        skipWhitespace(expr, pos);
        
        if (pos < expr.length() && (expr[pos] == '*' || expr[pos] == '/' || expr[pos] == '%')) {
            char op = expr[pos];
            pos++;
            double right = parseFactor(expr, pos);
            
            if (op == '*') {
                result *= right;
            } else if (op == '/') {
                if (right == 0) {
                    throw std::runtime_error("Division by zero");
                }
                result /= right;
            } else if (op == '%') {
                if (right == 0) {
                    throw std::runtime_error("Modulo by zero");
                }
                result = std::fmod(result, right);
            }
        } else {
            break;
        }
    }
    
    return result;
}
// 解析因子（数字、常量、函数、括号表达式）
double parseFactor(const std::string& expr, size_t& pos) {
    skipWhitespace(expr, pos);
    
    if (pos >= expr.length()) {
        throw std::runtime_error("Unexpected end of expression");
    }
    
    // 处理负号
    if (expr[pos] == '-') {
        pos++;
        return -parseFactor(expr, pos);
    }
    
    // 处理正号
    if (expr[pos] == '+') {
        pos++;
        return parseFactor(expr, pos);
    }
    
    // 处理括号
    if (expr[pos] == '(') {
        pos++; // 跳过 '('
        double result = parseExpression(expr, pos);
        if (pos >= expr.length() || expr[pos] != ')') {
            throw std::runtime_error("Expected ')'");
        }
        pos++; // 跳过 ')'
        return result;
    }
    
    // 解析数字或标识符
    size_t start = pos;
    
    // 检查是否是数学常量或函数
    if (std::isalpha(expr[pos])) {
        while (pos < expr.length() && std::isalnum(expr[pos])) {
            pos++;
        }
        
        std::string identifier = expr.substr(start, pos - start);
        
        // 数学常量
        if (identifier == "pi") return PI;
        else if (identifier == "e") return E;
        
        // 数学函数
        if (isFunction(identifier)) {
            return parseFunction(identifier, expr, pos);
        } else {
            throw std::runtime_error("Unknown identifier: " + identifier);
        }
    }
    
    // 解析数字（包括小数和科学计数法）
    if (std::isdigit(expr[pos]) || expr[pos] == '.') {
        while (pos < expr.length() && 
               (std::isdigit(expr[pos]) || expr[pos] == '.' || 
                expr[pos] == 'e' || expr[pos] == 'E' || 
                expr[pos] == '+' || expr[pos] == '-')) {
            pos++;
        }
        
        std::string numStr = expr.substr(start, pos - start);
        try {
            return std::stod(numStr);
        } catch (const std::exception&) {
            throw std::runtime_error("Invalid number format: " + numStr);
        }
    }
    
    throw std::runtime_error("Unexpected character: " + std::string(1, expr[pos]));
}
// 解析数学函数调用
double parseFunction(const std::string& funcName, const std::string& expr, size_t& pos) {
    // 跳过函数名
    pos += funcName.length();
    
    // 期望左括号
    if (pos >= expr.length() || expr[pos] != '(') {
        throw std::runtime_error("Expected '(' after function name");
    }
    pos++; // 跳过 '('
    
    // pow 函数需要两个参数
    if (funcName == "pow") {
        double arg1 = parseExpression(expr, pos);
        
        // 期望逗号
        if (pos >= expr.length() || expr[pos] != ',') {
            throw std::runtime_error("Expected ',' in pow function");
        }
        pos++; // 跳过 ','
        
        double arg2 = parseExpression(expr, pos);
        
        // 期望右括号
        if (pos >= expr.length() || expr[pos] != ')') {
            throw std::runtime_error("Expected ')' after function arguments");
        }
        pos++; // 跳过 ')'
        
        return std::pow(arg1, arg2);
    } else {
        // 单参数函数
        double arg = parseExpression(expr, pos);
        
        // 期望右括号
        if (pos >= expr.length() || expr[pos] != ')') {
            throw std::runtime_error("Expected ')' after function argument");
        }
        pos++; // 跳过 ')'
        
        // 调用相应的数学函数
        if (funcName == "sin") return std::sin(arg);
        else if (funcName == "cos") return std::cos(arg);
        else if (funcName == "tan") return std::tan(arg);
        else if (funcName == "sqrt") {
            if (arg < 0) throw std::runtime_error("sqrt of negative number");
            return std::sqrt(arg);
        }
        else if (funcName == "log") {
            if (arg <= 0) throw std::runtime_error("log of non-positive number");
            return std::log10(arg);
        }
        else if (funcName == "ln") {
            if (arg <= 0) throw std::runtime_error("ln of non-positive number");
            return std::log(arg);
        }
        else if (funcName == "exp") return std::exp(arg);
        else if (funcName == "abs") return std::abs(arg);
        else if (funcName == "floor") return std::floor(arg);
        else if (funcName == "ceil") return std::ceil(arg);
        else if (funcName == "round") return std::round(arg);
        else if (funcName == "asin") {
            if (arg < -1 || arg > 1) throw std::runtime_error("asin argument out of range [-1,1]");
            return std::asin(arg);
        }
        else if (funcName == "acos") {
            if (arg < -1 || arg > 1) throw std::runtime_error("acos argument out of range [-1,1]");
            return std::acos(arg);
        }
        else if (funcName == "atan") return std::atan(arg);
        else if (funcName == "sinh") return std::sinh(arg);
        else if (funcName == "cosh") return std::cosh(arg);
        else if (funcName == "tanh") return std::tanh(arg);
        else throw std::runtime_error("Unknown function: " + funcName);
    }
}
// 跳过空白字符
void skipWhitespace(const std::string& expr, size_t& pos) {
    while (pos < expr.length() && std::isspace(expr[pos])) {
        pos++;
    }
}
// 检查字符串是否是数学函数
bool isFunction(const std::string& name) {
    static const std::set<std::string> functions = {
        "sin", "cos", "tan", "sqrt", "log", "ln", "exp", "abs", 
        "floor", "ceil", "round", "pow", "asin", "acos", "atan",
        "sinh", "cosh", "tanh"
    };
    return functions.find(name) != functions.end();
}
// 匹配模式
bool matchPattern(const std::string& text, const std::string& pattern) {
    // 简化的模式匹配实现，支持*通配符
    if (pattern == "*") return true;
    
    size_t star_pos = pattern.find('*');
    if (star_pos == std::string::npos) {
        // 没有通配符，直接比较
        return text == pattern;
    }
    
    if (star_pos == 0) {
        // *在开头
        std::string suffix = pattern.substr(1);
        return text.length() >= suffix.length() && 
               text.substr(text.length() - suffix.length()) == suffix;
    } else if (star_pos == pattern.length() - 1) {
        // *在末尾
        std::string prefix = pattern.substr(0, star_pos);
        return text.length() >= prefix.length() &&
               text.substr(0, prefix.length()) == prefix;
    } else {
        // *在中间
        std::string prefix = pattern.substr(0, star_pos);
        std::string suffix = pattern.substr(star_pos + 1);
        return text.length() >= prefix.length() + suffix.length() &&
               text.substr(0, prefix.length()) == prefix &&
               text.substr(text.length() - suffix.length()) == suffix;
    }
}
// 在文件中搜索模式
std::vector<json> searchInFile(
    const std::string& filepath,
    const std::string& pattern, 
    bool case_sensitive,
    bool line_numbers) {
    std::vector<json> matches;
    std::string content;
    
    if (!read_file_content(filepath, content)) {
        return matches;
    }
    
    std::string search_content = content;
    std::string search_pattern = pattern;
    
    if (!case_sensitive) {
        std::transform(search_content.begin(), search_content.end(), search_content.begin(), ::tolower);
        std::transform(search_pattern.begin(), search_pattern.end(), search_pattern.begin(), ::tolower);
    }
    
    std::istringstream iss(content);
    std::istringstream search_iss(search_content);
    std::string line, search_line;
    int line_num = 1;
    
    while (std::getline(iss, line) && std::getline(search_iss, search_line)) {
        if (search_line.find(search_pattern) != std::string::npos) {
            json match = {
                {"file", filepath},
                {"line_content", line},
                {"success", true}
            };
            
            if (line_numbers) {
                match["line_number"] = line_num;
            }
            
            matches.push_back(match);
        }
        line_num++;
    }
    
    return matches;
}

/********各个内置工具的具体实现函数********/

json executeGetCurrentTime(const json& args) {
    std::string format = args.value("format", "ISO8601");
    std::string timezone = args.value("timezone", "local");

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    // 根据时区参数选择合适的时间转换函数
    std::tm* time_info = nullptr;
    if (timezone == "UTC") {
        time_info = std::gmtime(&time_t);
    } else {
        time_info = std::localtime(&time_t);
    }

    // 错误处理：检查时间转换是否成功
    if (!time_info) {
        return json{
            {"error", "Failed to convert system time"},
            {"success", false}
        };
    }

    std::stringstream ss;
    if (format == "ISO8601") {
        ss << std::put_time(time_info, "%Y-%m-%dT%H:%M:%S");
        // 为 UTC 时间添加 Z 后缀，符合 ISO8601 标准
        if (timezone == "UTC") {
            ss << "Z";
        }
    } else if (format == "unix") {
        ss << time_t;
    } else {
        // default 格式
        ss << std::put_time(time_info, "%Y-%m-%d %H:%M:%S");
    }

    return json{
        {"time", ss.str()},
        {"format", format},
        {"timezone", timezone},
        {"success", true}
    };
}

json executeCalculate(const json& args) {
    std::string expression = args.value("expression", "");

    if (expression.empty()) {
        return json{
            {"error", "Expression is required"},
            {"success", false}
        };
    }

    // 输入验证：检查表达式长度是否合理
    if (expression.length() > 1000) {
        return json{
            {"error", "Expression too long (maximum 1000 characters)"},
            {"success", false}
        };
    }

    // 基本字符验证：确保只包含允许的字符
    for (char c : expression) {
        if (!std::isalnum(c) && !std::isspace(c) && 
            c != '+' && c != '-' && c != '*' && c != '/' && c != '%' &&
            c != '(' && c != ')' && c != '.' && c != ',' && c != '^') {
            return json{
                {"error", std::string("Invalid character in expression: '") + c + "'"},
                {"success", false}
            };
        }
    }

    // 增强的计算器实现 - 支持常见数学运算
    try {
        // 移除所有空格以简化解析
        std::string cleanExpr;
        for (char c : expression) {
            if (!std::isspace(c)) {
                cleanExpr += c;
            }
        }
        
        // 检查清理后的表达式是否为空
        if (cleanExpr.empty()) {
            return json{
                {"error", "Expression contains only whitespace"},
                {"success", false}
            };
        }
        
        // 基本语法检查：检查括号是否匹配
        int parentheses_count = 0;
        for (char c : cleanExpr) {
            if (c == '(') parentheses_count++;
            else if (c == ')') parentheses_count--;
            if (parentheses_count < 0) {
                return json{
                    {"error", "Mismatched parentheses: too many closing parentheses"},
                    {"success", false}
                };
            }
        }
        if (parentheses_count != 0) {
            return json{
                {"error", "Mismatched parentheses: unclosed opening parentheses"},
                {"success", false}
            };
        }
        
        double result = evaluateExpression(cleanExpr);
        
        // 检查结果是否有效
        if (std::isnan(result)) {
            return json{
                {"error", "Invalid mathematical operation resulted in NaN (Not a Number)"},
                {"success", false}
            };
        }
        
        if (std::isinf(result)) {
            return json{
                {"error", "Mathematical operation resulted in infinity"},
                {"success", false}
            };
        }

        return json{
            {"expression", expression},
            {"cleaned_expression", cleanExpr},
            {"result", result},
            {"success", true}
        };

    } catch (const std::runtime_error& e) {
        return json{
            {"error", std::string("Mathematical evaluation error: ") + e.what()},
            {"success", false}
        };
    } catch (const std::exception& e) {
        return json{
            {"error", std::string("Unexpected error: ") + e.what()},
            {"success", false}
        };
    } catch (...) {
        return json{
            {"error", "Unknown error occurred during expression evaluation"},
            {"success", false}
        };
    }
}

json executeReadFile(const json& args) {
    std::string path = args.value("path", "");
    std::string encoding = args.value("encoding", "utf-8");

    if (path.empty()) {
        return json{
            {"error", "Path is required"},
            {"success", false}
        };
    }

    // 路径安全检查：防止路径遍历攻击
    if (path.find("..") != std::string::npos) {
        return json{
            {"error", "Path traversal not allowed"},
            {"success", false}
        };
    }

    // 检查路径长度是否合理
    if (path.length() > 4096) {
        return json{
            {"error", "Path too long (maximum 4096 characters)"},
            {"success", false}
        };
    }

    // 使用 filesystem 库进行更完整的路径和文件检查
    try {
        std::filesystem::path fs_path(path);
        
        // 检查路径是否存在
        if (!std::filesystem::exists(fs_path)) {
            return json{
                {"error", "File or directory not found: " + path},
                {"success", false}
            };
        }
        
        // 检查是否是目录而不是文件
        if (std::filesystem::is_directory(fs_path)) {
            return json{
                {"error", "Path is a directory, not a file: " + path},
                {"success", false}
            };
        }
        
        // 检查是否是常规文件
        if (!std::filesystem::is_regular_file(fs_path)) {
            return json{
                {"error", "Path is not a regular file: " + path},
                {"success", false}
            };
        }
        
        // 获取文件大小并检查是否过大
        auto file_size = std::filesystem::file_size(fs_path);
        const size_t MAX_FILE_SIZE = 100 * 1024 * 1024; // 100MB 限制
        
        if (file_size > MAX_FILE_SIZE) {
            return json{
                {"error", "File too large (maximum 100MB): " + std::to_string(file_size) + " bytes"},
                {"success", false}
            };
        }
        
        // 检查文件权限（是否可读）
        auto perms = std::filesystem::status(fs_path).permissions();
        if ((perms & std::filesystem::perms::owner_read) == std::filesystem::perms::none &&
            (perms & std::filesystem::perms::group_read) == std::filesystem::perms::none &&
            (perms & std::filesystem::perms::others_read) == std::filesystem::perms::none) {
            return json{
                {"error", "File is not readable: " + path},
                {"success", false}
            };
        }
        
        std::string content;
        if (!read_file_content(path, content)) {
            return json{
                {"error", "Failed to read file content: " + path},
                {"success", false}
            };
        }
        
        // 获取文件的实际路径（解析符号链接等）
        std::string canonical_path;
        try {
            canonical_path = std::filesystem::canonical(fs_path).string();
        } catch (const std::exception&) {
            canonical_path = std::filesystem::absolute(fs_path).string();
        }
        
        // 获取文件修改时间
        std::string last_modified;
        try {
            auto ftime = std::filesystem::last_write_time(fs_path);
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - std::filesystem::file_time_type::clock::now() + 
                std::chrono::system_clock::now()
            );
            auto time_t = std::chrono::system_clock::to_time_t(sctp);
            
            std::ostringstream time_ss;
            time_ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
            last_modified = time_ss.str();
        } catch (const std::exception&) {
            last_modified = "unknown";
        }

        return json{
            {"path", path},
            {"canonical_path", canonical_path},
            {"content", content},
            {"size", content.size()},
            {"file_size", file_size},
            {"encoding", encoding},
            {"last_modified", last_modified},
            {"success", true}
        };
        
    } catch (const std::filesystem::filesystem_error& e) {
        return json{
            {"error", "Filesystem error: " + std::string(e.what())},
            {"success", false}
        };
    } catch (const std::exception& e) {
        return json{
            {"error", "Error reading file: " + std::string(e.what())},
            {"success", false}
        };
    } catch (...) {
        return json{
            {"error", "Unknown error occurred while reading file"},
            {"success", false}
        };
    }
}

json executeWriteFile(const json& args) {
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

json executeGlob(const json& args) {
    std::string pattern = args.value("pattern", "");
    std::string path = args.value("path", ".");

    if (pattern.empty()) {
        return json{
            {"error", "Pattern is required"},
            {"success", false}
        };
    }

    if (!file_exists(path)) {
        return json{
            {"error", "Path not found: " + path},
            {"success", false}
        };
    }

    std::vector<std::string> matched_files;
    
    try {
        // 使用filesystem库进行文件匹配
        // 简化实现：支持基本的*通配符
        if (pattern.find("**") != std::string::npos) {
            // 递归搜索
            std::string file_pattern = pattern;
            // 移除路径部分，保留文件名模式
            size_t last_slash = file_pattern.find_last_of("/\\");
            if (last_slash != std::string::npos) {
                file_pattern = file_pattern.substr(last_slash + 1);
            }
            
            // 递归遍历目录
            for (auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    if (matchPattern(filename, file_pattern)) {
                        matched_files.push_back(entry.path().string());
                    }
                }
            }
        } else {
            // 非递归搜索
            for (auto& entry : std::filesystem::directory_iterator(path)) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    if (matchPattern(filename, pattern)) {
                        matched_files.push_back(entry.path().string());
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        return json{
            {"error", "Failed to search files: " + std::string(e.what())},
            {"success", false}
        };
    }

    return json{
        {"pattern", pattern},
        {"path", path},
        {"matches", matched_files},
        {"count", matched_files.size()},
        {"success", true}
    };
}

json executeGrep(const json& args) {
    std::string pattern = args.value("pattern", "");
    std::string path = args.value("path", ".");
    std::string include = args.value("include", "");
    bool case_sensitive = args.value("case_sensitive", false);
    bool line_numbers = args.value("line_numbers", true);

    if (pattern.empty()) {
        return json{
            {"error", "Pattern is required"},
            {"success", false}
        };
    }

    std::vector<json> matches;
    
    try {
        if (std::filesystem::is_regular_file(path)) {
            // 搜索单个文件
            auto file_matches = searchInFile(path, pattern, case_sensitive, line_numbers);
            matches.insert(matches.end(), file_matches.begin(), file_matches.end());
        } else if (std::filesystem::is_directory(path)) {
            // 搜索目录中的文件
            for (auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    
                    // 如果指定了include模式，检查文件是否匹配
                    if (!include.empty() && !matchPattern(filename, include)) {
                        continue;
                    }
                    
                    auto file_matches = searchInFile(entry.path().string(), pattern, case_sensitive, line_numbers);
                    matches.insert(matches.end(), file_matches.begin(), file_matches.end());
                }
            }
        }
    } catch (const std::exception& e) {
        return json{
            {"error", "Failed to search: " + std::string(e.what())},
            {"success", false}
        };
    }

    return json{
        {"pattern", pattern},
        {"path", path},
        {"include", include},
        {"case_sensitive", case_sensitive},
        {"matches", matches},
        {"count", matches.size()},
        {"success", true}
    };
}

json executeMultiEdit(const json& args) {
    std::string file_path = args.value("file_path", "");
    
    if (file_path.empty()) {
        return json{
            {"error", "file_path is required"},
            {"success", false}
        };
    }

    if (!file_exists(file_path)) {
        return json{
            {"error", "File not found: " + file_path},
            {"success", false}
        };
    }

    if (!args.contains("edits") || !args["edits"].is_array()) {
        return json{
            {"error", "edits array is required"},
            {"success", false}
        };
    }

    // 读取文件内容
    std::string content;
    if (!read_file_content(file_path, content)) {
        return json{
            {"error", "Failed to read file"},
            {"success", false}
        };
    }

    std::string original_content = content;
    json edit_results = json::array();
    int successful_edits = 0;

    // 执行每个编辑操作
    for (const auto& edit : args["edits"]) {
        if (!edit.contains("old_string") || !edit.contains("new_string")) {
            edit_results.push_back({
                {"error", "Edit missing old_string or new_string"},
                {"success", false}
            });
            continue;
        }

        std::string old_string = edit["old_string"];
        std::string new_string = edit["new_string"];
        bool replace_all = edit.value("replace_all", false);

        size_t pos = 0;
        int replacements = 0;
        
        while ((pos = content.find(old_string, pos)) != std::string::npos) {
            content.replace(pos, old_string.length(), new_string);
            pos += new_string.length();
            replacements++;
            
            if (!replace_all) break;
        }

        edit_results.push_back({
            {"old_string", old_string},
            {"new_string", new_string},
            {"replacements", replacements},
            {"success", replacements > 0}
        });

        if (replacements > 0) successful_edits++;
    }

    // 写入修改后的内容
    bool write_success = write_file_content(file_path, content);
    
    return json{
        {"file_path", file_path},
        {"total_edits", args["edits"].size()},
        {"successful_edits", successful_edits},
        {"edit_results", edit_results},
        {"file_written", write_success},
        {"success", write_success && successful_edits > 0}
    };
}

json executeEdit(const json& args) {
    std::string file_path = args.value("file_path", "");
    std::string old_string = args.value("old_string", "");
    std::string new_string = args.value("new_string", "");
    bool replace_all = args.value("replace_all", false);

    if (file_path.empty() || old_string.empty()) {
        return json{
            {"error", "file_path and old_string are required"},
            {"success", false}
        };
    }

    if (!file_exists(file_path)) {
        return json{
            {"error", "File not found: " + file_path},
            {"success", false}
        };
    }

    // 读取文件内容
    std::string content;
    if (!read_file_content(file_path, content)) {
        return json{
            {"error", "Failed to read file"},
            {"success", false}
        };
    }

    // 执行替换
    size_t pos = 0;
    int replacements = 0;
    
    while ((pos = content.find(old_string, pos)) != std::string::npos) {
        content.replace(pos, old_string.length(), new_string);
        pos += new_string.length();
        replacements++;
        
        if (!replace_all) break;
    }

    if (replacements == 0) {
        return json{
            {"error", "String not found in file"},
            {"file_path", file_path},
            {"old_string", old_string},
            {"success", false}
        };
    }

    // 写入修改后的内容
    bool write_success = write_file_content(file_path, content);
    
    return json{
        {"file_path", file_path},
        {"old_string", old_string},
        {"new_string", new_string},
        {"replacements", replacements},
        {"replace_all", replace_all},
        {"success", write_success}
    };
}

json executeBash(const json& args) {
    std::string command = args.value("command", "");
    int timeout = args.value("timeout", 30000); // 30秒默认超时
    std::string description = args.value("description", "");

    if (command.empty()) {
        return json{
            {"error", "Command is required"},
            {"success", false}
        };
    }

    // 记录执行的命令
    if (!description.empty()) {
        LOG_INF("Executing bash command: %s - %s\n", description.c_str(), command.c_str());
    } else {
        LOG_INF("Executing bash command: %s\n", command.c_str());
    }

    // 执行命令
    std::string result = execute_command(command);
    
    // 简单检查命令是否成功（基于输出是否包含错误关键词）
    bool success = result.find("command not found") == std::string::npos &&
                   result.find("No such file") == std::string::npos &&
                   result.find("Permission denied") == std::string::npos;

    return json{
        {"command", command},
        {"description", description},
        {"timeout", timeout},
        {"output", result},
        {"success", success}
    };
}

json executeListDirectory(const json& args) {
    std::string path = args.value("path", ".");
    bool recursive = args.value("recursive", false);
    bool show_hidden = args.value("show_hidden", false);
    std::string file_types = args.value("file_types", "all");
    bool size_info = args.value("size_info", true);

    if (!file_exists(path)) {
        return json{
            {"error", "Directory not found: " + path},
            {"success", false}
        };
    }

    std::vector<json> file_list;
    
    try {
        auto process_entry = [&](const std::filesystem::directory_entry& entry) {
            std::string filename = entry.path().filename().string();
            
            // 跳过隐藏文件（如果设置）
            if (!show_hidden && filename[0] == '.') {
                return;
            }
            
            // 文件类型过滤
            if (file_types != "all") {
                std::string extension = entry.path().extension().string();
                if (!extension.empty() && extension[0] == '.') {
                    extension = extension.substr(1);  // 移除点
                }
                
                bool type_match = false;
                if (file_types == "source") {
                    static const std::set<std::string> source_exts = {
                        "cpp", "h", "hpp", "c", "py", "js", "ts", "java", "go", "rs", "php", "rb"
                    };
                    type_match = source_exts.count(extension) > 0;
                } else if (file_types == "data") {
                    static const std::set<std::string> data_exts = {
                        "csv", "json", "xml", "yaml", "yml", "sql", "db", "sqlite"
                    };
                    type_match = data_exts.count(extension) > 0;
                } else if (file_types == "docs") {
                    static const std::set<std::string> doc_exts = {
                        "md", "txt", "pdf", "doc", "docx", "rtf", "html", "rst"
                    };
                    type_match = doc_exts.count(extension) > 0;
                } else {
                    // 自定义扩展名列表
                    auto custom_exts = split_string(file_types, ',');
                    for (auto& ext : custom_exts) {
                        ext = trim(ext);
                        if (ext == extension) {
                            type_match = true;
                            break;
                        }
                    }
                }
                
                if (!type_match) return;
            }
            
            json file_info = {
                {"name", filename},
                {"path", entry.path().string()},
                {"type", entry.is_directory() ? "directory" : "file"}
            };
            
            if (size_info && entry.is_regular_file()) {
                try {
                    auto file_size = std::filesystem::file_size(entry);
                    file_info["size"] = file_size;
                    
                    // 人类可读的大小
                    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
                    double size = static_cast<double>(file_size);
                    int unit = 0;
                    while (size >= 1024 && unit < 4) {
                        size /= 1024;
                        unit++;
                    }
                    
                    std::ostringstream ss;
                    ss << std::fixed << std::setprecision(1) << size << " " << units[unit];
                    file_info["human_size"] = ss.str();
                } catch (...) {
                    file_info["size"] = 0;
                    file_info["human_size"] = "unknown";
                }
                
                try {
                    auto ftime = std::filesystem::last_write_time(entry);
                    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        ftime - std::filesystem::file_time_type::clock::now() + 
                        std::chrono::system_clock::now()
                    );
                    auto time_t = std::chrono::system_clock::to_time_t(sctp);
                    
                    std::ostringstream time_ss;
                    time_ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
                    file_info["modified"] = time_ss.str();
                } catch (...) {
                    file_info["modified"] = "unknown";
                }
            }
            
            file_list.push_back(file_info);
        };
        
        if (recursive) {
            for (auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                process_entry(entry);
            }
        } else {
            for (auto& entry : std::filesystem::directory_iterator(path)) {
                process_entry(entry);
            }
        }
        
        // 按名称排序
        std::sort(file_list.begin(), file_list.end(), 
            [](const json& a, const json& b) {
                return a["name"].get<std::string>() < b["name"].get<std::string>();
            });
            
    } catch (const std::exception& e) {
        return json{
            {"error", "Failed to list directory: " + std::string(e.what())},
            {"success", false}
        };
    }

    return json{
        {"path", path},
        {"recursive", recursive},
        {"show_hidden", show_hidden},
        {"file_types", file_types},
        {"files", file_list},
        {"count", file_list.size()},
        {"success", true}
    };
}

json executeFileStats(const json& args) {
    std::string path = args.value("path", "");
    bool detailed = args.value("detailed", false);
    bool checksum = args.value("checksum", false);
    
    if (path.empty()) {
        return json{
            {"error", "Path is required"},
            {"success", false}
        };
    }
    
    if (!file_exists(path)) {
        return json{
            {"error", "Path not found: " + path},
            {"success", false}
        };
    }
    
    json stats = {
        {"path", path},
        {"exists", true}
    };
    
    try {
        std::filesystem::path fs_path(path);
        stats["absolute_path"] = std::filesystem::absolute(fs_path).string();
        stats["filename"] = fs_path.filename().string();
        
        if (std::filesystem::is_regular_file(fs_path)) {
            stats["type"] = "file";
            auto file_size = std::filesystem::file_size(fs_path);
            stats["size"] = file_size;
            
            // 人类可读大小
            const char* units[] = {"B", "KB", "MB", "GB", "TB"};
            double size = static_cast<double>(file_size);
            int unit = 0;
            while (size >= 1024 && unit < 4) {
                size /= 1024;
                unit++;
            }
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(1) << size << " " << units[unit];
            stats["human_size"] = ss.str();
            
            // 文件扩展名
            if (fs_path.has_extension()) {
                stats["extension"] = fs_path.extension().string();
            }
            
        } else if (std::filesystem::is_directory(fs_path)) {
            stats["type"] = "directory";
            
            // 计算目录中的文件数量
            if (detailed) {
                size_t file_count = 0;
                size_t dir_count = 0;
                uintmax_t total_size = 0;
                
                try {
                    for (auto& entry : std::filesystem::recursive_directory_iterator(fs_path)) {
                        if (entry.is_regular_file()) {
                            file_count++;
                            total_size += std::filesystem::file_size(entry);
                        } else if (entry.is_directory()) {
                            dir_count++;
                        }
                    }
                    
                    stats["file_count"] = file_count;
                    stats["directory_count"] = dir_count;
                    stats["total_size"] = total_size;
                } catch (...) {
                    stats["file_count"] = "unknown";
                    stats["directory_count"] = "unknown"; 
                    stats["total_size"] = "unknown";
                }
            }
        } else {
            stats["type"] = "other";
        }
        
        // 时间戳信息
        auto ftime = std::filesystem::last_write_time(fs_path);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now() + 
            std::chrono::system_clock::now()
        );
        auto time_t = std::chrono::system_clock::to_time_t(sctp);
        
        std::ostringstream time_ss;
        time_ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        stats["last_modified"] = time_ss.str();
        stats["last_modified_timestamp"] = time_t;
        
        // 详细信息
        if (detailed) {
            auto perms = std::filesystem::status(fs_path).permissions();
            std::string perm_str;
            
            perm_str += (perms & std::filesystem::perms::owner_read) != std::filesystem::perms::none ? "r" : "-";
            perm_str += (perms & std::filesystem::perms::owner_write) != std::filesystem::perms::none ? "w" : "-";  
            perm_str += (perms & std::filesystem::perms::owner_exec) != std::filesystem::perms::none ? "x" : "-";
            perm_str += (perms & std::filesystem::perms::group_read) != std::filesystem::perms::none ? "r" : "-";
            perm_str += (perms & std::filesystem::perms::group_write) != std::filesystem::perms::none ? "w" : "-";
            perm_str += (perms & std::filesystem::perms::group_exec) != std::filesystem::perms::none ? "x" : "-";
            perm_str += (perms & std::filesystem::perms::others_read) != std::filesystem::perms::none ? "r" : "-";
            perm_str += (perms & std::filesystem::perms::others_write) != std::filesystem::perms::none ? "w" : "-";
            perm_str += (perms & std::filesystem::perms::others_exec) != std::filesystem::perms::none ? "x" : "-";
            
            stats["permissions"] = perm_str;
        }
        
        // 校验和计算（仅对文件）
        if (checksum && std::filesystem::is_regular_file(fs_path)) {
            std::string content;
            if (read_file_content(path, content)) {
                // 简单的哈希值计算（这里使用一个简化版本）
                std::hash<std::string> hasher;
                size_t hash_value = hasher(content);
                
                std::ostringstream hash_ss;
                hash_ss << std::hex << hash_value;
                stats["hash"] = hash_ss.str();
                stats["content_length"] = content.length();
            }
        }
        
    } catch (const std::exception& e) {
        return json{
            {"error", "Failed to get file stats: " + std::string(e.what())},
            {"path", path},
            {"success", false}
        };
    }
    
    stats["success"] = true;
    return stats;
}

} // namespace BuiltinTools
