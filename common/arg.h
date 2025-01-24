#pragma once
/*
Note:杨小兵-2025-01-24

1、该指令是一种预处理器指令，用于指导编译器在编译过程中只包含一次该头文件。
2、该指令主要作用就是防止头文件在同一个编译单元中被多次包含，从而避免因重复定义导致的编译错误。这种机制有效地替代了传统的包含保护（include guards），简化了代码编写。
3、该指令被主流编译器广泛支持，包括 GCC、Clang、MSVC 等。
4、该指令的一种实现方式：预处理器在内部将会自己维护一个该头文件的宏，从而判断是否在某个编译单元中是否重复包含该头文件。
5、从预处理器实现该指令的角度来说，有多种不同的方式，例如依赖宏的实现、依赖头文件路径的实现等等，但是最终实现的目的都是相同的。
*/
#include "common.h"

#include <set>
#include <string>
#include <vector>
/*
Note:杨小兵-2025-01-24

1、包含头文件，其目的就是为了减少重复，提高开发效率。
*/

//
// CLI argument parsing
//

struct common_arg {
    std::set<enum llama_example> examples = {LLAMA_EXAMPLE_COMMON};
    std::set<enum llama_example> excludes = {};
    std::vector<const char *> args;
    const char * value_hint   = nullptr; // help text or example for arg value
    const char * value_hint_2 = nullptr; // for second arg value
    const char * env          = nullptr;
    std::string help;
    bool is_sparam = false; // is current arg a sampling param?
    void (*handler_void)   (common_params & params) = nullptr;
    void (*handler_string) (common_params & params, const std::string &) = nullptr;
    void (*handler_str_str)(common_params & params, const std::string &, const std::string &) = nullptr;
    void (*handler_int)    (common_params & params, int) = nullptr;

    common_arg(
        const std::initializer_list<const char *> & args,
        const char * value_hint,
        const std::string & help,
        void (*handler)(common_params & params, const std::string &)
    ) : args(args), value_hint(value_hint), help(help), handler_string(handler) {}

    common_arg(
        const std::initializer_list<const char *> & args,
        const char * value_hint,
        const std::string & help,
        void (*handler)(common_params & params, int)
    ) : args(args), value_hint(value_hint), help(help), handler_int(handler) {}

    common_arg(
        const std::initializer_list<const char *> & args,
        const std::string & help,
        void (*handler)(common_params & params)
    ) : args(args), help(help), handler_void(handler) {}

    // support 2 values for arg
    common_arg(
        const std::initializer_list<const char *> & args,
        const char * value_hint,
        const char * value_hint_2,
        const std::string & help,
        void (*handler)(common_params & params, const std::string &, const std::string &)
    ) : args(args), value_hint(value_hint), value_hint_2(value_hint_2), help(help), handler_str_str(handler) {}

    common_arg & set_examples(std::initializer_list<enum llama_example> examples);
    common_arg & set_excludes(std::initializer_list<enum llama_example> excludes);
    common_arg & set_env(const char * env);
    common_arg & set_sparam();
    bool in_example(enum llama_example ex);
    bool is_exclude(enum llama_example ex);
    bool get_value_from_env(std::string & output);
    bool has_value_from_env();
    std::string to_string();
};
/*
Note:杨小兵-2025-01-24

1、自定义结构体
2、这里的common指的'通用的'
3、该结构体旨在描述一个命令行参数的属性和行为。它包含了参数的名称、相关提示、帮助信息、适用范围（哪些示例包含或排除该参数）、环境变量绑定，以及处理该参数的回调函数等。
4、该结构体提供了多个构造函数重载，以支持不同类型的参数：
    4.1 处理字符串值的参数
    4.2 处理整数值的参数
    4.3 无值参数
    4.4 支持两个值的参数
*/

struct common_params_context {
    enum llama_example ex = LLAMA_EXAMPLE_COMMON;
    common_params & params;
    std::vector<common_arg> options;
    void(*print_usage)(int, char **) = nullptr;
    common_params_context(common_params & params) : params(params) {}
};

// parse input arguments from CLI
// if one argument has invalid value, it will automatically display usage of the specific argument (and not the full usage message)
bool common_params_parse(int argc, char ** argv, common_params & params, llama_example ex, void(*print_usage)(int, char **) = nullptr);

// function to be used by test-arg-parser
common_params_context common_params_parser_init(common_params & params, llama_example ex, void(*print_usage)(int, char **) = nullptr);
