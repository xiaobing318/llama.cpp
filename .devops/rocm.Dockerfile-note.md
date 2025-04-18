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
