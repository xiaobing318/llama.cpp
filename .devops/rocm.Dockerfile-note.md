*2025-04-18-杨小兵*

这个 Dockerfile 是为 AMD ROCm（用于 AMD GPU 加速）平台准备的。

---

## 📦 构建参数定义（构建时可传入变量）

```dockerfile
ARG UBUNTU_VERSION=24.04
ARG ROCM_VERSION=6.3
ARG AMDGPU_VERSION=6.3
```
- **命令类型**：`ARG`
- **作用**：定义构建参数，可以通过 `docker build --build-arg` 传入实际值。
- 类似 C 中的 `#define`，是“宏参数”。

---

```dockerfile
ARG BASE_ROCM_DEV_CONTAINER=rocm/dev-ubuntu-${UBUNTU_VERSION}:${ROCM_VERSION}-complete
```
- 构造出完整的 ROCm 开发镜像名，如：
  ```
  rocm/dev-ubuntu-24.04:6.3-complete
  ```

---

## 🛠️ 构建阶段（build）

```dockerfile
FROM ${BASE_ROCM_DEV_CONTAINER} AS build
```
- **命令类型**：`FROM`
- **作用**：以构建镜像为基础开始构建流程，`AS build` 表示这个阶段命名为 `build`，用于后续引用。

---

```dockerfile
ARG ROCM_DOCKER_ARCH=gfx1100
```
- 定义要构建支持的 AMD GPU 架构，如 MI200 系列支持 `gfx1100`。

```dockerfile
ENV AMDGPU_TARGETS=${ROCM_DOCKER_ARCH}
```
- 设置环境变量，用于 cmake 检测 GPU 架构，传递给编译器。

---

```dockerfile
RUN apt-get update \
    && apt-get install -y \
    build-essential \
    cmake \
    git \
    libcurl4-openssl-dev \
    curl \
    libgomp1
```
- 安装编译工具：GCC、make、cmake、Git、curl、OpenMP 支持库。
- 类似于你本地编写 makefile 时安装的开发环境。

---

```dockerfile
WORKDIR /app
```
- 设置当前工作目录为 `/app`，相当于 `cd /app`。
- 所有后续操作都在该目录中进行。

---

```dockerfile
COPY . .
```
- 拷贝当前项目目录下的所有文件（源代码、脚本等）到 `/app`

---

### 🧱 编译构建命令

```dockerfile
RUN HIPCXX="$(hipconfig -l)/clang" HIP_PATH="$(hipconfig -R)" \
    cmake -S . -B build -DGGML_HIP=ON -DAMDGPU_TARGETS=$ROCM_DOCKER_ARCH -DCMAKE_BUILD_TYPE=Release -DLLAMA_CURL=ON \
    && cmake --build build --config Release -j$(nproc)
```

解释如下：

- `HIPCXX` 和 `HIP_PATH` 是 ROCm 的编译器配置，用于定位 AMD 的 `clang` 编译器。
- `cmake -S . -B build`：生成构建配置，类似执行 `configure` 步骤。
  - `-DGGML_HIP=ON` 启用 ROCm（HIP）后端
  - `-DAMDGPU_TARGETS=gfx1100` 设置目标架构
  - `-DLLAMA_CURL=ON` 启用联网功能（使用 curl）
- `cmake --build build -j$(nproc)`：实际编译，使用所有 CPU 核数并发执行。

---

```dockerfile
RUN mkdir -p /app/lib \
    && find build -name "*.so" -exec cp {} /app/lib \;
```
- 创建 `lib` 文件夹并复制所有编译好的 `.so` 动态库到此目录。

---

```dockerfile
RUN mkdir -p /app/full \
    && cp build/bin/* /app/full \
    && cp *.py /app/full \
    && cp -r gguf-py /app/full \
    && cp -r requirements /app/full \
    && cp requirements.txt /app/full \
    && cp .devops/tools.sh /app/full/tools.sh
```
- 整理运行环境需要的所有脚本、可执行文件和 Python 包依赖到 `/app/full`

---

## 🚀 运行时基础镜像（base）

```dockerfile
FROM ${BASE_ROCM_DEV_CONTAINER} AS base
```
- 从同一个 ROCm 镜像派生出一个“运行时”基础环境（不含编译工具）

---

```dockerfile
RUN apt-get update \
    && apt-get install -y libgomp1 curl\
    && apt autoremove -y \
    && apt clean -y \
    && rm -rf /tmp/* /var/tmp/* \
    && find /var/cache/apt/archives /var/lib/apt/lists -not -name lock -type f -delete \
    && find /var/cache -type f -delete
```
- 安装最基本运行依赖并清理缓存，减小镜像体积。

---

```dockerfile
COPY --from=build /app/lib/ /app
```
- 从 build 阶段复制 `.so` 库文件到运行镜像中。

---

## 🧩 多种运行模式

### Full 模式：包含完整依赖和 Python 脚本

```dockerfile
FROM base AS full
COPY --from=build /app/full /app
WORKDIR /app
```

```dockerfile
RUN apt-get update \
    && apt-get install -y \
    git \
    python3-pip \
    python3 \
    python3-wheel\
    && pip install --break-system-packages --upgrade setuptools \
    && pip install --break-system-packages -r requirements.txt \
    ...
```
- 安装 Python 环境并安装依赖包。
- 使用 `--break-system-packages` 表示强制安装不兼容系统版本的 pip 包。

