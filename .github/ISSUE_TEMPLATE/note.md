下面先给出核心结论，再逐项拆解说明。

> **概要**
> 在 `llama.cpp` 仓库里，位于 `.github/ISSUE_TEMPLATE/` 目录下的 `*.yml` 文件，都是 GitHub 的 **Issue Form 模板**，它们在用户点击 “New issue” 时，会以可填表单的方式指导提报问题的流程。每个模板文件开头都会定义 **name/description/labels/assignees** 等元数据；而同目录下的 `config.yml` 则控制模板选择器的行为（比如是否允许空白 Issue、提供外部帮助链接等）。

---

## 1. `.github/ISSUE_TEMPLATE/` 下的 YML 模板文件

仓库目录中列出了若干按编号命名的模板文件：
- `010-bug-compilation.yml`
- `011-bug-results.yml`
- `019-bug-misc.yml`
- `020-enhancement.yml`
- `030-research.yml`
- `040-refactor.yml`

这些都是 **Issue Form**（议题表单）模板，每当用户新建 Issue 时，GitHub 会让用户从这些表单中选择最符合场景的一项来填写。

---

## 2. 模板文件的结构和作用

以 `010-bug-compilation.yml` 为例：
```yaml
name: Bug (compilation)
about: Something goes wrong when trying to compile llama.cpp.
labels: [bug-unconfirmed, compilation]
...
- commit: Which commit are you trying to compile?
- os:     Which operating systems do you know to be affected?
...
```
- **name/about**：在模板下拉菜单和 Issue 描述里显示的标题与简述。
- **labels/assignees**：自动为新 Issue 打上标签、指定默认负责人。
- **fields**（如 `commit`、`os`）定义了表单里的输入项，用户填写后会自动渲染到 Issue 描述中。

---

## 3. `config.yml` 的作用

在同一目录下的 `config.yml` 用来 **配置模板选择器**：
```yaml
blank_issues_enabled: true
contact_links:
  - name: Got an idea?
    url:  https://github.com/ggml-org/llama.cpp/discussions/categories/ideas
    about: Pop it there. It may then become an enhancement ticket.
  - name: Got a question?
    url:  https://github.com/ggml-org/llama.cpp/discussions/categories/q-a
    about: Ask a question there!
```
- **blank_issues_enabled**：是否允许用户跳过模板，创建空白 Issue（这里设为 `true`） 。
- **contact_links**：在选择模板界面提供跳转链接，引导用户到 Discussions、Wiki 等外部帮助渠道 。

---

## 4. 与传统 Markdown 模板的区别

- **YAML Issue Forms** 支持更丰富的字段类型（如 `dropdown`、`textarea` 等），并且可以自动校验或预填选项。
- 相较旧版的 `issue_template.md`，无需手写 HTML 注释或占位符，更适合规范化大项目的提报流程 。

---

## 5. 使用流程示例

1. 在仓库首页点击 **“New issue”** → GitHub 弹出 **模板选择列表**，按 `name` 显示各 YML 表单选项。
2. 选择 “Bug (compilation)” → 表单里自动出现 “Which commit…”、“Which operating systems…” 等输入框。
3. 填写完毕提交后，GitHub 根据表单内容生成标准化的 Issue 文本，并附上预设的标签和负责人。

---

### 相关文档

- [Configuring issue templates for your repository](https://docs.github.com/en/communities/using-templates-to-encourage-useful-issues-and-pull-requests/configuring-issue-templates-for-your-repository)
- [Syntax for issue forms](https://docs.github.com/en/communities/using-templates-to-encourage-useful-issues-and-pull-requests/syntax-for-issue-forms)

以上即是 `.github/ISSUE_TEMPLATE/*.yml` 文件在 `llama.cpp` 项目中的 **作用与用法**。
