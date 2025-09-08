#include "calculate.h"
#include "expression_parser.h"
#include "../common/common_utils.h"
#include "mathTools_utils.h"
#include <cctype>
#include <cmath>

namespace BuiltinTools {
namespace MathTools {

ToolDefinition getCalculateDefinition() {
    return {
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
    };
}

json executeCalculate(const json& args) {
    std::string expression = args.value("expression", "");

    if (expression.empty()) {
        LOG_ERR("calculate: Empty expression provided");
        return BuiltinTools::Utils::createErrorResponse("Expression is required");
    }

    // 输入验证：检查表达式长度是否合理
    std::string error_message;
    if (!Utils::validateStringLength(expression, 1000, "Expression", error_message)) {
        LOG_ERR("calculate: Expression too long: %zu characters", expression.length());
        return BuiltinTools::Utils::createErrorResponse(error_message);
    }

    // 基本字符验证：确保只包含允许的字符
    for (char c : expression) {
        if (!std::isalnum(c) && !std::isspace(c) &&
            c != '+' && c != '-' && c != '*' && c != '/' && c != '%' &&
            c != '(' && c != ')' && c != '.' && c != ',' && c != '^') {
            LOG_ERR("calculate: Invalid character in expression: '%c' in '%s'", c, expression.c_str());
            return BuiltinTools::Utils::createErrorResponse(std::string("Invalid character in expression: '") + c + "'");
        }
    }

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
            LOG_ERR("calculate: Expression contains only whitespace: '%s'", expression.c_str());
            return BuiltinTools::Utils::createErrorResponse("Expression contains only whitespace");
        }

        // 基本语法检查：检查括号是否匹配
        int parentheses_count = 0;
        for (char c : cleanExpr) {
            if (c == '(') parentheses_count++;
            else if (c == ')') parentheses_count--;
            if (parentheses_count < 0) {
                LOG_ERR("calculate: Mismatched parentheses (too many closing) in expression: '%s'", expression.c_str());
                return BuiltinTools::Utils::createErrorResponse("Mismatched parentheses: too many closing parentheses");
            }
        }
        if (parentheses_count != 0) {
            LOG_ERR("calculate: Mismatched parentheses (unclosed opening) in expression: '%s'", expression.c_str());
            return BuiltinTools::Utils::createErrorResponse("Mismatched parentheses: unclosed opening parentheses");
        }

        double result = ExpressionParser::evaluateExpression(cleanExpr);

        // 检查结果是否有效
        if (std::isnan(result)) {
            LOG_ERR("calculate: Expression resulted in NaN: '%s'", expression.c_str());
            return BuiltinTools::Utils::createErrorResponse("Invalid mathematical operation resulted in NaN (Not a Number)");
        }

        if (std::isinf(result)) {
            LOG_ERR("calculate: Expression resulted in infinity: '%s'", expression.c_str());
            return BuiltinTools::Utils::createErrorResponse("Mathematical operation resulted in infinity");
        }

        LOG_INF("calculate: Successfully evaluated '%s' = %g", cleanExpr.c_str(), result);
        json response = BuiltinTools::Utils::createSuccessResponse();
        response["expression"] = expression;
        response["cleaned_expression"] = cleanExpr;
        response["result"] = result;

        return response;

    } catch (const std::runtime_error& e) {
        LOG_ERR("calculate: Mathematical evaluation error for '%s': %s", expression.c_str(), e.what());
        return BuiltinTools::Utils::createErrorResponse(std::string("Mathematical evaluation error: ") + e.what());
    } catch (const std::exception& e) {
        LOG_ERR("calculate: Unexpected error for '%s': %s", expression.c_str(), e.what());
        return BuiltinTools::Utils::createErrorResponse(std::string("Unexpected error: ") + e.what());
    } catch (...) {
        LOG_ERR("calculate: Unknown error occurred during expression evaluation for '%s'", expression.c_str());
        return BuiltinTools::Utils::createErrorResponse("Unknown error occurred during expression evaluation");
    }
}

} // namespace MathTools
} // namespace BuiltinTools
