# QCopilot 内置工具单元测试说明

本目录提供对 `tools/QCopilot/builtin_tools` 与 `common` 模块的全面单元测试，目标：

- 解耦 WebUI 行为与工具功能验证（缩短定位时间，降低耦合）。
- 跨平台（Windows 10/11、Ubuntu 22.04/24.04、macOS 15.6）一致性验证，重点覆盖：路径（含中文）、编码、权限、遍历等差异点。
- 可扩展的测试组织：每个工具、每个公共接口均有独立测试用例，便于并行与失败归因。

## 组织架构

- 静态库 `qcopilot_builtins`
  - 打包 builtin_tools 与 common 的实现文件，供测试目标链接复用。
  - 位置：`tools/QCopilot/tests/CMakeLists.txt`

- 测试目标命名规范
  - 工具测试：`test_<工具分类>_<工具名称>`（例如 `test_fileTools_read_text_lines`）
  - common 公共函数测试：`test_common_<函数接口名称>`（一函数一文件，不混测）

- 目录结构（节选）
  - `test_support.h`：测试公共工具（临时目录、二进制写入、轻量断言器）
  - 工具测试：`test_fileTools_*`、`test_systemTools_*`、`test_timeTools_*`、`test_mathTools_*`
- common 测试：`test_common_*`（函数名直观对应 common 接口）
  - internal 辅助函数测试：`test_common_utils_internal_*`（仅验证内部实现细节，业务代码不应直接依赖）

## 运行方式

### 构建

```bash
cmake -S . -B build -DLLAMA_BUILD_TESTS=ON -DLLAMA_BUILD_SERVER=ON
cmake --build build -j
```

Windows（PowerShell）：

```powershell
cmake -S . -B build -DLLAMA_BUILD_TESTS=ON -DLLAMA_BUILD_SERVER=ON
cmake --build build --config Release -m
```

### 执行

运行全部 QCopilot 测试：

```bash
ctest --test-dir build --output-on-failure -R test_
```

按域筛选：

```bash
ctest --test-dir build --output-on-failure -R test_common_
ctest --test-dir build --output-on-failure -R test_fileTools_
ctest --test-dir build --output-on-failure -R test_systemTools_
ctest --test-dir build --output-on-failure -R test_timeTools_
ctest --test-dir build --output-on-failure -R test_mathTools_
```

查看单个测试输出：

```bash
ctest --test-dir build -R test_common_validate_existing_path -V

或使用提供的跨平台脚本快速构建与测试：

- Linux/macOS: `tools/QCopilot/tests/run_local.sh -c Release -t`
- Windows (PowerShell): `tools/QCopilot/tests/run_local.ps1 -Config Release -JUnit`
```

## 新增测试用例指南

1. 新建源文件，命名为：
   - 工具：`test_<工具分类>_<工具名称>.cpp`
   - common：`test_common_<函数接口名称>.cpp`

2. 在 `tools/QCopilot/tests/CMakeLists.txt` 追加一行：

```cmake
qcopilot_add_test(test_<name> test_support.h <source>.cpp)
```

3. 在源文件中：
   - 引入对应头文件（例如 `../builtin_tools/systemTools/glob.h` 或 `../builtin_tools/common/common_utils.h`）。
   - 构造 JSON 参数，调用 `run_<tool>(args)`（工具）或直接调用函数（common）。
   - 使用 `qctest::Test` 的 `check` 断言，返回 `T.finish()` 作为进程退出码。
   - 涉及文件系统时，使用 `std::filesystem::u8path` 和中文路径进行覆盖验证。

## 开放接口建议（common 模块）

经对 builtin_tools 全量工具与 common 依赖关系梳理，建议对外开放以下接口（稳定、跨平台、通用性强）：

- 路径与编码
  - `utf8_to_path`、`path_to_utf8_string`：统一跨平台 UTF-8 ↔ path 转换（Windows 宽字符，Linux/macOS UTF-8）。
  - `open_ifstream_unicode`：以 UTF-8 友好方式打开文件流。
  - `sanitize_string_for_json`、`is_valid_utf8_string`、`is_valid_utf8_file`、`is_likely_binary`、`is_likely_binary_string`。
  - `validate_existing_path`（读）与 `prepare_writable_path`（写）：路径安全校验、非法字符检测与父目录验证。

- 字符串与 JSON
  - `split_string`、`trim_string`、`join_strings`。
  - `make_error`、`make_success`、`safe_parse_json`、`format_json`（统一返回格式）。

- 时间
  - `get_current_timestamp`、`get_current_time_ms`、`format_timestamp`。

