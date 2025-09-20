#include "expression_parser.h"
#include <cmath>
#include <stdexcept>
#include <cctype>
#include <algorithm>

namespace BuiltinTools {
namespace MathTools {

// 常量定义
const double ExpressionParser::PI = 3.141592653589793238462643383279502884;
const double ExpressionParser::E  = 2.718281828459045235360287471352662497;

// 内部辅助函数： 判断字符是否为标识符起始字符或组成字符
static inline bool isNameStart(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

// 内部辅助函数： 判断字符是否为标识符组成字符
static inline bool isNameChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

// 执行表达式计算的入口
double ExpressionParser::evaluateExpression(const std::string& expr) {
    if (expr.empty()) throw std::runtime_error("Empty expression");
    size_t pos = 0;
    double v = parseExpression(expr, pos);
    skipWhitespace(expr, pos);
    if (pos != expr.size()) {
        throw std::runtime_error("Unexpected characters at end: '" + expr.substr(pos) + "'");
    }
    return v;
}

// 解析表达式的递归下降实现
double ExpressionParser::parseExpression(const std::string& expr, size_t& pos) {
    double left = parseTerm(expr, pos);
    while (true) {
        skipWhitespace(expr, pos);
        if (pos >= expr.size()) break;
        char op = expr[pos];
        if (op != '+' && op != '-') break;
        ++pos;
        double right = parseTerm(expr, pos);
        left = (op == '+') ? (left + right) : (left - right);
    }
    return left;
}

// 解析项
double ExpressionParser::parseTerm(const std::string& expr, size_t& pos) {
    double left = parsePower(expr, pos);
    while (true) {
        skipWhitespace(expr, pos);
        if (pos >= expr.size()) break;
        char op = expr[pos];
        if (op != '*' && op != '/' && op != '%') break;
        ++pos;
        double right = parsePower(expr, pos);
        if (op == '*') {
            left *= right;
        } else if (op == '/') {
            if (right == 0.0) throw std::runtime_error("Division by zero");
            left /= right;
        } else { // '%'
            if (right == 0.0) throw std::runtime_error("Modulo by zero");
            left = std::fmod(left, right);
        }
    }
    return left;
}

// 幂：右结合，例如 2^3^2 == 2^(3^2)
double ExpressionParser::parsePower(const std::string& expr, size_t& pos) {
    double base = parseUnary(expr, pos);
    while (true) {
        skipWhitespace(expr, pos);
        if (pos >= expr.size() || expr[pos] != '^') break;
        ++pos;
        double exp = parseUnary(expr, pos);
        base = std::pow(base, exp);
    }
    return base;
}

double ExpressionParser::parseUnary(const std::string& expr, size_t& pos) {
    skipWhitespace(expr, pos);
    if (pos < expr.size() && (expr[pos] == '+' || expr[pos] == '-')) {
        char op = expr[pos++];
        double v = parseUnary(expr, pos);
        return (op == '-') ? -v : v;
    }
    return parsePrimary(expr, pos);
}

double ExpressionParser::parsePrimary(const std::string& expr, size_t& pos) {
    skipWhitespace(expr, pos);
    if (pos >= expr.size()) throw std::runtime_error("Unexpected end of expression");

    // 括号
    if (expr[pos] == '(') {
        ++pos;
        double v = parseExpression(expr, pos);
        expectChar(expr, pos, ')', "Expected ')'");
        return v;
    }

    // 数字
    double num = 0.0;
    size_t save = pos;
    if (parseNumber(expr, pos, num)) {
        return num;
    }
    pos = save;

    // 常量/函数
    if (isNameStart(expr[pos])) {
        std::string name = parseName(expr, pos);
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c){ return std::tolower(c); });

        if (lower == "pi") return PI;
        if (lower == "e")  return E;

        if (isFunction(lower)) {
            return parseFunctionCall(lower, expr, pos);
        }
        throw std::runtime_error("Unknown identifier: " + name);
    }