```dockerfile
ENTRYPOINT ["/app/tools.sh"]
```
- 容器启动后默认执行 `tools.sh` 脚本。

---

### Light 模式：只包含命令行工具

```dockerfile
FROM base AS light
COPY --from=build /app/full/llama-cli /app
WORKDIR /app
ENTRYPOINT [ "/app/llama-cli" ]
```
- 轻量模式，仅支持 CLI 运行。

---

### Server 模式：仅提供 Web 接口服务

```dockerfile
FROM base AS server
ENV LLAMA_ARG_HOST=0.0.0.0
COPY --from=build /app/full/llama-server /app
WORKDIR /app
HEALTHCHECK CMD [ "curl", "-f", "http://localhost:8080/health" ]
ENTRYPOINT [ "/app/llama-server" ]
```

- 设置 Web 服务监听地址为全部接口 `0.0.0.0`
- 添加健康检查命令
- 启动 `llama-server`，提供 HTTP API

---

## ✅ 总结（关键命令汇总）

| 指令 | 含义 | 类比 |
|------|------|------|
| `ARG` | 构建参数定义 | 类似 C 宏 |
| `ENV` | 设置环境变量 | `setenv()` |
| `FROM` | 指定基础镜像 | 类似 include 环境 |
| `RUN` | 执行构建命令 | 类似 Makefile 命令 |
| `COPY` | 拷贝文件 | 文件复制 |
| `WORKDIR` | 设置当前目录 | `cd` |
| `ENTRYPOINT` | 容器启动命令 | 主程序入口 |
| `HEALTHCHECK` | 健康检测机制 | 守护进程探针 |

---


```dockerfile
### =====================
### Annotated ROCm-based Dockerfile for llama.cpp
### 面向仅有 C 背景用户，逐行注释
### =====================

# 定义构建所需基础变量
ARG UBUNTU_VERSION=24.04
ARG ROCM_VERSION=6.3
ARG AMDGPU_VERSION=6.3

# 组合成 ROCm 开发镜像名
ARG BASE_ROCM_DEV_CONTAINER=rocm/dev-ubuntu-${UBUNTU_VERSION}:${ROCM_VERSION}-complete

# 使用 ROCm 开发镜像作为构建镜像
FROM ${BASE_ROCM_DEV_CONTAINER} AS build

# 指定 GPU 架构，例如 gfx1100 是较新一代架构（MI200 系列）
ARG ROCM_DOCKER_ARCH=gfx1100

# 设置编译器所需的目标架构环境变量
ENV AMDGPU_TARGETS=${ROCM_DOCKER_ARCH}

# 安装构建依赖
RUN apt-get update \
    && apt-get install -y \
    build-essential \
    cmake \
    git \
    libcurl4-openssl-dev \
    curl \
    libgomp1

# 设置构建目录
WORKDIR /app
COPY . .

# 使用 ROCm 平台构建项目（HIP后端）
RUN HIPCXX="$(hipconfig -l)/clang" HIP_PATH="$(hipconfig -R)" \
    cmake -S . -B build \
    -DGGML_HIP=ON \
    -DAMDGPU_TARGETS=$ROCM_DOCKER_ARCH \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLAMA_CURL=ON \
    && cmake --build build --config Release -j$(nproc)

# 提取构建后的动态库文件
RUN mkdir -p /app/lib \
    && find build -name "*.so" -exec cp {} /app/lib \;

# 拷贝可执行文件与运行所需脚本、依赖文件等
RUN mkdir -p /app/full \
    && cp build/bin/* /app/full \
    && cp *.py /app/full \
    && cp -r gguf-py /app/full \
    && cp -r requirements /app/full \
    && cp requirements.txt /app/full \
    && cp .devops/tools.sh /app/full/tools.sh

### ==============
### 运行镜像层 base
### ==============
FROM ${BASE_ROCM_DEV_CONTAINER} AS base

# 安装运行所需库并清理缓存
RUN apt-get update \
    && apt-get install -y libgomp1 curl\
    && apt autoremove -y \
    && apt clean -y \
    && rm -rf /tmp/* /var/tmp/* \
    && find /var/cache/apt/archives /var/lib/apt/lists -not -name lock -type f -delete \
    && find /var/cache -type f -delete

COPY --from=build /app/lib/ /app

### ==============
### Full 镜像：含 Python 环境
### ==============
FROM base AS full
COPY --from=build /app/full /app

WORKDIR /app

RUN apt-get update \
    && apt-get install -y \
    git \
    python3-pip \
    python3 \
    python3-wheel\
    && pip install --break-system-packages --upgrade setuptools \
    && pip install --break-system-packages -r requirements.txt \
    && apt autoremove -y \
    && apt clean -y \
    && rm -rf /tmp/* /var/tmp/* \
    && find /var/cache/apt/archives /var/lib/apt/lists -not -name lock -type f -delete \
    && find /var/cache -type f -delete

ENTRYPOINT ["/app/tools.sh"]

### Light：只包含 CLI 工具
FROM base AS light
COPY --from=build /app/full/llama-cli /app
WORKDIR /app
ENTRYPOINT [ "/app/llama-cli" ]

### Server：仅 Web 服务接口
FROM base AS server
ENV LLAMA_ARG_HOST=0.0.0.0
COPY --from=build /app/full/llama-server /app
WORKDIR /app
HEALTHCHECK CMD [ "curl", "-f", "http://localhost:8080/health" ]
ENTRYPOINT [ "/app/llama-server" ]
```
