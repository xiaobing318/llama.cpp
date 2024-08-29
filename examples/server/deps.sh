#!/bin/bash
# 这个bash脚本用于下载依赖项并将其保存到一个特定的目录
# Download and update deps for binary（说明这个脚本的目的是下载和更新二进制文件的依赖项）

# get the directory of this script file（解释了接下来的代码行将获取当前脚本文件所在的目录）
# 这行代码用于获取脚本文件的绝对路径。`BASH_SOURCE[0]` 表示当前执行的脚本文件路径。`dirname` 命令获取该路径的目录部分，
# 然后 `cd` 命令切换到该目录。`pwd` 命令输出当前目录的完整路径，这个值被赋给变量 `DIR`。`>/dev/null 2>&1` 是将标准输出
# 和标准错误都重定向到 `/dev/null`（即丢弃所有输出）
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
PUBLIC=$DIR/public

# 输出一行文本到标准输出，提示正在下载 JS 组件包
echo "download js bundle files"
# 这行使用 `curl` 命令从给定的 URL 下载数据。这个 URL 指向一个 JavaScript 库的集合。下载的内容被重定向（`>`）到 `$PUBLIC/index.js`，
# 即之前定义的公共目录下的 `index.js` 文件
curl https://npm.reversehttp.com/@preact/signals-core,@preact/signals,htm/preact,preact,preact/hooks > $PUBLIC/index.js
# 这行代码向 `index.js` 文件末尾添加一个新行。`echo >> $PUBLIC/index.js` 使用重定向 `>>`（追加模式），不带任何参数的 `echo` 默认输出一个新行
echo >> $PUBLIC/index.js # add newline
