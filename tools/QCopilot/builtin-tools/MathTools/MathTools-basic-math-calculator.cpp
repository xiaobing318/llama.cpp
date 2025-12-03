#include "MathTools-basic-math-calculator.h"
#include "expression-parser.h"
#include "../common/common-response.h"
#include "../../qcopilot-utils.h"
#include "MathTools-utils.h"
#include <cctype>
#include <cmath>
#include <string>

namespace BuiltinTools {
namespace MathTools {

constexpr const char* k_tool_name = "basic_math_calculator";

// 内部辅助函数：检查字符串是否只包含允许的字符
static bool has_only_allowed_chars(const std::string& s) {
    // 循环对给定字符串中的每一个字符进行检查
    for (unsigned char uc : s) {
        // 将无符号字符转化为有符号字符以便检查
        char c = static_cast<char>(uc);
        // 检查字符是否为数字、空格、运算符或者一些被允许的字符
        if (!std::isalnum(uc) &&
            !std::isspace(uc) &&
            c != '+' &&
            c != '-' &&
            c != '*' &&
            c != '/' &&
            c != '%' &&
            c != '(' &&
            c != ')' &&
            c != '.' &&
            c != ',' &&
            c != '^' &&
            c != '_') {
            // 如果当前检查的字符不属于上述白名单中的字符则立即返回 false
            return false;
        }
    }
    // 如果所有字符都通过了上述检查则返回 true
    return true;
}

// 获取 basic_math_calculator 工具的定义
BuiltinTools::Types::ToolDefinition get_basic_math_calculator_definition() {
    return {
        std::string(k_tool_name),
        {
            {"type", "function"},
            {"function", {
                {"name", std::string(k_tool_name)},
                {"description",
R"(basic_math_calculator：一个面向对话/工具链场景的轻量数学表达式求值器。

一、使用场景
- 作为 Chat/Agent 的内置计算器：对 LLM 生成的单条数学表达式进行快速、可控的数值计算。
- 工程/科研/学习中的临时计算：在无需引入大型数值库的前提下完成常见算术与初等函数的计算。
- 表达式合法性校验与可读报错：对非法字符、括号不平衡、越界取值（如 log/ln <= 0、sqrt < 0、asin/acos 超出 [-1,1]）等给出结构化错误信息。

二、能做什么 / 不能做什么
- 能做
  - 运算符与语法：加减乘除(+ - * /)、取模(%)、幂(^，右结合)、一元正负号、括号分组、科学计数法(如 1.2e-3)。
  - 函数：sin, cos, tan, asin, acos, atan, sinh, cosh, tanh, sqrt, log(以 10 为底), ln(自然对数), exp, abs, floor, ceil, round, pow(a,b)。三角函数使用弧度制。
  - 常量：pi, e。
  - 健壮性：忽略空白；对常见数学域错误与除零进行检测并返回明晰错误；字符白名单限制避免注入与无关字符。
- 不能做
  - 代数/符号能力：不支持符号推导、化简、方程/方程组求解、微积分、极限、矩阵/向量/复数运算。
  - 语言/语义扩展：不支持变量定义与赋值、用户自定义函数/常量、多语句/多表达式一次性求值、单位换算或维度分析、角度制(需自行转换为弧度)。
  - 系统交互：不进行文件/网络/系统调用，不维护跨调用状态。

三、使用例子（均为弧度制）
- sin(pi/4)               -> 0.70710678
- sqrt(pow(3,2)+pow(4,2)) -> 5
- 2*pi*3.5                -> 21.9911486
- log(100)/ln(10)         -> 1           // log 为以 10 为底
- (1+0.05)^12             -> 1.79585633
- abs(-5)+floor(3.7)      -> 8
- exp(1)                  -> 2.71828183
- 1.5e-3 * 2^10           -> 1.536
)"
                },
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"expression", {
                            {"type", "string"},
                            {"description", "用于求值的数学表达式（弧度制）。支持 + - * / % ^（^ 右结合）、括号、科学计数法、常量 pi/e，以及函数 sin/cos/tan/asin/acos/atan/sinh/cosh/tanh/sqrt/log(10)/ln/exp/abs/floor/ceil/round/pow(a,b)。默认值：空字符串（缺失或为空会返回错误响应）。"}
                        }}
                    }},
                    {"required", {"expression"}}
                }}
            }}
        }
    };
}

