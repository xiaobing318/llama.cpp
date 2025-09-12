QCopilot 配置与工具定义完整指南（Linux/macOS/Windows）

适用范围：tools/QCopilot 项目。本指南面向首次接触 QCopilot 的使用者与开发者，详细解释 QCopilot 配置文件 QCopilotConfig-***.json 的结构、每个字段含义、修改方法、必填与可选项、模板语法、常见陷阱与禁止事项，并给出可直接套用的示例。

一、项目与运行流程概览（不含 webui）
- 入口与服务
  - 二进制：qcopilot（编译自 `tools/QCopilot/qcopilot.cpp`）。
  - 启动时读取 JSON 配置，必要时自动拉起 BaseServer，并对外暴露 HTTP 接口：
    - 健康检查：`GET /health`（转发到 BaseServer）
    - 工具列表：`GET /tools`（返回已注册的内置工具与外部工具定义）
    - 推理与工具调用：`POST /v1/chat/completions`（与 BaseServer 建立多轮对话，自动合并工具调用）
    - 直连工具：`POST /execute_tool`（按名称直接执行某个工具）
- 工具执行
  - 内置工具：随程序内置并自动注册（见 `tools/QCopilot/qcopilot_builtin_tools.*`）。
  - 外部工具：由配置文件 tools 数组注册到执行器（`ToolExecutor`，见 `tools/QCopilot/qcopilot_executor.*`）。
  - 执行器负责：模板展开、命令行构建、子进程启动、超时与输出采集、JSON 尝试解析等。
- 运行关键点（代码位置）
  - 配置结构体与加载：`tools/QCopilot/qcopilot.cpp`（QCopilotConfig 定义、loadConfig、validateConfig）
  - 工具注册与验证：`tools/QCopilot/qcopilot_executor.cpp`（registerExternalTools、validateToolDefinition）
  - 参数校验（JSON Schema 子集）：`tools/QCopilot/qcopilot_utils.cpp`（validate_arguments）
  - 命令模板语法与跨平台转义：`tools/QCopilot/qcopilot_executor.cpp`（build_argv_from_template 等）

二、QCopilotConfig-***.json 顶层结构
顶层字段均在启动时读取（`loadConfig`），随后进行验证（`validateConfig`）。未提供的字段使用程序内默认值。字段含义如下：
- qcopilot_host（必填/有默认）：QCopilot 监听地址，默认 "127.0.0.1"。
- qcopilot_port（必填/有默认）：QCopilot 监听端口，默认 8081；范围 1–65535。
- base_server_host（必填/有默认）：BaseServer 的地址，默认 "127.0.0.1"。
- base_server_port（必填/有默认）：BaseServer 的端口，默认 8080；范围 1–65535。
- base_server_path（条件必填）：当 `auto_start_base_server = true` 时必填，指向 BaseServer 可执行文件路径（仅路径，不要附加参数）。
- model_path（条件必填）：当 `auto_start_base_server = true` 时必填，指向 GGUF 模型文件路径。
- n_ctx（可选/有默认）：上下文长度，默认 2048；校验范围为 512–1048576。
- n_gpu_layers（可选/有默认）：GPU 层数；-1 表示自动，0 仅 CPU，正数表示 GPU 层数。不得小于 -1。
- auto_start_base_server（可选/有默认）：是否由 QCopilot 自动启动 BaseServer，默认 true。为 false 时，需自行确保 BaseServer 已按 `base_server_host/port` 运行。
- log_level（可选/有默认）：日志级别，允许值：DEBUG、INFO、WARN、ERROR、NONE；默认 INFO。
- tools（可选/有默认）：外部工具定义数组；若数组内任一工具定义非法，配置加载失败（整体拒绝）。

提示：当自动启动 BaseServer 时，QCopilot 会附加 `--no-webui` 与 `--jinja`，并设置 `--chat-template-kwargs`（推理配置），不需要在 `base_server_path` 中自行添加这些参数。

三、tools 数组中单个工具定义的规范
每个元素是一个 JSON 对象，代表一个“可被 LLM 调用的函数式工具”。执行器会把这些定义注入到 /v1/chat/completions 的 tools 列表中，同时支持直接 `POST /execute_tool` 调用。

