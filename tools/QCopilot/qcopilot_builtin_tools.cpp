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
#include <regex>

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

    // read_text_file tool
    definitions.push_back({
        "read_text_file",
        {
            {"type", "function"},
            {"function", {
                {"name", "read_text_file"},
                {"description", "Specialized UTF-8 text file reading utility for loading text documents with line range selection.  This tool is designed exclusively for UTF-8 encoded text files and will fail on non-UTF-8 files. Primary use cases: 1) Reading UTF-8 configuration files (.ini, .conf, .json, .yaml) 2) Loading UTF-8 source code files (.py, .js, .cpp, .h) 3) Processing UTF-8 log files (.log) 4) Reading UTF-8 documentation (.md, .rst, .txt) 5) Extracting specific line ranges from large UTF-8 text files for analysis. Does not read binary files or provide file metadata - focuses solely on UTF-8 text content extraction. IMPORTANT: Before using this tool, you must first use the built-in tool validate_utf8_file to verify whether the target file is in UTF-8 character encoding. If you need to know the total number of lines in the target file, use the built-in tool inspect_path to obtain the total number of lines in the target file.Workflow: validate_utf8_file → read_text_file or validate_utf8_file → inspect_path → read_text_file."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"path", {{"type", "string"}, {"description", "The full file path (absolute or relative) of the target UTF-8 text file. For example: './config.txt', '/var/log/app.log', 'C:/Documents/readme.md'. Only UTF-8 text files are supported; binary files will be rejected. Also, please carefully check that the full file path you are extracting contains no spaces."}}},
                        {"start_line", {{"type", "integer"}, {"description", "Starting line number for range reading (1-based indexing). Default 1 (first line). Use with end_line to read specific sections of large files."}}},
                        {"end_line", {{"type", "integer"}, {"description", "Ending line number for range reading (1-based indexing, inclusive). Default -1 (last line). Combined with start_line allows reading specific portions of files."}}}
                    }},
                    {"required", {"path"}}
                }}
            }}
        }
    });

    // write_text_file tool
    definitions.push_back({
        "write_text_file",
        {
            {"type", "function"},
            {"function", {
                {"name", "write_text_file"},
                {"description", "Specialized UTF-8 text file writing utility for creating and modifying UTF-8 encoded text files with robust encoding validation and error handling. IMPORTANT: For existing files, you MUST first use the validate_utf8_file built-in tool to verify UTF-8 encoding before writing. New files are automatically created with UTF-8 encoding. Single responsibility: writes UTF-8 text content only, does not read file metadata. Key applications: 1) Creating UTF-8 source code files (.py, .js, .cpp, .h, .sql) 2) Writing UTF-8 configuration files (.ini, .conf, .json, .yaml) 3) Generating UTF-8 documentation (.md, .rst, .txt) 4) Maintaining UTF-8 log files with append mode 5) Creating UTF-8 data files (.csv, .xml).  Workflow for existing files: validate_utf8_file → write_text_file. For new files: directly use write_text_file."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"path", {{"type", "string"}, {"description", "Target UTF-8 text file path (absolute or relative) where content will be written. Automatically creates parent directories if they don't exist. Examples: './config.txt', '/var/log/app.log', 'C:/Documents/readme.md'. Only UTF-8 text files are supported. Ensure the full file path contains no spaces for cross-platform compatibility."}}},
                        {"content", {{"type", "string"}, {"description", "UTF-8 encoded text content to write to the file. Supports plain text, source code, configuration syntax, structured data (JSON, CSV, XML), and documentation formats. Handles newlines and UTF-8 special characters appropriately."}}},
                        {"append", {{"type", "boolean"}, {"description", "Write mode selection: false (overwrite mode, default) completely replaces existing file content, ideal for generating new files and configuration updates; true (append mode) adds content to existing file end, perfect for log files, data collection, and incremental updates. For existing files in append mode, UTF-8 encoding validation is required."}}}
                    }},
                    {"required", {"path", "content"}}
                }}
            }}
        }
    });

    // glob tool
    definitions.push_back({
        "glob",
        {
            {"type", "function"},
            {"function", {
                {"name", "glob"},
                {"description", "Advanced file pattern matching tool for discovering files using glob patterns with wildcard support. This tool is essential for file system navigation, project organization, and batch file operations. KEY FEATURES: 1) PATTERN MATCHING - Uses '*' wildcard for flexible filename matching (e.g., '*.txt' finds all text files, 'test_*' finds files starting with 'test_', '*_backup.*' finds backup files) 2) RECURSIVE SEARCH - Include '**' in pattern to enable recursive directory traversal (e.g., '**/*.cpp' searches all subdirectories for C++ files) 3) CASE SENSITIVITY - Control case-sensitive matching for cross-platform compatibility 4) RESULT LIMITING - Set max_results to prevent overwhelming output from large directories. COMMON USE CASES: • Finding source code files: pattern='**/*.py' (all Python files), pattern='src/**/*.h' (header files in src) • Locating configuration files: pattern='*.conf', pattern='**/config.*' • Searching for specific file types: pattern='*.log' (log files), pattern='*.json' (JSON files) • Finding files by name patterns: pattern='test_*', pattern='*_backup.*', pattern='README.*' • Project file discovery: pattern='**/*.md' (documentation), pattern='**/Makefile' (build files). PATTERN SYNTAX: Use '*' for any characters, '**' for recursive search. The tool matches against the final filename component, not the full path. Examples: {'pattern': '*.txt'} finds text files in current directory, {'pattern': '**/*.cpp', 'path': 'src'} recursively finds C++ files under src directory, {'pattern': 'test_*', 'case_sensitive': false} finds test files ignoring case."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"pattern",        {{"type","string"},  {"description","Glob pattern; '*' for wildcard. If the pattern contains '**', recursion is enabled (note: '**' only toggles recursion; filename matching still uses '*')."}}},
                        {"path",           {{"type","string"},  {"description","Base directory. Absolute or relative. Default '.'."}, {"default","."}}},
                        {"case_sensitive", {{"type","boolean"}, {"description","Case-sensitive filename matching. Default true."}, {"default", true}}},
                        {"max_results",    {{"type","integer"}, {"description","Soft cap on returned matches; sets 'truncated=true' when reached. Default 100."}, {"default", 100}}}
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
                {"description", "High-throughput text search utility for a single file or an entire directory tree. Supports literal substring search or ECMAScript regular expressions, optional filename include filter with '*', line numbers, case control, and global hit caps. Designed for code navigation/refactors, log mining, configuration audits, quick data extraction, and lightweight security sweeps without external tools. Capabilities: (1) Recursive directory search or single-file scan (2) Literal or regex matching via 'regex' (3) Case-insensitive or sensitive search (4) Basename filter using 'include' (e.g., '*.cpp') to reduce IO (5) Line numbers for direct navigation (6) 'max_matches' cap with 'truncated=true' when reached. Examples: args {'path':'src','pattern':'TODO','include':'*.cpp'} (find TODOs in C++ sources recursively); args {'path':'README.md','pattern':'\\bclass\\s+\\w+','regex':true} (regex for class definitions in a file); args {'pattern':'ERROR|FATAL','regex':true,'case_sensitive':false} (errors across current dir, case-insensitive); args {'path':'logs','pattern':'session_id=','include':'*.log','max_matches':1000} (cap hits for performance)."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"pattern",        {{"type","string"},  {"description","Search pattern. Literal text when 'regex'=false; ECMAScript regular expression when 'regex'=true."}}},
                        {"path",           {{"type","string"},  {"description","Target file or directory. Default '.'."}, {"default","."}}},
                        {"include",        {{"type","string"},  {"description","Optional filename filter using '*' (applies to basename), e.g., '*.cpp', '*.log'."}}},
                        {"regex",          {{"type","boolean"}, {"description","Use regular expression search. Default false."}, {"default", false}}},
                        {"case_sensitive", {{"type","boolean"}, {"description","Case-sensitive matching. Default false."}, {"default", false}}},
                        {"line_numbers",   {{"type","boolean"}, {"description","Include 'line_number' in results. Default true."}, {"default", true}}},
                        {"max_matches",    {{"type","integer"}, {"description","Global cap on total matches; sets 'truncated=true' when reached. Default 10000."}, {"default", 10000}}}
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
                        {"file_path", {{"type", "string"}, {"description", "Target file path for editing operations. Must be an existing readable file. Examples: './src/main.py', '/etc/nginx/nginx.conf', 'C:/Projects/config/app.json'. File is locked during editing to prevent concurrent modifications."}}},
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

    // list_directory tool
    definitions.push_back({
        "list_directory",
        {
            {"type", "function"},
            {"function", {
                {"name", "list_directory"},
                {"description", "Comprehensive directory enumeration utility with recursion, hidden-item visibility, kind filtering (files/dirs), extension allow-list, sorting, pagination, and optional size/timestamp enrichment. Ideal for project inventories, build prep, housekeeping (largest/oldest), packaging manifests, and pre-filters before heavier steps. Capabilities: (1) Single folder or full subtree traversal (2) Filter to 'files' or 'dirs' via 'kinds' (3) Restrict by extensions (e.g., 'cpp,h,py') via 'ext_filter' (4) Sort by 'name'/'size'/'modified' with 'asc'/'desc' (5) Paginate using 'offset' and 'limit' (6) Soft cap via 'max_results' and 'truncated=true' when reached (7) Optional size and human-readable sizes, plus modified time. Examples: args {'path':'.','kinds':'files','ext_filter':'cpp,h'} (list C/C++ sources in cwd); args {'path':'data','recursive':true,'sort_by':'size','order':'desc','limit':100} (top 100 largest under data); args {'path':'.','show_hidden':true,'kinds':'dirs'} (include hidden directories); args {'path':'assets','sort_by':'modified','order':'desc','offset':50,'limit':25} (paged recent items)."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"path",        {{"type","string"},  {"description","Directory to list. Absolute or relative."}}},
                        {"recursive",   {{"type","boolean"}, {"description","Recurse into subdirectories. Default false."}, {"default", false}}},
                        {"show_hidden", {{"type","boolean"}, {"description","Include entries whose names start with '.'. Default false."}, {"default", false}}},
                        {"kinds",       {{"type","string"},  {"enum", {"all","files","dirs"}}, {"description","Filter by kind: 'all' | 'files' | 'dirs'. Default 'all'."}, {"default","all"}}},
                        {"ext_filter",  {{"type","string"},  {"description","Comma-separated extension allow-list (without dots), e.g., 'cpp,h,py'. Applies to files only."}}},
                        {"size_info",   {{"type","boolean"}, {"description","Include 'size' and human-readable size for files. Default true."}, {"default", true}}},
                        {"sort_by",     {{"type","string"},  {"enum", {"name","size","modified"}}, {"description","Sort field. Default 'name'."}, {"default","name"}}},
                        {"order",       {{"type","string"},  {"enum", {"asc","desc"}}, {"description","Sort order. Default 'asc'."}, {"default","asc"}}},
                        {"limit",       {{"type","integer"}, {"description","Return at most this many items (pagination). Default 0 = no explicit limit."}, {"default", 0}}},
                        {"offset",      {{"type","integer"}, {"description","Skip this many items before returning (pagination). Default 0."}, {"default", 0}}},
                        {"max_results", {{"type","integer"}, {"description","Internal soft cap during enumeration; sets 'truncated=true' if reached. Default 50000."}, {"default", 50000}}}
                    }},
                    {"required", {"path"}}
                }}
            }}
        }
    });

    // inspect_path tool
    definitions.push_back({
        "inspect_path",
        {
            {"type", "function"},
            {"function", {
                {"name", "inspect_path"},
                {"description", "Cross-platform path inspection and file analysis utility for comprehensive path validation and metadata extraction. Designed exclusively for path validation, file type identification, and content analysis with UTF-8 text file support. IMPORTANT: Uses only C++ standard library for maximum compatibility across Windows and Linux systems. Primary use cases: 1) Path validation and existence verification before file operations 2) File type identification (file/directory/other) for workflow routing 3) Basic file metadata extraction (size, timestamps, permissions) 4) UTF-8 text file analysis including line counting for documentation and code files 5) Directory content summarization for project organization 6) Cross-platform file system inspection without platform-specific dependencies. Key features: path validation, absolute/relative path resolution, file type detection, size analysis with human-readable formats, modification timestamps, Unix-style permissions (detailed mode), and UTF-8 text line counting. Workflow: inspect_path → read_text_file/write_text_file (for confirmed text files)."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"path", {{"type", "string"}, {"description", "Target path for inspection (absolute or relative). Examples: './config.txt', '/var/log/app.log', 'C:/Documents/readme.md', '../src/main.cpp'. Supports cross-platform path formats. The tool validates path syntax and existence before analysis."}}},
                        {"detailed", {{"type", "boolean"}, {"description", "Analysis depth: false (basic inspection - path validation, type identification, size, modified time) or true (comprehensive analysis - includes permissions, directory statistics, and enhanced metadata). Default false for optimal performance. Detailed mode provides Unix-style permission strings and recursive directory analysis."}}},
                        {"text_analysis", {{"type", "boolean"}, {"description", "Enable UTF-8 text file analysis including line counting and content statistics. Only applicable to regular files that can be read as text. Provides line count, character count for text files. Default false to avoid unnecessary file reading. Useful for code files, documentation, configuration files."}}}
                    }},
                    {"required", {"path"}}
                }}
            }}
        }
    });

    // validate_utf8_file tool
    definitions.push_back({
        "validate_utf8_file",
        {
            {"type", "function"},
            {"function", {
                {"name", "validate_utf8_file"},
                {"description", "UTF-8 encoding validation tool for text files. Verifies whether a specified file contains valid UTF-8 encoded content. Essential for text processing workflows, data validation, and ensuring compatibility with UTF-8-only tools. Primary use cases: 1) Pre-processing validation before using UTF-8-only text tools 2) Data import validation to ensure encoding compatibility 3) File conversion workflow validation 4) International text content verification 5) Web content encoding validation. Performs comprehensive UTF-8 validation including proper byte sequences, overlong encodings, and invalid code points. Supports files up to 10MB for validation performance."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"path", {{"type", "string"}, {"description", "Complete file path (absolute or relative) to the file to validate for UTF-8 encoding. Examples: './document.txt', '/var/log/application.log', 'C:/Data/content.csv'. Only regular files are supported."}}}
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

    functions["read_text_file"] = [](const json& args) {
        return executeReadTextFile(args);
    };

    functions["write_text_file"] = [](const json& args) {
        return executeWriteTextFile(args);
    };

    functions["glob"] = [](const json& args) {
        return executeGlob(args);
    };

    functions["grep"] = [](const json& args) {
        return executeGrep(args);
    };

    functions["edit"] = [](const json& args) {
        return executeEdit(args);
    };

    functions["list_directory"] = [](const json& args) {
        return executeListDirectory(args);
    };

    functions["inspect_path"] = [](const json& args) {
        return executeInspectPath(args);
    };

    functions["validate_utf8_file"] = [](const json& args) {
        return executeCheckUtf8Encoding(args);
    };

    return functions;
}

/********各个内置工具使用到的函数********/

// 前向声明内部函数
static double evaluateExpression(const std::string& expr);
static double parseExpression(const std::string& expr, size_t& pos);
static double parseTerm(const std::string& expr, size_t& pos);
static double parseFactor(const std::string& expr, size_t& pos);
static double parseFunction(const std::string& funcName, const std::string& expr, size_t& pos);
static void skipWhitespace(const std::string& expr, size_t& pos);
static bool isFunction(const std::string& name);

// 计算工具将会用到的常量
static const double PI = 3.14159265358979323846;
static const double E = 2.71828182845904523536;

// 计算表达式的主函数
static double evaluateExpression(const std::string& expr) {
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
static double parseExpression(const std::string& expr, size_t& pos) {
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
static double parseTerm(const std::string& expr, size_t& pos) {
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
static double parseFactor(const std::string& expr, size_t& pos) {
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
static double parseFunction(const std::string& funcName, const std::string& expr, size_t& pos) {
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
static void skipWhitespace(const std::string& expr, size_t& pos) {
    while (pos < expr.length() && std::isspace(expr[pos])) {
        pos++;
    }
}
// 检查字符串是否是数学函数
static bool isFunction(const std::string& name) {
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
// 使用 Regex 模式在文件中匹配
std::vector<json> searchInFileRegex(
    const std::string& filepath,
    const std::string& pattern,
    bool use_regex,
    bool case_sensitive,
    bool line_numbers,
    int& total_matches,
    int max_matches) {

    std::vector<json> matches;
    std::string content;

    if (!read_file_content(filepath, content)) {
        return matches;
    }

    // 准备正则表达式（如果需要）
    std::regex regex_pattern;
    if (use_regex) {
        try {
            auto flags = std::regex::ECMAScript;
            if (!case_sensitive) {
                flags |= std::regex::icase;
            }
            regex_pattern = std::regex(pattern, flags);
        } catch (const std::regex_error& e) {
            LOG_WRN("Invalid regex pattern: %s", e.what());
            return matches;
        }
    }

    std::istringstream iss(content);
    std::string line;
    int line_num = 1;

    while (std::getline(iss, line) && total_matches < max_matches) {
        bool found = false;

        if (use_regex) {
            found = std::regex_search(line, regex_pattern);
        } else {
            // 子串搜索
            std::string search_line = line;
            std::string search_pattern = pattern;

            if (!case_sensitive) {
                std::transform(search_line.begin(), search_line.end(), search_line.begin(), ::tolower);
                std::transform(search_pattern.begin(), search_pattern.end(), search_pattern.begin(), ::tolower);
            }

            found = (search_line.find(search_pattern) != std::string::npos);
        }

        if (found) {
            json match = {
                {"file", filepath},
                {"line_content", line}
            };

            if (line_numbers) {
                match["line_number"] = line_num;
            }

            matches.push_back(match);
            total_matches++;

            if (total_matches >= max_matches) {
                break;
            }
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

json executeReadTextFile(const json& args) {
    //  从json参数中提取输入，如果有就获取，否则使用默认值
    std::string path = args.value("path", "");
    int start_line = args.value("start_line", 1);
    int end_line = args.value("end_line", -1);

    // 判断输入的路径是否为空
    if (path.empty()) {
        return json{
            {"error", "Path is required"},
            {"success", false}
        };
    }
    // 验证行号参数
    if (start_line < 1) {
        return json{
            {"error", "start_line must be >= 1"},
            {"success", false}
        };
    }
    // end_line 为 -1 表示读取到文件末尾
    if (end_line != -1 && end_line < start_line) {
        return json{
            {"error", "end_line must be >= start_line or -1 for end of file"},
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
                {"error", "File not found: " + path},
                {"success", false}
            };
        }

        // 检查是否是目录而不是文件
        if (std::filesystem::is_directory(fs_path)) {
            return json{
                {"error", "Path is a directory, not a text file: " + path},
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
        // 100MB 限制
        const size_t MAX_FILE_SIZE = 100 * 1024 * 1024;
        // 如果文件过大，返回错误
        if (file_size > MAX_FILE_SIZE) {
            return json{
                {"error", "File too large for text reading (maximum 100MB): " + std::to_string(file_size) + " bytes"},
                {"success", false}
            };
        }

        // 读取文本文件内容
        std::string content;
        int lines_read = 0;
        int actual_end_line = end_line;
        // 读取文件时进行错误处理
        if (!read_text_file_with_encoding_and_range(path, start_line, end_line, content, lines_read, actual_end_line)) {
            return json{
                {"error", "Failed to read UTF-8 text file. File may be binary or not UTF-8 encoded."},
                {"success", false}
            };
        }

        // 对内容进行清理，确保JSON序列化安全
        std::string safe_content = sanitize_string_for_json(content);

        // 额外的安全措施：确保字符串长度合理，10MB限制
        if (safe_content.size() > 10 * 1024 * 1024) {
            LOG_WRN("Content too large (%zu bytes), truncating", safe_content.size());
            safe_content = safe_content.substr(0, 10 * 1024 * 1024) + "...[truncated]";
        }

        // 安全地构建JSON对象
        json result;
        result["success"] = true;
        result["lines_read"] = lines_read;
        result["start_line"] = start_line;
        result["end_line"] = actual_end_line;

        // 对路径字段进行清理
        result["path"] = sanitize_string_for_json(path);

        // 最后设置内容字段
        try {
            result["content"] = safe_content;
        } catch (const std::exception& e) {
            LOG_ERR("Failed to set content field: %s", e.what());
            result["content"] = "[content unavailable due to UTF-8 encoding issues]";
        }
        // 返回结果
        return result;

    } catch (const std::filesystem::filesystem_error& e) {
        return json{
            {"error", "Filesystem error: " + std::string(e.what())},
            {"success", false}
        };
    } catch (const std::exception& e) {
        return json{
            {"error", "Error reading text file: " + std::string(e.what())},
            {"success", false}
        };
    } catch (...) {
        return json{
            {"error", "Unknown error occurred while reading text file"},
            {"success", false}
        };
    }
}

json executeWriteTextFile(const json& args) {
    /*
    功能说明：专用于UTF-8文本文件写入的工具，支持覆盖/追加两种模式
    注意事项：
    1. 对于已存在的文件，使用前应先用validate_utf8_file检查编码
    2. 新建文件自动创建为UTF-8编码
    3. 具备完善的错误处理和日志记录
    4. 单一责任：只写入内容，不读取文件状态信息
    */

    std::string path = args.value("path", "");
    std::string content = args.value("content", "");
    bool append = args.value("append", false);

    // 参数验证
    if (path.empty()) {
        LOG_ERR("write_text_file: Empty path provided");
        return json{
            {"error", "Path is required"},
            {"success", false}
        };
    }
    // 内容允许为空，但记录警告日志，因为有可能只是创建一个空文件
    if (content.empty()) {
        LOG_WRN("write_text_file: Empty content provided for path: %s", path.c_str());
    }

    // 路径安全检查：防止路径遍历攻击
    if (path.find("..") != std::string::npos) {
        LOG_ERR("write_text_file: Path traversal attack detected: %s", path.c_str());
        return json{
            {"error", "Path traversal not allowed"},
            {"success", false}
        };
    }

    // 检查路径长度是否合理
    if (path.length() > 4096) {
        LOG_ERR("write_text_file: Path too long (%zu characters): %s", path.length(), path.c_str());
        return json{
            {"error", "Path too long (maximum 4096 characters)"},
            {"success", false}
        };
    }

    try {
        bool file_existed = file_exists(path);

        // 对于已存在的文件，记录日志提醒应先检查编码
        if (file_existed) {
            LOG_INF("write_text_file: Writing to existing file: %s (mode: %s)",
                path.c_str(), append ? "append" : "overwrite");
        } else {
            LOG_INF("write_text_file: Creating new UTF-8 text file: %s", path.c_str());
        }

        bool write_success;
        if (append && file_existed) {
            // 追加模式：直接以追加方式打开文件
            write_success = append_file_content(path, content);
        } else {
            // 覆盖模式或新文件：直接写入
            write_success = write_file_content(path, content);
        }

        if (!write_success) {
            LOG_ERR("write_text_file: Failed to write content to file: %s", path.c_str());
            return json{
                {"error", "Failed to write file"},
                {"success", false}
            };
        }

        LOG_INF("write_text_file: Successfully wrote %zu bytes to: %s", content.size(), path.c_str());
        return json{
            {"path", path},
            {"bytes_written", content.size()},
            {"mode", append ? "append" : "overwrite"},
            {"file_existed", file_existed},
            {"success", true}
        };

    } catch (const std::exception& e) {
        LOG_ERR("write_text_file: Unexpected error writing file %s: %s", path.c_str(), e.what());
        return json{
            {"error", "Unexpected error during file write: " + std::string(e.what())},
            {"success", false}
        };
    }
}

json executeGlob(const json& args) {
    /*
    1、功能解释：文件匹配工具，用于根据通配符模式在指定路径中搜索文件。
    2、参数说明：
        2.1 pattern (必需参数) - 文件匹配模式，支持通配符
        2.2 path (可选，默认 ".") - 搜索路径
        2.3 case_sensitive (可选，默认 true) - 是否区分大小写
        2.4 max_results (可选，默认 10000) - 最大返回结果数
    */
    std::string pattern = args.value("pattern", "");
    std::string path = args.value("path", ".");
    bool case_sensitive = args.value("case_sensitive", true);
    int max_results = args.value("max_results", 10000);

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
    bool truncated = false;

    try {
        // ** 仅作为递归开关，实际匹配仍使用 * 模式
        bool recursive = (pattern.find("**") != std::string::npos);

        // 提取文件名模式（移除路径部分）
        std::string file_pattern = pattern;
        // 如果包含 **，将其简化为 * 用于文件名匹配
        size_t star_star_pos = file_pattern.find("**");
        if (star_star_pos != std::string::npos) {
            file_pattern.replace(star_star_pos, 2, "*");
        }

        // 提取纯文件名部分用于匹配
        size_t last_slash = file_pattern.find_last_of("/\\");
        if (last_slash != std::string::npos) {
            file_pattern = file_pattern.substr(last_slash + 1);
        }

        auto match_file = [&](const std::filesystem::path& entry_path) {
            if (!std::filesystem::is_regular_file(entry_path)) {
                return;
            }

            if (matched_files.size() >= static_cast<size_t>(max_results)) {
                truncated = true;
                return;
            }

            std::string filename = entry_path.filename().string();
            std::string pattern_to_match = file_pattern;

            // 大小写不敏感时统一转换为小写
            if (!case_sensitive) {
                std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
                std::transform(pattern_to_match.begin(), pattern_to_match.end(), pattern_to_match.begin(), ::tolower);
            }

            if (matchPattern(filename, pattern_to_match)) {
                matched_files.push_back(entry_path.string());
            }
        };

        if (recursive) {
            for (auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                if (truncated) break;
                match_file(entry.path());
            }
        } else {
            for (auto& entry : std::filesystem::directory_iterator(path)) {
                if (truncated) break;
                match_file(entry.path());
            }
        }
    } catch (const std::exception& e) {
        return json{
            {"error", "Failed to search files: " + std::string(e.what())},
            {"success", false}
        };
    }

    json result = {
        {"pattern", pattern},
        {"path", path},
        {"case_sensitive", case_sensitive},
        {"matches", matched_files},
        {"count", matched_files.size()},
        {"success", true}
    };

    if (truncated) {
        result["truncated"] = true;
    }

    return result;
}

json executeGrep(const json& args) {
    std::string pattern = args.value("pattern", "");
    std::string path = args.value("path", ".");
    std::string include = args.value("include", "");
    bool case_sensitive = args.value("case_sensitive", false);
    bool line_numbers = args.value("line_numbers", true);
    bool use_regex = args.value("regex", false);
    int max_matches = args.value("max_matches", 10000);

    if (pattern.empty()) {
        return json{
            {"error", "Pattern is required"},
            {"success", false}
        };
    }

    std::vector<json> matches;
    int total_matches = 0;
    bool truncated = false;

    try {
        if (std::filesystem::is_regular_file(path)) {
            // 搜索单个文件
            auto file_matches = searchInFileRegex(path, pattern, use_regex,
                                                  case_sensitive, line_numbers,
                                                  total_matches, max_matches);
            matches.insert(matches.end(), file_matches.begin(), file_matches.end());
        } else if (std::filesystem::is_directory(path)) {
            // 搜索目录中的文件
            for (auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                if (total_matches >= max_matches) {
                    truncated = true;
                    break;
                }

                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();

                    // include 过滤
                    if (!include.empty()) {
                        std::string include_pattern = include;
                        if (!case_sensitive) {
                            std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
                            std::transform(include_pattern.begin(), include_pattern.end(), include_pattern.begin(), ::tolower);
                        }
                        if (!matchPattern(filename, include_pattern)) {
                            continue;
                        }
                    }

                    auto file_matches = searchInFileRegex(entry.path().string(), pattern, use_regex,
                                                          case_sensitive, line_numbers,
                                                          total_matches, max_matches);
                    matches.insert(matches.end(), file_matches.begin(), file_matches.end());
                }
            }
        } else {
            return json{
                {"error", "Path is neither file nor directory: " + path},
                {"success", false}
            };
        }
    } catch (const std::exception& e) {
        return json{
            {"error", "Failed to search: " + std::string(e.what())},
            {"success", false}
        };
    }

    json result = {
        {"pattern", pattern},
        {"path", path},
        {"regex", use_regex},
        {"case_sensitive", case_sensitive},
        {"include", include},
        {"matches", matches},
        {"count", matches.size()},
        {"success", true}
    };

    if (truncated) {
        result["truncated"] = true;
    }

    return result;
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

json executeListDirectory(const json& args) {
    std::string path = args.value("path", ".");
    bool recursive = args.value("recursive", false);
    bool show_hidden = args.value("show_hidden", false);
    std::string kinds = args.value("kinds", "all");
    std::string ext_filter = args.value("ext_filter", "");
    bool size_info = args.value("size_info", true);
    std::string sort_by = args.value("sort_by", "name");
    std::string order = args.value("order", "asc");
    int limit = args.value("limit", 0);
    int offset = args.value("offset", 0);
    int max_results = args.value("max_results", 50000);

    if (!file_exists(path)) {
        return json{
            {"error", "Directory not found: " + path},
            {"success", false}
        };
    }

    if (!std::filesystem::is_directory(path)) {
        return json{
            {"error", "Path is not a directory: " + path},
            {"success", false}
        };
    }

    std::vector<json> file_list;

    // 解析扩展名过滤器
    std::set<std::string> allowed_exts;
    if (!ext_filter.empty()) {
        auto exts = split_string(ext_filter, ',');
        for (auto& ext : exts) {
            std::string trimmed = trim(ext);
            if (!trimmed.empty()) {
                if (trimmed[0] != '.') trimmed = "." + trimmed;
                allowed_exts.insert(trimmed);
            }
        }
    }

    try {
        auto process_entry = [&](const std::filesystem::directory_entry& entry) {
            if (file_list.size() >= static_cast<size_t>(max_results)) {
                return false; // 达到上限
            }

            std::string filename = entry.path().filename().string();

            // 隐藏文件过滤
            if (!show_hidden && !filename.empty() && filename[0] == '.') {
                return true; // 继续
            }

            // 类型过滤
            bool is_dir = entry.is_directory();
            bool is_file = entry.is_regular_file();

            if (kinds == "files" && !is_file) return true;
            if (kinds == "dirs" && !is_dir) return true;

            // 扩展名过滤（仅对文件）
            if (!allowed_exts.empty() && is_file) {
                std::string ext = entry.path().extension().string();
                if (allowed_exts.find(ext) == allowed_exts.end()) {
                    return true;
                }
            }

            json file_info = {
                {"name", filename},
                {"path", entry.path().string()},
                {"type", is_dir ? "directory" : "file"}
            };

            if (size_info && is_file) {
                try {
                    auto file_size = std::filesystem::file_size(entry);
                    file_info["size"] = file_size;

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
                    file_info["human_size"] = ss.str();
                } catch (...) {
                    file_info["size"] = 0;
                    file_info["human_size"] = "0 B";
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
                    file_info["modified_timestamp"] = time_t;
                } catch (...) {
                    file_info["modified"] = "";
                    file_info["modified_timestamp"] = 0;
                }
            }

            file_list.push_back(file_info);
            return true; // 继续
        };

        bool truncated = false;

        if (recursive) {
            for (auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                if (!process_entry(entry)) {
                    truncated = true;
                    break;
                }
            }
        } else {
            for (auto& entry : std::filesystem::directory_iterator(path)) {
                if (!process_entry(entry)) {
                    truncated = true;
                    break;
                }
            }
        }

        // 排序
        if (sort_by == "name") {
            std::sort(file_list.begin(), file_list.end(),
                [&order](const json& a, const json& b) {
                    bool less = a["name"].get<std::string>() < b["name"].get<std::string>();
                    return order == "asc" ? less : !less;
                });
        } else if (sort_by == "size" && size_info) {
            std::sort(file_list.begin(), file_list.end(),
                [&order](const json& a, const json& b) {
                    uint64_t size_a = a.contains("size") ? a["size"].get<uint64_t>() : 0;
                    uint64_t size_b = b.contains("size") ? b["size"].get<uint64_t>() : 0;
                    bool less = size_a < size_b;
                    return order == "asc" ? less : !less;
                });
        } else if (sort_by == "modified" && size_info) {
            std::sort(file_list.begin(), file_list.end(),
                [&order](const json& a, const json& b) {
                    int64_t time_a = a.contains("modified_timestamp") ? a["modified_timestamp"].get<int64_t>() : 0;
                    int64_t time_b = b.contains("modified_timestamp") ? b["modified_timestamp"].get<int64_t>() : 0;
                    bool less = time_a < time_b;
                    return order == "asc" ? less : !less;
                });
        }

        // 分页
        std::vector<json> paged_list;
        if (limit > 0 || offset > 0) {
            size_t start = static_cast<size_t>(offset);
            size_t end = limit > 0 ? start + static_cast<size_t>(limit) : file_list.size();

            for (size_t i = start; i < std::min(end, file_list.size()); ++i) {
                paged_list.push_back(file_list[i]);
            }
        } else {
            paged_list = file_list;
        }

        // 清理不需要的 modified_timestamp 字段
        for (auto& item : paged_list) {
            if (item.contains("modified_timestamp")) {
                item.erase("modified_timestamp");
            }
        }

        json result = {
            {"path", path},
            {"recursive", recursive},
            {"show_hidden", show_hidden},
            {"kinds", kinds},
            {"files", paged_list},
            {"count", paged_list.size()},
            {"success", true}
        };

        if (truncated) {
            result["truncated"] = true;
        }

        return result;

    } catch (const std::exception& e) {
        return json{
            {"error", "Failed to list directory: " + std::string(e.what())},
            {"success", false}
        };
    }
}

json executeInspectPath(const json& args) {
    /*
    功能说明：跨平台路径检查和文件分析工具
    主要功能：
    1. 路径有效性验证
    2. 文件类型识别（文件/目录/其他）
    3. 基本元数据获取（大小、时间戳、权限）
    4. UTF-8文本文件行数统计
    5. 目录内容统计
    */

    std::string path = args.value("path", "");
    bool detailed = args.value("detailed", false);
    bool text_analysis = args.value("text_analysis", false);

    // 参数验证
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

    try {
        std::filesystem::path fs_path(path);

        // 检查路径是否存在
        if (!std::filesystem::exists(fs_path)) {
            return json{
                {"error", "Path not found: " + path},
                {"success", false}
            };
        }

        json result = {
            {"path", path},
            {"exists", true},
            {"absolute_path", std::filesystem::absolute(fs_path).string()},
            {"filename", fs_path.filename().string()}
        };

        // 文件类型识别
        if (std::filesystem::is_regular_file(fs_path)) {
            result["type"] = "file";

            // 文件大小信息
            auto file_size = std::filesystem::file_size(fs_path);
            result["size"] = file_size;

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
            result["human_size"] = ss.str();

            // 文件扩展名
            if (fs_path.has_extension()) {
                result["extension"] = fs_path.extension().string();
            }

            // UTF-8文本文件分析
            if (text_analysis) {
                std::string content;
                if (read_file_content(path, content)) {
                    // 统计行数
                    size_t line_count = 1; // 至少有一行
                    if (!content.empty()) {
                        line_count = std::count(content.begin(), content.end(), '\n') + 1;
                        // 如果文件以换行符结尾，行数减1
                        if (content.back() == '\n' && content.size() > 1) {
                            line_count--;
                        }
                    } else {
                        line_count = 0; // 空文件
                    }

                    result["text_stats"] = {
                        {"line_count", line_count},
                        {"character_count", content.size()},
                        {"is_text_readable", true}
                    };
                } else {
                    result["text_stats"] = {
                        {"is_text_readable", false},
                        {"error", "Cannot read file as text (may be binary or encoding issue)"}
                    };
                }
            }

        } else if (std::filesystem::is_directory(fs_path)) {
            result["type"] = "directory";

            // 目录统计（详细模式）
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

                    result["directory_stats"] = {
                        {"file_count", file_count},
                        {"subdirectory_count", dir_count},
                        {"total_size", total_size}
                    };
                } catch (const std::exception& e) {
                    result["directory_stats"] = {
                        {"error", "Failed to analyze directory contents: " + std::string(e.what())}
                    };
                }
            }
        } else {
            result["type"] = "other";
        }

        // 时间戳信息
        try {
            auto ftime = std::filesystem::last_write_time(fs_path);
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - std::filesystem::file_time_type::clock::now() +
                std::chrono::system_clock::now()
            );
            auto time_t = std::chrono::system_clock::to_time_t(sctp);

            std::ostringstream time_ss;
            time_ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
            result["last_modified"] = time_ss.str();
            result["last_modified_timestamp"] = time_t;
        } catch (...) {
            result["last_modified"] = "unknown";
            result["last_modified_timestamp"] = 0;
        }

        // 详细权限信息（跨平台Unix风格）
        if (detailed) {
            try {
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

                result["permissions"] = perm_str;
            } catch (...) {
                result["permissions"] = "unknown";
            }
        }

        result["success"] = true;
        return result;

    } catch (const std::filesystem::filesystem_error& e) {
        return json{
            {"error", "Filesystem error: " + std::string(e.what())},
            {"path", path},
            {"success", false}
        };
    } catch (const std::exception& e) {
        return json{
            {"error", "Failed to inspect path: " + std::string(e.what())},
            {"path", path},
            {"success", false}
        };
    }
}

json executeCheckUtf8Encoding(const json& args) {
    std::string path = args.value("path", "");

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

    try {
        std::filesystem::path fs_path(path);

        // 检查路径是否存在
        if (!std::filesystem::exists(fs_path)) {
            return json{
                {"error", "File not found: " + path},
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

        // 获取文件大小
        auto file_size = std::filesystem::file_size(fs_path);

        // 使用UTF-8验证函数检查文件
        bool is_utf8 = is_valid_utf8_file(path);

        // 构建结果
        json result;
        result["success"] = true;
        result["is_utf8"] = is_utf8;
        result["path"] = sanitize_string_for_json(path);
        result["file_size"] = static_cast<int64_t>(file_size);

        return result;

    } catch (const std::filesystem::filesystem_error& e) {
        return json{
            {"error", "Filesystem error: " + std::string(e.what())},
            {"success", false}
        };
    } catch (const std::exception& e) {
        return json{
            {"error", "Error checking UTF-8 encoding: " + std::string(e.what())},
            {"success", false}
        };
    } catch (...) {
        return json{
            {"error", "Unknown error occurred while checking UTF-8 encoding"},
            {"success", false}
        };
    }
}

} // namespace BuiltinTools