    throw std::runtime_error(std::string("Unexpected character: '") + expr[pos] + "'");
}

bool ExpressionParser::parseNumber(const std::string& expr, size_t& pos, double& out) {
    skipWhitespace(expr, pos);
    size_t i = pos;
    bool hasDigit = false;

    // integer part
    while (i < expr.size() && std::isdigit(static_cast<unsigned char>(expr[i]))) {
        hasDigit = true;
        ++i;
    }
    // fractional part
    if (i < expr.size() && expr[i] == '.') {
        ++i;
        while (i < expr.size() && std::isdigit(static_cast<unsigned char>(expr[i]))) {
            hasDigit = true;
            ++i;
        }
    }
    if (!hasDigit) return false;

    // exponent part
    if (i < expr.size() && (expr[i] == 'e' || expr[i] == 'E')) {
        size_t j = i + 1;
        if (j < expr.size() && (expr[j] == '+' || expr[j] == '-')) ++j;
        size_t jStart = j;
        while (j < expr.size() && std::isdigit(static_cast<unsigned char>(expr[j]))) ++j;
        if (j == jStart) {
            // 'e' 后没有有效数字，不把它当指数，仍交给 stod 报错更合理
        } else {
            i = j;
        }
    }

    try {
        out = std::stod(expr.substr(pos, i - pos));
        pos = i;
        return true;
    } catch (...) {
        return false;
    }
}

// 解析标识符（常量名或函数名）
std::string ExpressionParser::parseName(const std::string& expr, size_t& pos) {
    size_t start = pos;
    while (pos < expr.size() && isNameChar(expr[pos])) ++pos;
    return expr.substr(start, pos - start);
}

// 判断是否为已知函数
bool ExpressionParser::isFunction(const std::string& lowerName) {
    // 单参 + pow 双参
    static const char* kFuncs[] = {
        "sin","cos","tan","asin","acos","atan",
        "sinh","cosh","tanh",
        "sqrt","log","ln","exp","abs","floor","ceil","round",
        "pow"
    };
    for (auto* f : kFuncs) {
        if (lowerName == f) return true;
    }
    return false;
}

// 解析函数调用
double ExpressionParser::parseFunctionCall(const std::string& lowerName, const std::string& expr, size_t& pos) {
    // 注意：此时 pos 已经位于函数名后一个字符处，通常应当是 '('
    skipWhitespace(expr, pos);
    expectChar(expr, pos, '(', "Expected '(' after function name");

    if (lowerName == "pow") {
        double a = parseExpression(expr, pos);
        skipWhitespace(expr, pos);
        expectChar(expr, pos, ',', "Expected ',' in pow(a,b)");
        double b = parseExpression(expr, pos);
        skipWhitespace(expr, pos);
        expectChar(expr, pos, ')', "Expected ')' after function arguments");
        return std::pow(a, b);
    }

    // 单参函数
    double v = parseExpression(expr, pos);
    skipWhitespace(expr, pos);
    expectChar(expr, pos, ')', "Expected ')' after function argument");

    // 域检查与计算
    if (lowerName == "sin")   return std::sin(v);
    if (lowerName == "cos")   return std::cos(v);
    if (lowerName == "tan")   return std::tan(v);

    if (lowerName == "asin") {
        if (v < -1.0 || v > 1.0) throw std::runtime_error("asin argument out of range [-1,1]");
        return std::asin(v);
    }
    if (lowerName == "acos") {
        if (v < -1.0 || v > 1.0) throw std::runtime_error("acos argument out of range [-1,1]");
        return std::acos(v);
    }
    if (lowerName == "atan")  return std::atan(v);

    if (lowerName == "sinh")  return std::sinh(v);
    if (lowerName == "cosh")  return std::cosh(v);
    if (lowerName == "tanh")  return std::tanh(v);

    if (lowerName == "sqrt") {
        if (v < 0.0) throw std::runtime_error("sqrt of negative number");
        return std::sqrt(v);
    }
    if (lowerName == "log") {
        if (v <= 0.0) throw std::runtime_error("log of non-positive number");
        return std::log10(v);
    }
    if (lowerName == "ln") {
        if (v <= 0.0) throw std::runtime_error("ln of non-positive number");
        return std::log(v);
    }
    if (lowerName == "exp")   return std::exp(v);
    if (lowerName == "abs")   return std::fabs(v);
    if (lowerName == "floor") return std::floor(v);
    if (lowerName == "ceil")  return std::ceil(v);
    if (lowerName == "round") return std::round(v);

    throw std::runtime_error("Unknown function: " + lowerName);
}

// 跳过空白字符
void ExpressionParser::skipWhitespace(const std::string& expr, size_t& pos) {
    while (pos < expr.size() && std::isspace(static_cast<unsigned char>(expr[pos]))) ++pos;
}

// 期望遇到特定字符，否则抛出异常
void ExpressionParser::expectChar(const std::string& expr, size_t& pos, char ch, const char* err) {
    skipWhitespace(expr, pos);
    if (pos >= expr.size() || expr[pos] != ch) {
        throw std::runtime_error(err);
    }
    ++pos;
}

} // namespace MathTools
} // namespace BuiltinTools
