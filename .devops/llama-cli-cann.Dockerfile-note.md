## 🧱 第一阶段：构建阶段（build stage）

```dockerfile
ARG ASCEND_VERSION=8.0.rc2.alpha003-910b-openeuler22.03-py3.8
```
- **类型**：构建参数
- **含义**：定义 Ascend 容器的版本，类似于 C 的 `#define` 常量

```dockerfile
FROM ascendai/cann:$ASCEND_VERSION AS build
```
- **类型**：命令（基础镜像声明）
- **含义**：以华为提供的 Ascend 容器为基础镜像，名字叫 `build`，用于构建编译工具和环境

```dockerfile
WORKDIR /app
```
- **类型**：命令
- **含义**：设置当前的工作路径为 `/app`，后续命令都在该路径下执行

```dockerfile
COPY . .
```
- **类型**：命令
- **含义**：将宿主机当前目录下的文件复制到容器内的当前工作路径（/app）

```dockerfile
RUN yum install -y gcc g++ cmake make
```
- **类型**：命令
- **含义**：使用 `yum` 包管理器安装基础构建工具，包括 C/C++ 编译器和 `cmake`

---

## 🌐 Ascend Toolkit 相关环境变量配置

以下一系列命令都是：

```dockerfile
ENV 变量名=值
```
- **类型**：设置环境变量
- **作用**：配置编译器、库、Python 包、工具链路径，方便编译器找到头文件、动态库、脚本等

主要变量说明：

| 环境变量名 | 含义 |
|------------|------|
| `ASCEND_TOOLKIT_HOME` | Ascend 工具链根目录 |
| `LIBRARY_PATH` | 供编译器查找 `.a` `.so` 静/动态库的路径 |
| `LD_LIBRARY_PATH` | 供运行时动态链接器查找库 |
| `PYTHONPATH` | 供 Python 查找模块 |
| `PATH` | 加入 Ascend 编译器等工具到可执行路径 |
| `ASCEND_AICPU_PATH`, `ASCEND_OPP_PATH`, `TOOLCHAIN_HOME`, `ASCEND_HOME_PATH` | 各类硬件/算子/编译工具路径 |

```dockerfile
ENV LD_LIBRARY_PATH=${ASCEND_TOOLKIT_HOME}/runtime/lib64/stub:$LD_LIBRARY_PATH
```
- 增加一个“虚拟库路径”，用于容器内没有 Ascend 驱动的情况下编译链接（Stub 库）

---

## 🛠️ 构建命令

```dockerfile
RUN echo "Building with static libs" && \
    source /usr/local/Ascend/ascend-toolkit/set_env.sh --force && \
    cmake -B build -DGGML_NATIVE=OFF -DGGML_CANN=ON -DBUILD_SHARED_LIBS=OFF  && \
    cmake --build build --config Release --target llama-cli
```

每个小命令说明：

1. `source set_env.sh`：载入 Ascend 工具链的完整环境变量
2. `cmake -B build ...`：配置构建系统
   - `GGML_CANN=ON` 启用 Ascend CANN 支持
   - `GGML_NATIVE=OFF` 禁用本地 CPU 路径
   - `BUILD_SHARED_LIBS=OFF` 构建静态库
3. `cmake --build ... --target llama-cli`：只构建 `llama-cli` 这个目标

---

## 📦 第二阶段：运行镜像（runtime）

```dockerfile
FROM ascendai/cann:$ASCEND_VERSION AS runtime
```
- 使用相同版本的 Ascend 容器作为运行镜像（轻量，通常不含构建工具）

```dockerfile
COPY --from=build /app/build/bin/llama-cli /llama-cli
```
- 从构建阶段复制 `llama-cli` 可执行文件到运行镜像根目录

```dockerfile
ENV LC_ALL=C.utf8
```
- 设置系统语言编码，确保 Python/C 程序中的字符显示正常

以下 `ENV` 变量与构建阶段相同，目的是让运行镜像也能找到 Ascend 的库和工具。

```dockerfile
ENTRYPOINT ["/llama-cli" ]
```
- **类型**：容器启动命令
- **含义**：启动容器时，自动运行 `/llama-cli` 程序（即你刚刚构建的主程序）

---

## ✅ 总结结构：

| 类型 | 指令 | 示例 | 含义 |
|------|------|------|------|
| 构建参数 | `ARG` | `ARG ASCEND_VERSION=...` | 定义变量，供 `FROM` 和 `RUN` 使用 |
| 设置镜像 | `FROM` | `FROM ascendai/cann` | 指定构建或运行的基础环境 |
| 复制文件 | `COPY` | `COPY . .` | 将源文件复制到容器内 |
| 安装依赖 | `RUN` | `yum install` | 在容器中运行 shell 命令 |
| 设置变量 | `ENV` | `ENV PATH=...` | 配置环境路径变量 |
| 构建项目 | `cmake` | `cmake --build` | 构建 llama.cpp 目标 |
| 启动入口 | `ENTRYPOINT` | `ENTRYPOINT [\"/llama-cli\"]` | 启动容器后默认执行的程序 |

---

如果你想要我再为 CUDA、SYCL 和 Ascend 三者做一个表格对比（支持什么加速、编译器用什么、镜像大小等），我也可以整理成一页学习笔记。