3.1 顶层字段
- type（必填）：固定为 "function"。否则注册失败。
- function（必填，对外展示定义）：包含 name、description、parameters（见 3.2）。
- executable_windows / executable_linux / executable_macos（至少其一或 executable_generic 必填）：平台专属可执行文件路径（绝对/相对均可）。
  - 注意：仅支持 `executable_generic` 作为通用字段；`executable` 已不再支持。
  - 解析优先级：平台专属 > `executable_generic`。
- executable_generic（可选，回退路径）：若当前平台未提供专属字段，则使用该通用路径。允许仅写 `executable_generic`。
- command_template（可选）：命令模板字符串，支持占位符与修饰语法（详见 3.4）。
  - 若缺省或为空：仅启动可执行文件进程，并把工具调用的 JSON 参数整体写入子进程的标准输入（stdin）。适用于“读取 stdin JSON、输出 JSON”风格的工具。
- timeout_ms（可选）：超时毫秒（整型或可解析成长整型的字符串）。默认 -1（不超时）。超时会终止子进程并在结果中标记 `timed_out: true`。

3.2 function 对象（对 LLM 的函数说明）
- 校验规则清单（与 `validateToolDefinition` 完全一致）：
  - name（必填）：存在、类型为字符串、非空；且名称格式必须合法：以英文字母开头，只能包含字母/数字/下划线。
  - description（必填）：存在、类型为字符串、非空。
  - parameters（可选）：若存在则必须是对象，且：
    - 必含 `type` 且值为 `object`（字符串）。
    - 必含 `properties` 且为对象。
    - `required` 若存在，必须是字符串数组（每个元素为字符串）。
    - `additionalProperties` 可选（布尔）。
  - 其余键（如 `default`/`enum`/`minLength`/`pattern` 等）不由该函数强制校验，但会在运行时参数校验（`validate_arguments`）中生效。

- 子 schema 常见键（供编写参考）：
  - type: `string` | `number` | `integer` | `boolean` | `object` | `array` | `null`
  - description: 参数说明字符串
  - enum: 值集合
  - 字符串约束：minLength / maxLength / pattern
  - 数值约束：minimum / maximum / exclusiveMinimum / exclusiveMaximum

3.2.1 真实示例：shapefile_converter（来自 macOS 配置）
- 位置：tools/QCopilot/QCopilotConfig-macos.json:13
- 片段（省略不相关字段）：
```
{
  "type": "function",
  "executable_generic": "shapefile_converter",
  "executable_macos": "/usr/local/bin/shapefile_converter",
  "function": {
    "name": "shapefile_converter",
    "description": "Convert ESRI Shapefile attribute text encoding ...",
    "parameters": {
      "type": "object",
      "properties": {
        "input_path": { "type": "string", "minLength": 1 },
        "s_flag": { "type": "string", "enum": ["-s"], "default": "-s" },
        "source_encoding": { "type": "string", "minLength": 2, "maxLength": 40, "pattern": "^[A-Za-z0-9.-]+$" },
        "t_flag": { "type": "string", "enum": ["-t"], "default": "-t" },
        "target_encoding": { "type": "string", "minLength": 2, "maxLength": 40, "pattern": "^[A-Za-z0-9.-]+$" }
      },
      "required": ["input_path","s_flag","t_flag"],
      "additionalProperties": false
    }
  },
  "command_template": "shapefile_converter {s_flag} {source_encoding} {t_flag} {target_encoding} \"{input_path}\""
}
```
- 对照校验要点：
  - 顶层 `type=function` 满足要求；
  - 可执行文件：提供了平台专属 `executable_macos` 与通用 `executable_generic`（满足“至少其一或 generic”）；
  - function.name/description：均存在且非空；名称符合格式；
  - parameters：存在，type=object；properties 为对象；required 为字符串数组；允许 additionalProperties=false；
  - command_template：存在且为字符串。

