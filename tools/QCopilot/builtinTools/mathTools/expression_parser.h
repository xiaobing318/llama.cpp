#pragma once

#include <string>
#include <set>

namespace BuiltinTools {
namespace MathTools {

// 数学表达式解析器
class ExpressionParser {
public:
    // 计算表达式的主函数
    static double evaluateExpression(const std::string& expr);

private:
    // 数学常量
    static const double PI;
    static const double E;

    // 解析表达式（处理 +, - 运算符）
    static double parseExpression(const std::string& expr, size_t& pos);
    
    // 解析项（处理 *, /, % 运算符）
    static double parseTerm(const std::string& expr, size_t& pos);
    
    // 解析因子（数字、常量、函数、括号表达式）
    static double parseFactor(const std::string& expr, size_t& pos);
    
    // 解析数学函数调用
    static double parseFunction(const std::string& funcName, const std::string& expr, size_t& pos);
    
    // 跳过空白字符
    static void skipWhitespace(const std::string& expr, size_t& pos);
    
    // 检查字符串是否是数学函数
    static bool isFunction(const std::string& name);
};

} // namespace MathTools
} // namespace BuiltinTools