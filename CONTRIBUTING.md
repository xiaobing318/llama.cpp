# 1 Pull requests (for contributors)
```c
/*
Note:杨小兵-2024-12-30

1、Pull Request（简称 PR）是在使用版本控制系统（如 Git）进行协作开发时的一种机制，主要用于向项目的主分支（如 `main` 或 `master`）提交代码更改请求。它允许开发者在独立的分支上进行修改，然后通过 Pull Request 将这些修改合并到主项目中。
2、版本控制系统不止一个，常见的版本控制系统有下列几个
  - Git：
    - 简介：分布式版本控制系统，广泛用于开源和商业项目。
    - 特点：支持分支和合并操作，高效处理大规模项目，拥有丰富的生态系统（如 GitHub、GitLab、Bitbucket）。

  - Subversion（SVN）：
    - 简介：集中式版本控制系统。
    - 特点：简单易用，适合需要集中管理的项目，但在分支和合并操作上不如 Git 灵活。

  - Mercurial：
    - 简介：分布式版本控制系统，与 Git 类似。
    - 特点：用户界面友好，性能优越，适合大规模项目，但生态系统不如 Git 丰富。

  - Bazaar：
    - 简介：分布式和集中式版本控制系统兼容。
    - 特点：灵活性高，支持多种工作流，但使用人数较少，社区支持有限。

  - Perforce（Helix Core）：
    - 简介：集中式和分布式版本控制系统。
    - 特点：高性能，适合大型企业和需要处理大量二进制文件的项目。

*/
```
- Test your changes:
  - Execute [the full CI locally on your machine](ci/README.md) before publishing
  - Verify that the perplexity and the performance are not affected negatively by your changes (use `llama-perplexity` and `llama-bench`)
  - If you modified the `ggml` source, run the `test-backend-ops` tool to check whether different backend implementations of the `ggml` operators produce consistent results (this requires access to at least two different `ggml` backends)
  - If you modified a `ggml` operator or added a new one, add the corresponding test cases to `test-backend-ops`
- Consider allowing write access to your branch for faster reviews, as reviewers can push commits directly
- If your PR becomes stale, don't hesitate to ping the maintainers in the comments
```c
/*
Note:杨小兵-2024-12-30

1、测试你所做出的改变
  1.1 在publishing之前执行[the full CI locally on your machine]
    1.1.1 我对CI还不是很了解，需要深入了解对应的内容
  1.2 验证perplexity和performance没有被你的代码更改产生负向影响（使用llama-perplexity、llama-bench）
  1.3 如果你修改了ggml的源代码，运行test-backend-ops工具检查不同的ggml operators后端实现是否产生一致的结果（这需要至少访问两个不同的ggml backends
  1.4 如果你修改了一个ggml operator或者添加了一个ggml operator，需要在test-backend-ops中添加对应的测试用例
2、为了更快的代码审查考虑允许你的分支有写入访问权限，如此一来审查者便可以将commits直接进行推送
3、如果你的pull request变得稳定，在comments中不要犹豫ping项目维护者
4、这里对想要PR的贡献者提了一些建议和要求
*/
```

# 2 Pull requests (for collaborators)

- Squash-merge PRs
- Use the following format for the squashed commit title: `<module> : <commit title> (#<issue_number>)`. For example: `utils : fix typo in utils.py (#1234)`
- Optionally pick a `<module>` from here: https://github.com/ggerganov/llama.cpp/wiki/Modules
- Consider adding yourself to [CODEOWNERS](CODEOWNERS)
```c
/*
Note:杨小兵-2024-12-30

1、压缩合并PRs
2、针对压缩的commit title使用下列的格式：<module> : <commit title> (#<issue_number>)
3、可以从https://github.com/ggerganov/llama.cpp/wiki/Modules有选择的选取一个module
4、可以将自己添加到[CODEOWNERS]中
*/
```

# 3 Coding guidelines

- Avoid adding third-party dependencies, extra files, extra headers, etc.
- Always consider cross-compatibility with other operating systems and architectures
- Avoid fancy-looking modern STL constructs, use basic `for` loops, avoid templates, keep it simple
- There are no strict rules for the code style, but try to follow the patterns in the code (indentation, spaces, etc.). Vertical alignment makes things more readable and easier to batch edit
- Clean-up any trailing whitespaces, use 4 spaces for indentation, brackets on the same line, `void * ptr`, `int & a`
- Naming usually optimizes for common prefix (see https://github.com/ggerganov/ggml/pull/302#discussion_r1243240963)
- Tensors store data in row-major order. We refer to dimension 0 as columns, 1 as rows, 2 as matrices
- Matrix multiplication is unconventional: [`C = ggml_mul_mat(ctx, A, B)`](https://github.com/ggerganov/llama.cpp/blob/880e352277fc017df4d5794f0c21c44e1eae2b84/ggml.h#L1058-L1064) means $C^T = A B^T \Leftrightarrow C = B A^T.$

![matmul](media/matmul.png)
```c
/*
Note:杨小兵-2024-12-30

1、这部分内容提到一些编码的指导
2、避免添加第三方库依赖、extra files、extra header等等
3、总是考虑与不同的operating systems和architectures保持跨平台兼容性
4、避免使用比较花哨的现代STL构造函数，使用基本的for循环，避免模版，保持简单
5、对于代码风格没有严格的规则，但是尝试遵循代码中的模式（对齐、空格、等等）。垂直对齐使得代码更可读和更加容易批量修改。
6、清除任何的尾部空格，使用4个空格进行对齐，在相同行添加括号，void *ptr，int & a
7、命名通常针对通用前缀进行优化（可以查看对应的网址中对这一点的讨论）
8、tensors中以row-major的次序存储数据。我们将dimension 0看成是column，将dimension 1看成是row，将dimension 2看成是matrices
9、matrix multiplication不是传统的方式：[`C = ggml_mul_mat(ctx, A, B)`]（这部分内容还需要其他的先验知识）
*/
```

# 4 Resources

The Github issues, PRs and discussions contain a lot of information that can be useful to get familiar with the codebase. For convenience, some of the more important information is referenced from Github projects:

https://github.com/ggerganov/llama.cpp/projects
```c
/*
Note:杨小兵-2024-12-30

1、如果想要熟悉codebase的话，GitHub中的issues、PRs和discussions包含很多有用的信息。为了方便起见，一些更重要的信息可以从GitHub projects中找到。
2、这里的重点就是：https://github.com/ggerganov/llama.cpp/projects
*/
```