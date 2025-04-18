### 先讲结论
- **`.github/labeler.yml` 只是 *配置文件 (configuration file)*，不是 *工作流 (workflow)*。**
  - .github.labeler.yml是.github/workflows/labeler.yml文件的配置文件，因为actions/labeler这个官方action需要这样一个配置文件，并且将会在.github中查找labeler.yml配置文件。
- 真正会在 **Actions** 页出现的工作流文件叫 **`.github/workflows/labeler.yml`**（名字也叫 *Pull Request Labeler*）。
- 之所以把配置文件放在 `.github` 根目录，而不是放进 `workflows/` 子目录，是因为 `actions/labeler` 这个官方 Action 默认就在这里找配置。
- 这个文件不是 GitHub 自动生成的「内置文件」，而是项目维护者手动写的，用来告诉 Labeler Action：**“当哪些路径被改动时，自动给 PR 打哪些标签”。**

---

## 目录结构分别负责什么？

| 位置 | 角色 | GitHub 如何处理 |
|------|------|----------------|
| `.github/workflows/*.yml` | **Workflow definition**：定义事件触发、job、step、用到的 Action。 | 只要 push 到默认分支，GitHub Actions **立即注册为工作流**，在 **Actions** 选项卡能看到。 |
| `.github/labeler.yml` | **Labeler Action 的 config**：告诉 `actions/labeler` 各种 path‑to‑label 规则。 | **不会** 被当成工作流执行，而是在 Labeler Action 运行时被 **读取**。 |

---

## 具体到 llama.cpp 的实现

1. **工作流文件**（`.github/workflows/labeler.yml`）大致如下：
   ```yaml
   name: "Pull Request Labeler"
   on: [pull_request_target]

   jobs:
     labeler:
       runs-on: ubuntu-latest
       steps:
         - uses: actions/checkout@v4
         - uses: actions/labeler@v5
           with:
             configuration-path: '.github/labeler.yml'
   ```
   这里的 `configuration-path` 就指向了根目录下的 **`.github/labeler.yml`**。（可以知道对于GitHub actions来说也是会用到对应的配置文件）

2. **配置文件**（你贴出的内容）列出了各种 globs → label 规则，例如：
   ```yaml
   Kompute:
     - changed-files:
         - any-glob-to-any-file:
             - ggml/include/ggml-kompute.h
             - ggml/src/ggml-kompute/**
   ```
   当 PR 改动这些路径时，Labeler Action 就会自动给 PR 加上 `Kompute` 这个标签。
   官方文档也说明：**“Create a `.github/labeler.yml` file …”**。

---

## 为什么你在 Actions 页面没看到 “labeler.yml”？

- **Actions 页列的是工作流名字**（如 *Pull Request Labeler*），不是文件名。
- 如果你只搜索 “labeler.yml”，看不到是正常的；要找 **Workflow name** 或者直接翻到 “All workflows” 里找 *Pull Request Labeler*。
- 也可能只是被折叠在 “Show more workflows…” 里，需要展开才能看到。

---

## 小结（TL;DR）

1. **配置** 放 `.github/labeler.yml` —— 这是 `actions/labeler` 的默认查找路径。
2. **工作流** 放 `.github/workflows/labeler.yml` —— GitHub 才能识别并运行它。
3. 两者分工明确：
   - 工作流文件 = 触发规则 + 运行 steps
   - 配置文件 = Labeler 规则
4. `.github/labeler.yml` 并不是 GitHub 系统自动带的，完全是仓库维护者自己写的。

这样就能解释为什么你在本地看到两个同名但不同层级的文件，以及为什么只有 `workflows/` 里的那个会在 GitHub Actions UI 里出现。
