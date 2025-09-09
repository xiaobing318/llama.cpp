#pragma once
#include <string>
#include <cstddef>

namespace BuiltinTools {
namespace MathTools {

/*
1、数学表达式解析器（递归下降）
2、文法（简化）：
    Expr    := Term { ('+' | '-') Term }*
    Term    := Power { ('*' | '/' | '%') Power }*
    Power   := Unary { '^' Unary }*         // 右结合
    Unary   := ('+' | '-') Unary | Primary
    Primary := Number | Constant | FunctionCall | '(' Expr ')'
    Number  := digits [ '.' digits ] [ [eE] [+-]? digits ]
    Constant:= 'pi' | 'e'
    FunctionCall := name '(' [Expr [',' Expr]] ')' // 目前只有 pow(a,b) 双参，其余单参
*/
class ExpressionParser {
public:
    static double evaluateExpression(const std::string& expr);

private:
    static const double PI;
    static const double E;

    // 递归子解析
    static double parseExpression(const std::string& expr, size_t& pos);
    static double parseTerm(const std::string& expr, size_t& pos);
    static double parsePower(const std::string& expr, size_t& pos);
    static double parseUnary(const std::string& expr, size_t& pos);
    static double parsePrimary(const std::string& expr, size_t& pos);

    // 词法辅助
    static void skipWhitespace(const std::string& expr, size_t& pos);
    static bool parseNumber(const std::string& expr, size_t& pos, double& out);
    static std::string parseName(const std::string& expr, size_t& pos);

    // 函数解析
    static bool isFunction(const std::string& lowerName);
    static double parseFunctionCall(const std::string& lowerName, const std::string& expr, size_t& pos);

    // 小工具
    static void expectChar(const std::string& expr, size_t& pos, char ch, const char* err);
};

} // namespace MathTools
} // namespace BuiltinTools
