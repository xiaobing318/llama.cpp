#include "calculate.h"
#include "expression_parser.h"
#include "../common/tool_response.h"
#include "mathTools_utils.h"
#include <cctype>
#include <cmath>
#include <string>

namespace BuiltinTools {
namespace MathTools {

constexpr const char* k_tool_name = "calculate";

namespace common = builtin_tools::common;

/*
Note:
1、内置工具定义中的 R"( ... )" 是 raw string literal，它里面的换行符会被直接当成字符串里的 \n 存储。最终传给 nlohmann::json 的就是一个普通的 std::string，不会破坏 JSON 结构。
*/

// 内部辅助函数：检查字符串是否只包含允许的字符
static bool hasOnlyAllowedChars(const std::string& s) {
    for (unsigned char uc : s) {
        char c = static_cast<char>(uc);
        if (!std::isalnum(uc) && !std::isspace(uc) &&
            c != '+' && c != '-' && c != '*' && c != '/' && c != '%' &&
            c != '(' && c != ')' && c != '.' && c != ',' && c != '^' && c != '_') {
            return false;
        }
    }
    return true;
}

// 获取 calculate 工具的定义
ToolDefinition get_calculate_definition() {
    return {
        "calculate",
        {
            {"type", "function"},
            {"function", {
                {"name", "calculate"},
                {"description",
                    R"(Basic mathematical expression evaluator for arithmetic, functions, and constants. Designed for quick scientific/engineering math, not a general-purpose data calculator. Supports:
1) Operators: +, -, *, /, %, ^ (right-associative for exponent).
2) Functions: sin, cos, tan, asin, acos, atan, sinh, cosh, tanh, sqrt, log (base-10), ln (natural), exp, abs, floor, ceil, round, pow(a,b).
3) Constants: pi, e.
4) Parentheses and scientific notation (e.g., 1.5e-3).
Examples:
- 'sin(pi/4)'
- 'sqrt(pow(3,2)+pow(4,2))'
- '2*pi*3.5'
- 'log(100)/ln(10)'
- '(1+0.05)^12'
- 'abs(-5)+floor(3.7)'.)"},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"expression", {
                            {"type", "string"},
                            {"description", "A math expression using + - * / % ^, functions, constants, parentheses, and scientific notation. Radians are used for trigonometric functions."}
                        }}
                    }},
                    {"required", {"expression"}}
                }}
            }}
        }
    };
}

