#include "test_support.h"
#include "../builtin_tools/mathTools/calculate.h"
#include <cmath>

using json = nlohmann::ordered_json;

static bool approx(double a, double b, double eps=1e-9) { return std::fabs(a-b) <= eps * (1.0 + std::fabs(a)); }

int main(){
    qctest::Test T;

    auto ok = [&](const std::string &expr, double expect, double eps=1e-6){
        auto r = BuiltinTools::MathTools::run_calculate({{"expression", expr}});
        if (!(r.value("success", false) == true)) { T.check(false, std::string("success expected: ")+expr); return; }
        qctest::expect_json_schema(r, {{"success","boolean"},{"expression","string"},{"cleaned_expression","string"},{"result","number"}}, T, "calculate schema");
        double v = r.value("result", 0.0);
        T.check(approx(v, expect, eps), std::string("result mismatch for ")+expr+", got="+std::to_string(v));
    };
    auto bad = [&](const std::string &expr){
        auto r = BuiltinTools::MathTools::run_calculate({{"expression", expr}});
        T.check(r.value("success", true) == false, std::string("expect failure: ")+expr);
    };

    // 基本算术
    ok("1+2*3", 7);
    ok("(1+2)*3", 9);
    ok("2^3^2", std::pow(2.0, std::pow(3.0,2.0))); // 右结合
    ok("10%3", std::fmod(10.0,3.0));

    // 常量与函数
    ok("sin(pi/4)", std::sin(std::acos(-1.0)/4.0), 1e-6);
    ok("sqrt(pow(3,2)+pow(4,2))", 5.0);
    ok("log(100)/ln(10)", 2.0, 1e-6);
    ok("abs(-5)+floor(3.7)", 8.0);

    // 科学计数法
    ok("1.5e-3 * 2e3", 3.0);

    // 错误场景
    bad("");                 // 空
    bad("   ");              // 空白
    bad("(1+2");            // 括号不匹配
    bad("sqrt(-1)");        // 域错误
    bad("asin(2)");         // 域错误
    bad("1/0");             // 无穷
    bad("x+1");             // 未知标识符

    return T.finish();
}