- 文件系统与搜索
  - `file_exists`、`is_regular_readable_file`、`read_file_content`、`write_file_content`、`append_file_content`、`list_directory`、`read_text_with_range`。
  - `glob_paths`、`search_in_file_regex`。

建议保留为内部使用或谨慎开放（特定语义、易被误用或更适合由上层工具封装）：

- `read_text_with_range`：虽通用，但上层工具（如 `read_text_lines`）已做更完整的 BOM/CRLF/UTF-8/二进制片段处理与参数校验，直接开放容易跳过这些安全约束。建议提供“高阶工具”优先，`read_text_with_range` 面向内部复用。
- `search_in_file_regex`：返回结构稳定，但其正则风格、大小写行为、匹配上限等在工具层有更明确协议（如 `grep`）。若开放，建议明确行为契约与限制。

> 说明：以上分类并非强制，仅为 API 稳定性与可维护性建议，可结合业务场景选择性开放。

内部接口说明：

- `read_text_with_range` 已迁移至 `builtin_tools/common/common_utils_internal.h`，标记为内部接口，仅供内部复用与单测验证。
  业务逻辑中请优先使用 `FileTools::run_read_text_lines` 工具，以获得更完整的 BOM/CRLF/UTF‑8/二进制片段校验与错误处理。

## 设计准则与覆盖要点

- 一函数一测试文件：降低认知负担，便于快速定位失败点。
- 覆盖常见场景与关键边界：空/非法输入、中文路径、BOM/CRLF、NUL 字节、权限拒绝、递归遍历、大小写敏感等。
- 保持 JSON 输出稳定：使用 `make_success`/`make_error` 与 `format_json`，便于自动化检查。
- 跨平台一致性：统一使用 `fs::u8path`、`path_to_utf8_string` 序列化路径字段。

## 常见问题

- Windows 上中文路径打不开？
  - 所有文件流打开统一使用 `open_ifstream_unicode`，路径构造使用 `utf8_to_path`，避免 CP_ACP 乱码问题。

- 目录遍历出现权限异常？
  - `glob_paths` 与目录枚举使用 `directory_options::skip_permission_denied`，测试中避免失败。

- 为什么禁止在 common 混测多个函数？
  - 便于新人理解“一个文件一个接口”的映射关系；失败时快速定位；便于按需挑选/重跑。

## 生成测试报告（JUnit/XML）

可用 CTest 的 JUnit 输出将结果导出为 XML，便于与外部系统集成（例如 GitHub Actions、Jenkins 等）：

```bash
ctest --test-dir build --output-on-failure -R test_ --output-junit build/test-results/qcopilot-tests.xml
```

输出文件：`build/test-results/qcopilot-tests.xml`。

如需与 CI 对接，请在 CI 中运行上述命令后上传该 XML 作为构件（artifact）。
此外，部分测试演示使用了轻量级 JSON Schema 断言辅助（见 `test_support.h` 中 `expect_json_schema`），
可用于校验工具返回 JSON 的基本字段与类型，便于更严格地把关契约稳定性。

## CI 平台矩阵与缓存并行

仓库已提供 GitHub Actions 工作流：`.github/workflows/qcopilot-tests.yml`

- 平台矩阵：`ubuntu-22.04`、`ubuntu-24.04`、`windows-2022`、`macos-14`（可根据 Runner 支持切换到 `macos-15`）
- 缓存：Linux/macOS 使用 `ccache`，加速重复构建
- 并行：`CTEST_PARALLEL_LEVEL` 控制单测并行度
- JUnit：通过 `ctest --output-junit` 生成 XML，并作为构件上传

如需本地模拟 CI 行为，可手动执行：

```bash
cmake -S . -B build -DLLAMA_BUILD_TESTS=ON -DLLAMA_BUILD_SERVER=ON
cmake --build build -j
ctest --test-dir build --output-on-failure -R test_ --output-junit build/test-results/qcopilot-tests.xml
```

### Windows 上的 sccache 与 Debug/Release 矩阵

- CI 在 Linux/macOS 使用 ccache，在 Windows 使用 sccache；分别通过编译器 launcher 启用缓存，缩短重复构建时间。
- CI 同时覆盖 Debug 与 Release 两种构建配置，保证两种配置的可用性与一致性。

## 徽章（Badge）集成

可在仓库根 README 中加入工作流状态徽章（将 OWNER/REPO 替换为实际仓库标识）：

```markdown
![qcopilot-tests](https://github.com/OWNER/REPO/actions/workflows/qcopilot-tests.yml/badge.svg)
```

CI 会将 JUnit 报告作为构件（artifact）上传，可在 Actions 运行详情页下载并在外部平台解析展示。