// 执行 basic_math_calculator 工具的实现
json run_basic_math_calculator(const json& args) {
    // 从参数中提取表达式，如果缺失则默认为空字符串
    std::string expression = args.value("expression", "");
    // 如果参数为空，则给出提示信息并返回错误响应
    if (expression.empty()) {
        // 输入日志信息到终端供开发者调试
        LOG_ERR("[%s] %s", k_tool_name, "Empty expression provided");

        // 构造并返回错误响应，包含请求的参数和错误信息
        json err = BuiltinTools::Common::make_error(k_tool_name, "The expression is empty, but the tool must require a valid expression");
        err["requested"] = { {"expression", expression} };
        err["messages"] = json::array({"Missing 'expression' argument"});
        return err;
    }

    // 表达式长度校验
    std::string error_message;
    if (!BuiltinTools::MathTools::Utils::validateStringLength(expression, 1000, "Expression", error_message)) {
        // 输入日志信息到终端供开发者调试
        LOG_ERR("[%s] Expression too long: %zu characters", k_tool_name, expression.length());

        // 构造并返回错误响应，包含请求的参数和错误信息
        json err = BuiltinTools::Common::make_error(k_tool_name, error_message);
        err["requested"] = { {"expression", expression} };
        err["messages"] = json::array({"Expression exceeds maximum length"});
        return err;
    }

    // 验证表达式中是否存在不被允许的字符
    if (!has_only_allowed_chars(expression)) {
        // 输入日志信息到终端供开发者调试
        LOG_ERR("[%s] Invalid character detected in expression: '%s'", k_tool_name, expression.c_str());

        // 构造并返回错误响应，包含请求的参数和错误信息
        json err = BuiltinTools::Common::make_error(k_tool_name, "Expression contains invalid characters");
        err["requested"] = { {"expression", expression} };
        err["messages"] = json::array({"Expression contains unsupported characters"});
        return err;
    }

    try {
        // 对表达式进行清洗，构造 cleaned 表达式
        std::string cleanExpr;
        // 预分配内存以提升性能
        cleanExpr.reserve(expression.size());
        // 去除所有空白字符
        for (unsigned char uc : expression) {
            if (!std::isspace(uc)){
                cleanExpr.push_back(static_cast<char>(uc));
            }
        }
        // 空表达式检查
        if (cleanExpr.empty()) {
            // 输入日志信息到终端供开发者调试
            LOG_ERR("[%s] Expression contains only whitespace: '%s'", k_tool_name, expression.c_str());

            // 构造并返回错误响应，包含请求的参数和错误信息
            json err = BuiltinTools::Common::make_error(k_tool_name, "Expression contains only whitespace");
            err["requested"] = { {"expression", expression} };
            err["messages"] = json::array({"Expression becomes empty after trimming whitespace"});
            return err;
        }

        // 括号匹配快速检查，提前给出更友好的报错
        int paren = 0;
        // 循环检查表达式中的每个字符
        for (char c : cleanExpr) {
            if (c == '('){
                ++paren;
            }
            else if (c == ')') {
                --paren;
                if (paren < 0) {
                    // 输入日志信息到终端供开发者调试
                    LOG_ERR("[%s] Mismatched parentheses (too many closing) in '%s'", k_tool_name, expression.c_str());

                    // 构造并返回错误响应，包含请求的参数和错误信息
                    json err = BuiltinTools::Common::make_error(k_tool_name, "Mismatched parentheses: too many closing parentheses");
                    err["requested"] = { {"expression", expression} };
                    err["cleaned_expression"] = cleanExpr;
                    err["messages"] = json::array({"Unbalanced parentheses"});
                    return err;
                }
            }
        }
        // 最终检查
        if (paren != 0) {
            // 输入日志信息到终端供开发者调试
            LOG_ERR("[%s] Mismatched parentheses (unclosed opening) in '%s'", k_tool_name, expression.c_str());

            // 构造并返回错误响应，包含请求的参数和错误信息
            json err = BuiltinTools::Common::make_error(k_tool_name, "Mismatched parentheses: unclosed opening parentheses");
            err["requested"] = { {"expression", expression} };
            err["cleaned_expression"] = cleanExpr;
            err["messages"] = json::array({"Unbalanced parentheses"});
            return err;
        }
        // 输出提示日志，用来提醒开发者清洗前的表达式和清洗后的表达式
        LOG_INF("[%s] Parsing/Executing Expressions: '%s' (After cleaning: '%s')", k_tool_name, expression.c_str(), cleanExpr.c_str());
        // 计算表达式
        double result = ExpressionParser::evaluateExpression(cleanExpr);
        // 检查结果是否为 NaN
        if (std::isnan(result)) {
            // 输入日志信息到终端供开发者调试
            LOG_ERR("[%s] Result is NaN for '%s'", k_tool_name, expression.c_str());
            return BuiltinTools::Common::make_error(k_tool_name, "Invalid mathematical operation resulted in NaN");
        }
        // 检查结果是否为无穷大
        if (std::isinf(result)) {
            LOG_ERR("[%s] Result is infinite for '%s'", k_tool_name, expression.c_str());
            return BuiltinTools::Common::make_error(k_tool_name, "Mathematical operation resulted in infinity");
        }
        // 构造成功响应
        json response = BuiltinTools::Common::make_success(k_tool_name);
        response["expression"] = expression;
        response["cleaned_expression"] = cleanExpr;
        response["result"] = result;
        return response;

    } catch (const std::runtime_error& e) {
        LOG_ERR("[%s] Mathematical evaluation error for '%s': %s", k_tool_name, expression.c_str(), e.what());

        json err = BuiltinTools::Common::make_error(k_tool_name, std::string("Mathematical evaluation error: ") + e.what());
        err["requested"] = { {"expression", expression} };
        err["messages"] = json::array({"Evaluation failed due to invalid domain or operation"});
        return err;
    } catch (const std::exception& e) {
        LOG_ERR("[%s] Unexpected error for '%s': %s", k_tool_name, expression.c_str(), e.what());

        json err = BuiltinTools::Common::make_error(k_tool_name, std::string("Unexpected error: ") + e.what());
        err["requested"] = { {"expression", expression} };
        err["messages"] = json::array({"Unexpected exception during evaluation"});
        return err;
    } catch (...) {
        LOG_ERR("[%s] Unknown error occurred during expression evaluation for '%s'", k_tool_name, expression.c_str());

        json err = BuiltinTools::Common::make_error(k_tool_name, "Unknown error occurred during expression evaluation");
        err["requested"] = { {"expression", expression} };
        err["messages"] = json::array({"Unknown error during evaluation"});
        return err;
    }
}

} // namespace MathTools
} // namespace BuiltinTools