// 执行 calculate 工具
json run_calculate(const json& args) {
    // 提取参数，设置默认值
    std::string expression = args.value("expression", "");
    // 如果参数为空，则给出提示信息
    if (expression.empty()) {
        LOG_ERR("calculate: Empty expression provided");
        json err = common::make_error(k_tool_name, "Expression is required");
        err["requested"] = { {"expression", expression} };
        err["messages"] = json::array({"Missing 'expression' argument"});
        return err;
    }

    // 长度校验（使用 MathTools 专属校验工具，避免与全局 Utils 冲突）
    std::string error_message;
    if (!BuiltinTools::MathTools::Utils::validateStringLength(expression, 1000, "Expression", error_message)) {
        LOG_ERR("calculate: Expression too long: %zu characters", expression.length());
        json err = common::make_error(k_tool_name, error_message);
        err["requested"] = { {"expression", expression} };
        err["messages"] = json::array({"Expression exceeds maximum length"});
        return err;
    }

    // 基本字符白名单校验
    if (!hasOnlyAllowedChars(expression)) {
        LOG_ERR("calculate: Invalid character detected in expression: '%s'", expression.c_str());
        json err = common::make_error(k_tool_name, "Expression contains invalid characters");
        err["requested"] = { {"expression", expression} };
        err["messages"] = json::array({"Expression contains unsupported characters"});
        return err;
    }

    try {
        // 去空白，构造 cleaned 表达式，便于日志与回显
        std::string cleanExpr;
        cleanExpr.reserve(expression.size());
        for (unsigned char uc : expression) {
            if (!std::isspace(uc)) cleanExpr.push_back(static_cast<char>(uc));
        }
        // 空表达式检查
        if (cleanExpr.empty()) {
            LOG_ERR("calculate: Expression contains only whitespace: '%s'", expression.c_str());
            json err = common::make_error(k_tool_name, "Expression contains only whitespace");
            err["requested"] = { {"expression", expression} };
            err["messages"] = json::array({"Expression becomes empty after trimming whitespace"});
            return err;
        }

        // 括号匹配快速检查（提前给出更友好的报错）
        int paren = 0;
        for (char c : cleanExpr) {
            if (c == '(') ++paren;
            else if (c == ')') {
                --paren;
                if (paren < 0) {
                    // 输出错误日志和返回错误响应
                    LOG_ERR("calculate: Mismatched parentheses (too many closing) in '%s'", expression.c_str());
                    json err = common::make_error(k_tool_name, "Mismatched parentheses: too many closing parentheses");
                    err["requested"] = { {"expression", expression} };
                    err["cleaned_expression"] = cleanExpr;
                    err["messages"] = json::array({"Unbalanced parentheses"});
                    return err;
                }
            }
        }
        // 最终检查
        if (paren != 0) {
            LOG_ERR("calculate: Mismatched parentheses (unclosed opening) in '%s'", expression.c_str());
            json err = common::make_error(k_tool_name, "Mismatched parentheses: unclosed opening parentheses");
            err["requested"] = { {"expression", expression} };
            err["cleaned_expression"] = cleanExpr;
            err["messages"] = json::array({"Unbalanced parentheses"});
            return err;
        }
        // 输出提示日志，用来提醒开发者清洗前的表达式和清洗后的表达式
        LOG_INF("[calculate]工具: 解析/执行表达式: '%s' (清洗后: '%s')", expression.c_str(), cleanExpr.c_str());
        // 计算表达式
        double result = ExpressionParser::evaluateExpression(cleanExpr);
        // 检查结果是否为 NaN
        if (std::isnan(result)) {
            LOG_ERR("calculate: Result is NaN for '%s'", expression.c_str());
            return common::make_error(k_tool_name, "Invalid mathematical operation resulted in NaN");
        }
        // 检查结果是否为无穷大
        if (std::isinf(result)) {
            LOG_ERR("calculate: Result is infinite for '%s'", expression.c_str());
            return common::make_error(k_tool_name, "Mathematical operation resulted in infinity");
        }
        // 构造成功响应
        json response = common::make_success(k_tool_name);
        response["expression"] = expression;
        response["cleaned_expression"] = cleanExpr;
        response["result"] = result;
        return response;

    } catch (const std::runtime_error& e) {
        LOG_ERR("calculate: Mathematical evaluation error for '%s': %s", expression.c_str(), e.what());
        json err = common::make_error(k_tool_name, std::string("Mathematical evaluation error: ") + e.what());
        err["requested"] = { {"expression", expression} };
        err["messages"] = json::array({"Evaluation failed due to invalid domain or operation"});
        return err;
    } catch (const std::exception& e) {
        LOG_ERR("calculate: Unexpected error for '%s': %s", expression.c_str(), e.what());
        json err = common::make_error(k_tool_name, std::string("Unexpected error: ") + e.what());
        err["requested"] = { {"expression", expression} };
        err["messages"] = json::array({"Unexpected exception during evaluation"});
        return err;
    } catch (...) {
        LOG_ERR("calculate: Unknown error occurred during expression evaluation for '%s'", expression.c_str());
        json err = common::make_error(k_tool_name, "Unknown error occurred during expression evaluation");
        err["requested"] = { {"expression", expression} };
        err["messages"] = json::array({"Unknown error during evaluation"});
        return err;
    }
}

} // namespace MathTools
} // namespace BuiltinTools
