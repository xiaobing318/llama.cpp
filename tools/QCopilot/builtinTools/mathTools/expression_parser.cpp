#include "expression_parser.h"
#include <cmath>
#include <stdexcept>
#include <cctype>

namespace BuiltinTools {
namespace MathTools {

// 计算工具将会用到的常量
const double ExpressionParser::PI = 3.14159265358979323846;
const double ExpressionParser::E = 2.71828182845904523536;

double ExpressionParser::evaluateExpression(const std::string& expr) {
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

double ExpressionParser::parseExpression(const std::string& expr, size_t& pos) {
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

double ExpressionParser::parseTerm(const std::string& expr, size_t& pos) {
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

double ExpressionParser::parseFactor(const std::string& expr, size_t& pos) {
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

double ExpressionParser::parseFunction(const std::string& funcName, const std::string& expr, size_t& pos) {
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

void ExpressionParser::skipWhitespace(const std::string& expr, size_t& pos) {
    while (pos < expr.length() && std::isspace(expr[pos])) {
        pos++;
    }
}

bool ExpressionParser::isFunction(const std::string& name) {
    static const std::set<std::string> functions = {
        "sin", "cos", "tan", "sqrt", "log", "ln", "exp", "abs",
        "floor", "ceil", "round", "pow", "asin", "acos", "atan",
        "sinh", "cosh", "tanh"
    };
    return functions.find(name) != functions.end();
}

} // namespace MathTools
} // namespace BuiltinTools