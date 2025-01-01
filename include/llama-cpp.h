#pragma once
/*
Note:杨小兵-2024-12-30

1、`#pragma once` 是一个预处理指令，用于防止头文件被多次包含，从而避免重复定义的问题。它确保编译器在一个编译单元中只包含一次该头文件，无论该头文件被引用了多少次。
2、#pragma once预处理指令在不同编译器中的支持情况：
    2.1 Microsoft 的 Visual C++ 编译器（MSVC）支持 #pragma once 预处理指令。MSVC 从早期版本（如 Visual Studio 5.0）开始就已经支持该指令，并且在后续的所有版本中继续保持支持。
    2.2 GNU Compiler Collection（GCC）支持 #pragma once 预处理指令。GCC 从 版本 3.4 开始引入对 #pragma once 的支持，并在之后的所有版本中持续支持该指令。
    2.3 Clang 编译器支持 #pragma once 预处理指令。Clang 从其早期版本（大约从 1.0 版本起）就已经实现了对 #pragma once 的支持，并在后续的所有版本中继续支持该指令。
*/

#ifndef __cplusplus
#error "This header is for C++ only"
#endif
/*
Note:杨小兵-2025-01-01

1、__cplusplus` 是一个**预定义的宏（Predefined Macro）**，由C++编译器自动定义。它用于标识当前的编译环境是否为C++，以及使用的C++标准版本。
2、编译器自动定义：当编译器以C++模式编译代码时，会自动定义 `__cplusplus` 宏。该宏在编译过程的预处理阶段被定义，并且可以在代码的任何地方使用（通常在预处理指令中）
*/

#include <memory>
/*
Note:杨小兵-2025-01-01

1、<memory> 是C++标准库中的一个头文件，位于 <cstddef> 之后，提供了多种与内存管理相关的工具和类。其主要功能包括：
    1.1 智能指针（Smart Pointers）：如 std::unique_ptr、std::shared_ptr、std::weak_ptr，用于自动管理动态分配的内存，避免内存泄漏。
    1.2 内存管理工具：如 std::allocator，用于自定义内存分配策略。
    1.3 其他辅助工具：如 std::enable_shared_from_this，用于在类中获取 std::shared_ptr。
2、#include <memory> 头文件在C++中扮演着至关重要的角色，提供了智能指针和内存管理工具，帮助开发者高效、安全地管理动态内存。通过使用 <memory> 中提供的工具，可以显著减少内存泄漏和其他内存管理相关的错误，提高代码的健壮性和可维护性。
*/
#include "llama.h"
/*
Note:杨小兵-2025-01-01

1、包含的头文件是 `llama.h`，该头文件是LLAMA库的C接口头文件，定义了LLAMA库的函数接口和数据结构。
*/

struct llama_model_deleter {
    void operator()(llama_model * model) { llama_free_model(model); }
};


struct llama_context_deleter {
    void operator()(llama_context * context) { llama_free(context); }
};

struct llama_sampler_deleter {
    void operator()(llama_sampler * sampler) { llama_sampler_free(sampler); }
};
/*
Note:杨小兵-2025-01-01

1、struct和classs之间的对比
    1.1 相同点：
        1.1.1 成员功能： struct和class都可以包含成员变量、成员函数（包括构造函数、析构函数、重载运算符等）、访问说明符（public、protected、private）、继承（包括多继承）等。
        1.1.2 继承机制： 两者在继承时都支持相同的继承方式（公有继承、保护继承、私有继承）。
        1.1.3 访问控制： 都支持相同的访问控制机制。
    1.2 主要区别：
        1.2.1 默认访问权限：
            1.2.1.1 struct： 默认的成员访问权限是 public。
            1.2.1.3 class： 默认的成员访问权限是 private。
        1.2.2 默认继承权限：
            1.2.2.1 struct： 默认的继承方式是 public。
            1.2.2.2 class： 默认的继承方式是 private。
2、declaration(forward declaration) and definition
    2.1 struct struct_name; 是前向声明，用于告诉编译器存在一个名为 struct_name 的结构体，但不提供其具体内容。
    2.2 完整的 struct 定义包含成员变量和/或成员函数，使用语法 struct struct_name {  成员  };。
3、运算符重载语法
    return_type operatorop(parameters) {
        // 实现
    }
*/

typedef std::unique_ptr<llama_model, llama_model_deleter> llama_model_ptr;
typedef std::unique_ptr<llama_context, llama_context_deleter> llama_context_ptr;
typedef std::unique_ptr<llama_sampler, llama_sampler_deleter> llama_sampler_ptr;
/*
Note:杨小兵-2025-01-01

1、这三行代码使用typedef为带有自定义删除器的std::unique_ptr智能指针定义了别名。这样可以简化代码中的类型声明，使代码更具可读性和可维护性。具体来说：
    llama_model_ptr：是std::unique_ptr指向llama_model对象，并使用llama_model_deleter作为删除器的别名。
    llama_context_ptr：是std::unique_ptr指向llama_context对象，并使用llama_context_deleter作为删除器的别名。
    llama_sampler_ptr：是std::unique_ptr指向llama_sampler对象，并使用llama_sampler_deleter作为删除器的别名。
2、typedef 关键字
    2.1 含义： typedef是C++中的一个关键字，用于为现有类型创建一个新的别名。它可以使复杂的类型声明变得更简单、更易读。
    2.2 语法：typedef existing_type new_alias;
    2.3 实例：typedef unsigned long ulong;
3、std::unique_ptr
    3.1 含义： std::unique_ptr是C++11引入的一种智能指针，用于独占式地管理动态分配的对象。它确保在智能指针超出作用域时自动释放所管理的资源，防止内存泄漏。
    3.2 特点：
        3.2.1 独占所有权： 一个std::unique_ptr只能拥有一个指针。不能复制，只能移动。
        3.2.2 自动资源管理： 当std::unique_ptr被销毁时，会自动调用其删除器来释放资源。
    3.3 std::unique_ptr<T, Deleter>
4、自定义删除器（deleter）
    4.1 含义： 默认情况下，std::unique_ptr使用delete操作符来释放所管理的对象。但在某些情况下，您可能需要使用不同的方式来释放资源，例如使用自定义的释放函数。在这种情况下，可以为std::unique_ptr指定一个自定义删除器。
    4.2 实现方式： 自定义删除器通常是一个结构体或类，重载了函数调用运算符operator()，以执行特定的资源释放逻辑。
*/