是否继续？

## 对dockerfile文件逐行解释
```dockerfile
### =====================
### Annotated Ascend-based Dockerfile for llama.cpp
### 面向仅有 C 背景用户，逐行注释
### =====================

# 定义构建参数 ASCEND_VERSION（相当于 C 中的宏定义）
ARG ASCEND_VERSION=8.0.rc2.alpha003-910b-openeuler22.03-py3.8

# 指定使用华为 Ascend AI 容器作为构建基础镜像
FROM ascendai/cann:$ASCEND_VERSION AS build

# 设置当前工作目录为 /app（相当于 cd /app）
WORKDIR /app

# 将宿主机的当前目录下所有文件复制到容器的 /app 目录
COPY . .

# 安装编译工具链（gcc, g++, cmake, make）
RUN yum install -y gcc g++ cmake make

# 以下为环境变量设置，用于配置 Ascend 编译与运行路径
# 相当于 C 中设置编译器的头文件路径、库路径等环境
ENV ASCEND_TOOLKIT_HOME=/usr/local/Ascend/ascend-toolkit/latest
ENV LIBRARY_PATH=${ASCEND_TOOLKIT_HOME}/lib64:$LIBRARY_PATH
ENV LD_LIBRARY_PATH=${ASCEND_TOOLKIT_HOME}/lib64:${ASCEND_TOOLKIT_HOME}/lib64/plugin/opskernel:${ASCEND_TOOLKIT_HOME}/lib64/plugin/nnengine:${ASCEND_TOOLKIT_HOME}/opp/built-in/op_impl/ai_core/tbe/op_tiling:${LD_LIBRARY_PATH}
ENV PYTHONPATH=${ASCEND_TOOLKIT_HOME}/python/site-packages:${ASCEND_TOOLKIT_HOME}/opp/built-in/op_impl/ai_core/tbe:${PYTHONPATH}
ENV PATH=${ASCEND_TOOLKIT_HOME}/bin:${ASCEND_TOOLKIT_HOME}/compiler/ccec_compiler/bin:${PATH}
ENV ASCEND_AICPU_PATH=${ASCEND_TOOLKIT_HOME}
ENV ASCEND_OPP_PATH=${ASCEND_TOOLKIT_HOME}/opp
ENV TOOLCHAIN_HOME=${ASCEND_TOOLKIT_HOME}/toolkit
ENV ASCEND_HOME_PATH=${ASCEND_TOOLKIT_HOME}

# 添加 stub 路径，用于构建时模拟驱动库（避免实际设备依赖）
ENV LD_LIBRARY_PATH=${ASCEND_TOOLKIT_HOME}/runtime/lib64/stub:$LD_LIBRARY_PATH

# 执行构建流程
RUN echo "Building with static libs" && \
    source /usr/local/Ascend/ascend-toolkit/set_env.sh --force && \
    cmake -B build -DGGML_NATIVE=OFF -DGGML_CANN=ON -DBUILD_SHARED_LIBS=OFF  && \
    cmake --build build --config Release --target llama-cli

# 说明：
# GGML_CANN=ON 表示启用华为 CANN 后端
# BUILD_SHARED_LIBS=OFF 表示生成静态链接文件（.a 而非 .so）

### =====================
### 运行时镜像（精简版）
### =====================

# 使用同样版本的 Ascend 运行时镜像
FROM ascendai/cann:$ASCEND_VERSION AS runtime

# 从构建阶段复制生成的 llama-cli 可执行文件
COPY --from=build /app/build/bin/llama-cli /llama-cli

# 设置 locale 编码环境，避免 Python 或 C 程序字符问题
ENV LC_ALL=C.utf8

# 再次设置 Ascend 的运行时环境变量（与构建阶段相同）
ENV ASCEND_TOOLKIT_HOME=/usr/local/Ascend/ascend-toolkit/latest
ENV LIBRARY_PATH=${ASCEND_TOOLKIT_HOME}/lib64:$LIBRARY_PATH
ENV LD_LIBRARY_PATH=${ASCEND_TOOLKIT_HOME}/lib64:${ASCEND_TOOLKIT_HOME}/lib64/plugin/opskernel:${ASCEND_TOOLKIT_HOME}/lib64/plugin/nnengine:${ASCEND_TOOLKIT_HOME}/opp/built-in/op_impl/ai_core/tbe/op_tiling:${LD_LIBRARY_PATH}
ENV PYTHONPATH=${ASCEND_TOOLKIT_HOME}/python/site-packages:${ASCEND_TOOLKIT_HOME}/opp/built-in/op_impl/ai_core/tbe:${PYTHONPATH}
ENV PATH=${ASCEND_TOOLKIT_HOME}/bin:${ASCEND_TOOLKIT_HOME}/compiler/ccec_compiler/bin:${PATH}
ENV ASCEND_AICPU_PATH=${ASCEND_TOOLKIT_HOME}
ENV ASCEND_OPP_PATH=${ASCEND_TOOLKIT_HOME}/opp
ENV TOOLCHAIN_HOME=${ASCEND_TOOLKIT_HOME}/toolkit
ENV ASCEND_HOME_PATH=${ASCEND_TOOLKIT_HOME}

# 设置容器启动后的默认程序
ENTRYPOINT ["/llama-cli" ]
```
