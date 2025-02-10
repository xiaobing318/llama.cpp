*杨小兵-2025-02-10*

 - 下列内容将会逐行解释 GitHub Actions 工作流配置文件中的每个命令和步骤的作用
 - 解释的文件名为server.yml

```yaml
# Server build and tests
# Notes:这是yaml配置文件中编写注释的方式同python的方式类似

name: Server
# Notes:定义了该工作流的名称是Server，这个名称将显示在 GitHub Actions 中，帮助区分不同的工作流。
on:
# Notes:定义了触发该工作流的事件。
  workflow_dispatch: # allows manual triggering
  # Notes:workflow_dispatch 事件允许手动触发工作流。用户可以在 GitHub 的 UI 中手动启动工作流，并传递输入参数（如 sha 和 slow_tests），其中sha表示要构建的提交的 SHA-1 哈希，允许指定某个提交来进行构建，slow_tests布尔值，指定是否运行慢速测试。
    inputs:
      sha:
        description: 'Commit SHA1 to build'
        required: false
        type: string
      slow_tests:
        description: 'Run slow tests'
        required: true
        type: boolean
  push:
  # Notes:触发当有推送到 master 分支且修改了指定的文件路径时。此事件监控 .github/workflows/server.yml、构建配置文件（如 CMakeLists.txt、Makefile）和代码文件（如 .cpp、.h 等）的修改。
    branches:
      - master
    paths: ['.github/workflows/server.yml', '**/CMakeLists.txt', '**/Makefile', '**/*.h', '**/*.hpp', '**/*.c', '**/*.cpp', '**/*.cu', '**/*.swift', '**/*.m', 'examples/server/**.*']
  pull_request:
  # Notes:当拉取请求创建、同步或重新打开时触发，监视的路径与 push 事件相同。
    types: [opened, synchronize, reopened]
    paths: ['.github/workflows/server.yml', '**/CMakeLists.txt', '**/Makefile', '**/*.h', '**/*.hpp', '**/*.c', '**/*.cpp', '**/*.cu', '**/*.swift', '**/*.m', 'examples/server/**.*']

env:
# Notes:定义了几个环境变量，用于日志记录和调试。
  LLAMA_LOG_COLORS: 1
  # Notes:启用日志输出的颜色。
  LLAMA_LOG_PREFIX: 1
  # Notes:在日志中添加前缀。
  LLAMA_LOG_TIMESTAMPS: 1
  # Notes:在日志中显示时间戳。
  LLAMA_LOG_VERBOSITY: 10
  # Notes:设置日志详细程度。

concurrency:
# Notes:管理工作流的并发执行。确保在同一分支上的多个工作流实例不会并行运行。并且，如果有新工作流触发，会取消正在进行的工作流实例。
  group: ${{ github.workflow }}-${{ github.ref }}-${{ github.head_ref || github.run_id }}
  cancel-in-progress: true

jobs:
# Notes:定义了工作流的具体任务，任务将并行或顺序执行。
  server:
    runs-on: ubuntu-latest
    # Notes:指定工作流在 Ubuntu 最新版本上运行。

    strategy:
      matrix:
        sanitizer: [ADDRESS, UNDEFINED] # THREAD is broken
        build_type: [RelWithDebInfo]
        include:
          - build_type: Release
            sanitizer: ""
      fail-fast: false # While -DLLAMA_SANITIZE_THREAD=ON is broken
      # Notes:即使某个任务失败，其他任务仍继续执行。

    steps:
    # Notes:列出一系列具体的步骤，这些步骤会按照顺序执行。
      - name: Dependencies
        id: depends
        run: |
          sudo apt-get update
          sudo apt-get -y install \
            build-essential \
            xxd \
            git \
            cmake \
            curl \
            wget \
            language-pack-en \
            libcurl4-openssl-dev

      - name: Clone
      # Notes:检出仓库代码，确保在工作流运行时可以访问最新的代码。ref 指定了提交的哈希。
        id: checkout
        uses: actions/checkout@v4
        with:
          fetch-depth: 0
          ref: ${{ github.event.inputs.sha || github.event.pull_request.head.sha || github.sha || github.head_ref || github.ref_name }}

      - name: Python setup
      # Notes:安装 Python 3.11 环境，确保测试能够正确运行。
        id: setup_python
        uses: actions/setup-python@v5
        with:
          python-version: '3.11'

      - name: Tests dependencies
      # Notes:安装运行测试所需的 Python 库。
        id: test_dependencies
        run: |
          pip install -r examples/server/tests/requirements.txt

      # Setup nodejs (to be used for verifying bundled index.html)
      - uses: actions/setup-node@v4
      # Notes:设置 Node.js 环境，用于构建 WebUI。
        with:
          node-version: '22.11.0'

      - name: WebUI - Install dependencies
        id: webui_lint
        run: |
          cd examples/server/webui
          npm ci

      - name: WebUI - Check code format
      # Notes:检查 WebUI 的代码格式，运行 npm run format，并确保没有不符合格式要求的文件。
        id: webui_format
        run: |
          git config --global --add safe.directory $(realpath .)
          cd examples/server/webui
          git status

          npm run format
          git status
          modified_files="$(git status -s)"
          echo "Modified files: ${modified_files}"
          if [ -n "${modified_files}" ]; then
            echo "Files do not follow coding style. To fix: npm run format"
            echo "${modified_files}"
            exit 1
          fi

      - name: Verify bundled index.html
      # Notes:构建 WebUI，确保构建结果是干净的，没有未提交的更改。
        id: verify_server_index_html
        run: |
          git config --global --add safe.directory $(realpath .)
          cd examples/server/webui
          git status

          npm run build
          git status
          modified_files="$(git status -s)"
          echo "Modified files: ${modified_files}"
          if [ -n "${modified_files}" ]; then
            echo "Repository is dirty or server/webui is not built as expected"
            echo "Hint: You may need to follow Web UI build guide in server/README.md"
            echo "${modified_files}"
            exit 1
          fi

      - name: Build (no OpenMP)
      # Notes:使用 CMake 构建服务器端代码，禁用 OpenMP
        id: cmake_build_no_openmp
        if: ${{ matrix.sanitizer == 'THREAD' }}
        run: |
          cmake -B build \
              -DGGML_NATIVE=OFF \
              -DLLAMA_BUILD_SERVER=ON \
              -DLLAMA_CURL=ON \
              -DCMAKE_BUILD_TYPE=${{ matrix.build_type }} \
              -DLLAMA_SANITIZE_${{ matrix.sanitizer }}=ON \
              -DGGML_OPENMP=OFF ;
          cmake --build build --config ${{ matrix.build_type }} -j $(nproc) --target llama-server

      - name: Build (sanitizers)
        id: cmake_build_sanitizers
        if: ${{ matrix.sanitizer != '' && matrix.sanitizer != 'THREAD' }}
        run: |
          cmake -B build \
              -DGGML_NATIVE=OFF \
              -DLLAMA_BUILD_SERVER=ON \
              -DLLAMA_CURL=ON \
              -DCMAKE_BUILD_TYPE=${{ matrix.build_type }} \
              -DLLAMA_SANITIZE_${{ matrix.sanitizer }}=ON ;
          cmake --build build --config ${{ matrix.build_type }} -j $(nproc) --target llama-server

      - name: Build (sanitizers)
        id: cmake_build
        if: ${{ matrix.sanitizer == '' }}
        run: |
          cmake -B build \
              -DGGML_NATIVE=OFF \
              -DLLAMA_BUILD_SERVER=ON \
              -DLLAMA_CURL=ON \
              -DCMAKE_BUILD_TYPE=${{ matrix.build_type }} ;
          cmake --build build --config ${{ matrix.build_type }} -j $(nproc) --target llama-server

      - name: Tests
        id: server_integration_tests
        if: ${{ matrix.sanitizer == '' }}
        run: |
          cd examples/server/tests
          ./tests.sh

      - name: Tests (sanitizers)
        id: server_integration_tests_sanitizers
        if: ${{ matrix.sanitizer != '' }}
        run: |
          cd examples/server/tests
          LLAMA_SANITIZE=1 ./tests.sh

      - name: Slow tests
        id: server_integration_tests_slow
        if: ${{ (github.event.schedule || github.event.inputs.slow_tests == 'true') && matrix.build_type == 'Release' }}
        run: |
          cd examples/server/tests
          SLOW_TESTS=1 ./tests.sh


  server-windows:
    runs-on: windows-2019

    steps:
      - name: Clone
        id: checkout
        uses: actions/checkout@v4
        with:
          fetch-depth: 0
          ref: ${{ github.event.inputs.sha || github.event.pull_request.head.sha || github.sha || github.head_ref || github.ref_name }}

      - name: libCURL
        id: get_libcurl
        env:
          CURL_VERSION: 8.6.0_6
        run: |
          curl.exe -o $env:RUNNER_TEMP/curl.zip -L "https://curl.se/windows/dl-${env:CURL_VERSION}/curl-${env:CURL_VERSION}-win64-mingw.zip"
          mkdir $env:RUNNER_TEMP/libcurl
          tar.exe -xvf $env:RUNNER_TEMP/curl.zip --strip-components=1 -C $env:RUNNER_TEMP/libcurl

      - name: Build
        id: cmake_build
        run: |
          cmake -B build -DLLAMA_CURL=ON -DCURL_LIBRARY="$env:RUNNER_TEMP/libcurl/lib/libcurl.dll.a" -DCURL_INCLUDE_DIR="$env:RUNNER_TEMP/libcurl/include"
          cmake --build build --config Release -j ${env:NUMBER_OF_PROCESSORS} --target llama-server

      - name: Python setup
        id: setup_python
        uses: actions/setup-python@v5
        with:
          python-version: '3.11'

      - name: Tests dependencies
        id: test_dependencies
        run: |
          pip install -r examples/server/tests/requirements.txt

      - name: Copy Libcurl
        id: prepare_libcurl
        run: |
          cp $env:RUNNER_TEMP/libcurl/bin/libcurl-x64.dll ./build/bin/Release/libcurl-x64.dll

      - name: Tests
        id: server_integration_tests
        if: ${{ !matrix.disabled_on_pr || !github.event.pull_request }}
        run: |
          cd examples/server/tests
          $env:PYTHONIOENCODING = ":replace"
          pytest -v -x -m "not slow"

      - name: Slow tests
        id: server_integration_tests_slow
        if: ${{ (github.event.schedule || github.event.inputs.slow_tests == 'true') && matrix.build_type == 'Release' }}
        run: |
          cd examples/server/tests
          $env:SLOW_TESTS = "1"
          pytest -v -x
```
