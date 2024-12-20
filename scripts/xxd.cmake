#[[
杨小兵-2024-12-20
1、CMake equivalent of `xxd -i ${INPUT} ${OUTPUT}`
  这些注释说明了脚本的功能和使用方法。脚本的目的是将输入文件转换为包含其十六进制表示的C头文件（类似于 `xxd -i` 的功能）。
使用方法示例展示了如何通过命令行传递 `INPUT` 和 `OUTPUT` 参数并执行脚本。

2、Usage: cmake -DINPUT=examples/server/public/index.html -DOUTPUT=examples/server/index.html.hpp -P scripts/xxd.cmake
  - **`cmake`**：调用CMake命令。
  - **`-DINPUT=examples/server/public/index.html`**：通过 `-D` 选项定义一个CMake变量 `INPUT`，其值为 `examples/server/public/index.html`。这是脚本中将要读取的输入文件路径。
  - **`-DOUTPUT=examples/server/index.html.hpp`**：通过 `-D` 选项定义一个CMake变量 `OUTPUT`，其值为 `examples/server/index.html.hpp`。这是脚本将要生成的输出文件路径。
  - **`-P scripts/xxd.cmake`**：指定要运行的CMake脚本文件 `scripts/xxd.cmake`。

3、xxd.cmake脚本作用
  `xxd.cmake` 脚本的目的是将指定的输入文件转换为包含其十六进制表示的C头文件。根据注释中的用法示例，可以通过命令行调用CMake并传递必要的参数来执行该脚本。
]]
# CMake equivalent of `xxd -i ${INPUT} ${OUTPUT}`
# Usage: cmake -DINPUT=examples/server/public/index.html -DOUTPUT=examples/server/index.html.hpp -P scripts/xxd.cmake

# 这一行在CMake的缓存中设置一个名为 `INPUT` 的变量，初始值为空字符串。`CACHE` 关键字表示该变量可以从命令行或CMake GUI中设置。`STRING` 指定变量类型为字符串，`"Input File"` 是变量的描述。
SET(INPUT "" CACHE STRING "Input File")
# 类似地，这一行设置一个名为 `OUTPUT` 的缓存变量，用于指定输出文件路径。
SET(OUTPUT "" CACHE STRING "Output File")
# 这一命令从 `INPUT` 路径中提取文件名（不包括路径），并将结果存储在变量 `filename` 中。例如，如果 `INPUT` 是 `examples/server/public/index.html`，则 `filename` 将是 `index.html`。
get_filename_component(filename "${INPUT}" NAME)
# 这一行使用正则表达式将 `filename` 中的点 (`.`) 和破折号 (`-`) 替换为下划线 (`_`)，并将结果存储在变量 `name` 中。例如，`index.html` 将被转换为 `index_html`。
string(REGEX REPLACE "\\.|-" "_" name "${filename}")
# 这一命令以十六进制格式读取 `INPUT` 文件的内容，并将结果存储在变量 `hex_data` 中。`HEX` 选项指定以十六进制格式读取数据。
file(READ "${INPUT}" hex_data HEX)
# 这一行使用正则表达式将 `hex_data` 中的每两个十六进制字符（即一个字节）替换为 `0x` 前缀的格式，并在后面加上逗号。例如，`4f` 将被转换为 `0x4f,`。结果存储在变量 `hex_sequence` 中。
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," hex_sequence "${hex_data}")
# 这一命令计算 `hex_data` 字符串的长度（即字符数），并将结果存储在变量 `hex_len` 中。由于每个字节由两个十六进制字符表示，后续需要将其除以2以得到实际的字节长度。
string(LENGTH ${hex_data} hex_len)
# 这一行使用 `math` 命令执行数学表达式，将 `hex_len` 除以2以计算实际的字节长度，并将结果存储在变量 `len` 中。
math(EXPR len "${hex_len} / 2")
# 这一命令将生成的C代码写入 `OUTPUT` 文件。具体来说，它写入一个无符号字符数组和一个表示数组长度的无符号整数。
file(WRITE "${OUTPUT}" "unsigned char ${name}[] = {${hex_sequence}};\nunsigned int ${name}_len = ${len};\n")