3.2.2 真实示例：ogr_convert_format_basic（来自 macOS 配置）
- 位置：tools/QCopilot/QCopilotConfig-macos.json:131
- 片段（省略不相关字段）：
```
{
  "type": "function",
  "executable_generic": "ogr2ogr",
  "executable_macos": "/usr/bin/ogr2ogr",
  "function": {
    "name": "ogr_convert_format_basic",
    "description": "Convert a vector dataset from one format to another ...",
    "parameters": {
      "type": "object",
      "properties": {
        "input_file": { "type": "string", "minLength": 1 },
        "output_file": { "type": "string", "minLength": 1 },
        "output_format": { "type": "string", "enum": ["GPKG","ESRI Shapefile","GeoJSON", ...], "default": "GPKG" },
        "overwrite_flag": { "type": "string", "enum": ["-overwrite"], "default": "-overwrite" }
      },
      "required": ["input_file","output_file","output_format","overwrite_flag"],
      "additionalProperties": false
    }
  },
  "command_template": "executableFilePath {overwrite_flag} -f \"{output_format}\" \"{output_file}\" \"{input_file}\""
}
```
- 对照校验要点：同上，完全满足 `validateToolDefinition` 的各项检查。

3.3 可执行文件解析与首参数替换规则
- 平台解析：在执行前，执行器按当前平台优先选择 `executable_windows/linux/macos`，若缺省则回退 `executable_generic`。
- 首 token 覆盖：构建 argv 时，会解析 `command_template` 的第一个 token：
  - 若第一个 token 不含路径分隔符（`/` 或 `\`）且已解析到可执行文件路径，则用解析出的“可执行文件路径”替换模板中的“第一个 token”。
  - 这意味着模板中常见写法如：
    - `"ogr2ogr ..."` 或 `"executableFilePath ..."` 都会在运行时被替换成对应平台的实际可执行文件路径；`executableFilePath` 只是占位写法，无需占位符花括号。
  - 若你在模板第一个 token 写了带路径的可执行文件（例如 `"/usr/bin/ogr2ogr"`），则不会被自动替换（按模板字面执行）。

3.4 command_template 模板语法（逐 token 展开）
模板以 shell 风格拆分为 token（识别 ' 与 " 引号）。每个 token 内支持占位符与修饰符：
- 基本替换：`{param}` → 用参数值替换。非字符串会序列化为 JSON 文本。
- 条件插入：`{param:?text}` → 当参数为“真”（true、非零、非空、非空数组/对象）时替换为 text；否则替换为空。
  - text 内允许再次包含同名占位：如 `{encoding:?-lco ENCODING={encoding}}`。
- 不等触发：`{param:!default?text}` → 当参数值不等于 default 时输出 text。
- 默认回退：`{param:or:default}` → 参数缺失或非真时，替换为 default。
- 数组连接：`{param:join:sep}` → 将数组用分隔符连接成一个 token。
  - 特例：若 token 未加引号且 `sep` 为单个空格，将展开成“多个 argv token”（相当于把数组元素逐个作为独立参数）。
- 旗标输出：`{param:flag:--name}` → 当参数为真时输出指定 flag（例如 `--verbose`）。
- 大小写/编码：`{param:upper}`、`{param:lower}`、`{param:json}`、`{param:url}`（或 `urlencode`）。

引号与空格规则：
- 模板解析保留引号语义。你在模板中用引号把某个 token（含占位后）包起来，最终该 token 作为整体传给子进程，不会再被拆分。
- 只有在未被引号包裹且使用 `{arr:join: }`（空格分隔）时，才会“多 token 展开”。

3.5 无模板模式（stdin JSON）
- 若省略 `command_template` 或置为空：执行器仅启动可执行文件，并把本次工具调用的参数 JSON 原样写入子进程 stdin。
- 适用：你编写的外部工具从 stdin 读取 JSON 请求，处理后把 JSON 结果写到 stdout。

3.6 进程与输出约定
- 退出码：0 视为成功；非 0 视为失败（`success: false`，并携带 `exit_code`、`stderr`）。
- 标准输出处理：优先尝试把 stdout 解析为 JSON：
  - 若能解析：将其与统一元数据合并返回（包含 `success: true`、`stdout`、`stderr`、`argv`、`duration_ms` 等）。
  - 若不能解析：视为普通文本，同样 `success: true` 返回，但 `llm_message` 会提示“非 JSON 输出已原样返回”。
- 超时：若 `timeout_ms` 有效且发生超时，子进程会被终止，并返回 `timed_out: true`。

四、必填/可选清单与最小可用示例
4.1 顶层字段
- 必填或有默认：`qcopilot_host`、`qcopilot_port`、`base_server_host`、`base_server_port`。
- 条件必填：`base_server_path`、`model_path`（当 `auto_start_base_server = true`）。
- 可选：`n_ctx`、`n_gpu_layers`、`auto_start_base_server`、`log_level`、`tools`。

4.2 单个工具定义
- 必填：`type`（固定 `function`）、`function.name`、`function.description`。
- 必填（其一）：`executable_windows` / `executable_linux` / `executable_macos` / `executable_generic`（至少一个）。
- 可选：`function.parameters`、`command_template`、`timeout_ms`。

4.3 最小可用工具（Windows 示例）
```
{
  "type": "function",
  "executable_windows": "C:/tools/my_tool.exe",
  "function": {
    "name": "my_tool",
    "description": "示例：把输入路径列表打印为一行文本",
    "parameters": {
      "type": "object",
      "properties": {
        "files": { "type": "array", "description": "要处理的文件路径数组" },
        "verbose": { "type": "boolean", "description": "是否详细输出" }
      },
      "required": ["files"],
      "additionalProperties": false
    }
  },
  "command_template": "my_tool {verbose:flag:--verbose} {files:join: }"
}
```
说明：首 token `my_tool` 将被自动替换成 `executable_windows` 实际路径；`files` 为数组，使用 `join: ` 在未加引号的 token 中展开为多个命令参数；`verbose` 为真时输出 `--verbose`。

4.4 最小可用工具（stdin JSON 模式）
```
{
  "type": "function",
  "executable_linux": "/usr/local/bin/my_json_tool",
  "function": {
    "name": "my_json_tool",
    "description": "示例：从stdin读取JSON并返回JSON"
  }
  // 无 command_template：参数 JSON 将从 stdin 传入
}
```

五、修改步骤建议（首次落地）
- 选择合适模板文件：根据平台拷贝并改写 `QCopilotConfig-windows.json` / `QCopilotConfig-macos.json` / `QCopilotConfig-linux.json`。
- 填好顶层字段：路径使用绝对路径最稳妥；Windows JSON 字符串中的反斜杠需写成 `\\`。
- 新增或裁剪 tools：
  - 每个工具满足 3.2/3.3/3.4 规范；
  - 校验点：`function.name` 合法、`description` 非空、`parameters.type=object`（如存在）、平台可执行文件至少一项存在；
  - 尽量让外部工具 stdout 输出 JSON，便于后续二次处理与 UI 展示。
- 启动与验证：
  - 启动 qcopilot 时指定配置：`./qcopilot --config-file-path /path/to/QCopilotConfig.json`
  - 查看已注册工具：`GET /tools`
  - 直连测试某工具：`POST /execute_tool`，请求体：`{"name":"<tool_name>","arguments":{...}}`

六、常见坑与禁止事项（务必遵守）
- 工具名称非法：禁止 `function.name` 以数字/下划线开头、包含空格或连字符（仅允许字母/数字/下划线）。
- 描述为空：禁止空的 `function.description`。
- 参数 schema 错误：
  - 禁止漏写 `parameters.type = "object"`（若提供 `parameters`）。
  - 当 `additionalProperties=false` 时，禁止传入未在 `properties` 声明的参数名。
  - `required` 列表中的参数，禁止在调用时缺失。
  - `type`、`enum`、`pattern`、`minimum/maximum` 等请严格遵守，否则运行前会被校验拒绝。
- 可执行文件字段：
  - 至少提供 `executable_windows/linux/macos/executable_generic` 之一；`executable` 字段不受支持（写入会被拒绝）。
  - 若希望“自动替换首 token 为实际路径”，禁止把模板第一个 token 写成带路径的可执行文件（那样不会被替换）。
- 模板使用：
  - 禁止把数组占位符放在被引号包裹的 token 中再期望拆分为多个参数；`{arr:join: }` 的多 token 展开只在未加引号时生效。
  - 禁止误用布尔位期望自动生成 `--flag`；请使用 `{param:flag:--flag}`。
  - 禁止在 Windows 路径字符串中遗漏 JSON 转义（应使用 `C:\\path\\to\\tool.exe`）。
- 超时：`timeout_ms` 不是“秒”；请用毫秒，或写可解析为整数的字符串。过短的超时会导致频繁终止子进程。
- 输出：
  - 若你的工具输出非 JSON，QCopilot 仍然会返回 success=true，但建议尽量输出 JSON 以便上游更好地消费。
  - 返回非 0 退出码会被视为失败；若工具使用非 0 退出码承载业务含义，请同时在 stdout 输出可解析 JSON 并解释原因。
- 重名注册：禁止在配置中重复注册相同 `function.name` 的外部工具（会被拒绝）。

七、实用示例片段（可直接参考工程内示例）
- Windows 示例：`tools/QCopilot/QCopilotConfig-windows.json`
- macOS 示例：`tools/QCopilot/QCopilotConfig-macos.json`
- Linux 示例：`tools/QCopilot/QCopilotConfig-linux.json`

八、附录：与运行相关的代码位置（便于交叉参考）
- 配置读取与验证：`tools/QCopilot/qcopilot.cpp`（loadConfig/validateConfig、自动启动 BaseServer、路由注册）
- 工具注册/执行：`tools/QCopilot/qcopilot_executor.*`（模板语法、子进程与超时、输出合并）
- 参数校验规则：`tools/QCopilot/qcopilot_utils.*`（支持的 JSON Schema 约束）
- 内置工具一览：`tools/QCopilot/qcopilot_builtin_tools.*`（已预置的时间/数学/文件/系统等工具）

如需进一步问题排查：
- 将 `log_level` 设为 `DEBUG`，重启 qcopilot，观察命令展开、argv 构建与子进程返回细节。
- 使用 `GET /tools` 核对注册的工具定义是否符合预期；用 `POST /execute_tool` 单独验证模板展开与可执行文件路径解析是否正确。

九、command_template 完整语法速查（与实现一致）
- 拆分规则：按空白分隔 token；'...' 和 "..." 内部不再拆分；双引号内支持 `\"` 与 `\\` 转义。
- 首 token 替换：若首 token 不含 `/` 或 `\\`，将被替换为解析到的实际可执行文件路径（平台专属 > executable_generic）。
- 基础占位：`{param}`（值为 JSON 非字符串时将序列化为文本插入）。
- 条件插入：`{param:?text}`（真值插入 text；真值=非空/非零/true/非空数组或对象）。text 可包含同名占位：`{encoding:?-lco ENCODING={encoding}}`。
- 不等触发：`{param:!default?text}`（存在且不等于 default 时插入 text）。
- 默认回退：`{param:or:DEFAULT}`（缺失或非真时插入 DEFAULT）。如需“值优先否则默认”，写 `{param}{param:or:DEFAULT}`。
- 数组连接：`{arr:join:sep}`。若该 token 未加引号且 `sep` 为单空格，则多参数展开：`arr=[a,b]` → `... a b`；否则始终为单一参数。
- 旗标输出：`{bool:flag:--name}`（为真输出 `--name`，否则不输出该 token）。
- 转换：`{param:upper}`、`{param:lower}`、`{param:json}`、`{param:url}`/`{param:urlencode}`。
- 省略模板：当 `command_template` 为空/缺省，参数 JSON 将写入子进程 stdin（工具可按 stdin→stdout 的 JSON 协议实现）。
- 常见陷阱：
  - 把 `{arr:join: }` 放在引号内不会多参数展开；需要不加引号。
  - 直接写 `{opt}` 但 opt 可能缺失时会生成空参数；请改用 `{opt:?...}` 或 `{opt:flag:...}` 或 `{opt:or:...}`。
  - 不要把首 token 写成绝对路径（否则无法自动替换成平台路径）。
