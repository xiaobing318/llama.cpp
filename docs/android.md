
# Android
```c
/*
Note:杨小兵-2025-01-11

1、这部分内容将会解释如何在Android环境中构建llama.cpp
*/
```

## 1 Build on Android using Termux
```c
/*
Note:杨小兵-2025-01-11

1、指导用户如何在 Android 设备上利用 Termux 这一终端模拟器和 Linux 环境工具来构建（编译和运行）特定的软件或项目。具体而言，这意味着用户可以在移动设备上设置必要的开发环境，安装所需的依赖项，并通过命令行工具编译代码，从而在 Android 平台上运行复杂的应用程序或开发工具。
2、Android
    2.1 Android 是一个基于 Linux 内核的开源操作系统，主要用于移动设备，如智能手机、平板电脑、智能手表和智能电视等。最初由 Android Inc. 开发，后来被谷歌收购并持续维护和更新。Android 提供了一个用户友好的界面，支持触摸操作，并拥有庞大的应用生态系统，通过 Google Play 商店和其他应用市场，用户可以下载和安装各种应用程序。作为一个操作系统，Android 负责管理设备的硬件资源（如处理器、内存、存储和网络连接），并为应用程序提供运行环境和必要的系统服务。此外，Android 还支持多任务处理、安全性管理和用户隐私保护，使其成为全球最流行的移动操作系统之一。
    2.2 Android 是一个基于 Linux 内核的开源操作系统。
3、Termux
    3.1 Termux 是一个在 Android 设备上运行的开源终端模拟器和 Linux 环境应用程序。它结合了强大的命令行界面和丰富的软件包管理系统，允许用户在 Android 上执行类似于桌面 Linux 系统的操作。
    3.1 命令行接口：提供一个功能强大的终端界面，用户可以输入和执行各种 Linux 命令。
    3.2 包管理系统：通过内置的包管理器（基于 apt），用户可以安装、更新和管理各种软件包，如编程语言（Python、Node.js）、开发工具（Git、Vim）、网络工具（SSH、curl）等。
    3.3 开发环境：允许用户在 Android 设备上编写、编译和运行代码，支持多种编程语言和开发框架，适合开发者进行移动端编程和测试。
    3.4 远程访问：支持通过 SSH 等协议远程连接到其他服务器或设备，方便进行远程管理和操作。
    3.5 文件管理：提供对设备文件系统的访问和管理功能，用户可以浏览、编辑和操作文件和目录。
    3.6 脚本和自动化：支持编写和执行 Shell 脚本，实现自动化任务和复杂操作。
    3.7 扩展性：用户可以通过安装额外的软件包来扩展 Termux 的功能，满足不同的需求，如数据库管理、版本控制、数据处理等。
4、Termux 将 Android 设备转变为一个功能全面的命令行环境，结合了移动设备的便携性和 Linux 系统的强大功能。
*/
```

[Termux](https://termux.dev/en/) is an Android terminal emulator and Linux environment app (no root required). As of writing, Termux is available experimentally in the Google Play Store; otherwise, it may be obtained directly from the project repo or on F-Droid.
```c
/*
Note:杨小兵-2025-01-11

1、Termux
    1.1 Termux 是一个为 Android 设备设计的终端模拟器和 Linux 环境应用程序。它允许用户在无需获取设备 root 权限的情况下，在 Android 手机上或平板电脑上运行类 Unix 命令行工具和应用程序。
        1.1.1 终端模拟器：提供一个命令行界面，让用户能够输入和执行各种命令，就像在桌面 Linux 或 macOS 终端中一样。
        1.1.2 Linux 环境：内置了一个轻量级的 Linux 发行版环境，支持安装和运行多种开源软件包，使得 Android 设备具备类似于桌面或服务器 Linux 系统的功能。
2、获取 Termux 的方式
    2.1 Google Play 商店：截至撰写本文时，Termux 以实验性版本的形式在 Google Play 商店上架。这意味着它可能尚未完全稳定，且可能会有一些限制或需要测试阶段的反馈。
    2.2 项目仓库（Project Repo）：用户可以直接从 Termux 的官方项目仓库获取最新版本的安装包。这通常适用于希望获得最新功能或修复的高级用户。
    2.3 开源应用商店：F-Droid 是一个专注于开源应用的 Android 应用商店，用户可以通过它下载和安装 Termux。F-Droid 提供的版本通常是开源的，且不包含 Google Play 商店版可能存在的额外限制或广告。
3、上述内容旨在介绍 Termux 这一强大的 Android 应用，强调其作为终端模拟器和 Linux 环境的双重功能，且无需 root 权限即可使用。通过提供多种获取方式（Google Play Store、项目仓库和 F-Droid），确保用户能够根据自身需求和偏好选择最适合的安装途径。Termux 为希望在移动设备上进行开发、学习或执行命令行任务的用户提供了一个灵活且功能丰富的解决方案。
*/
```

With Termux, you can install and run `llama.cpp` as if the environment were Linux. Once in the Termux shell:

```
$ apt update && apt upgrade -y
$ apt install git cmake
```
```c
/*
Note:杨小兵-2025-01-11

1、有了Termux，如果所使用的环境是linux那么就直接可以安装和运行llama.cpp。
2、在Termux shell中可以通过上述提到的两个命令首先获取对应的开发环境
    2.1 apt update && apt upgrade -y
    2.2 apt install git cmake
*/
```

Then, follow the [build instructions](https://github.com/ggerganov/llama.cpp/blob/master/docs/build.md), specifically for CMake.
```c
/*
Note:杨小兵-2025-01-11

1、然后按照[build instructions](https://github.com/ggerganov/llama.cpp/blob/master/docs/build.md)cmake的构建流程来实现构建过程。
*/
```

Once the binaries are built, download your model of choice (e.g., from Hugging Face). It's recommended to place it in the `~/` directory for best performance:

```
$ curl -L {model-url} -o ~/{model}.gguf
```

Then, if you are not already in the repo directory, `cd` into `llama.cpp` and:

```
$ ./build/bin/llama-cli -m ~/{model}.gguf -c {context-size} -p "{your-prompt}"
```

Here, we show `llama-cli`, but any of the executables under `examples` should work, in theory. Be sure to set `context-size` to a reasonable number (say, 4096) to start with; otherwise, memory could spike and kill your terminal.

To see what it might look like visually, here's an old demo of an interactive session running on a Pixel 5 phone:

https://user-images.githubusercontent.com/271616/225014776-1d567049-ad71-4ef2-b050-55b0b3b9274c.mp4
```c
/*
Note:杨小兵-2025-01-11

1、上述内容指导用户在成功构建 llama.cpp 的二进制文件后，下载所选的模型（例如从 Hugging Face），并建议将模型文件放置在用户主目录 ~/ 以获得最佳性能。用户可以使用 curl 命令下载模型并保存为 .gguf 文件。接着，若不在 llama.cpp 仓库目录中，需切换到该目录，然后通过运行 llama-cli 或 examples 文件夹下的其他可执行文件，加载模型并输入提示语进行交互。建议将上下文大小设置为合理的数值（如 4096），以避免内存过载导致终端崩溃。最后，内容还提供了在 Pixel 5 手机上运行交互式会话的演示视频链接，以便用户直观了解实际操作效果。
*/
```

## 2 Cross-compile using Android NDK
It's possible to build `llama.cpp` for Android on your host system via CMake and the Android NDK. If you are interested in this path, ensure you already have an environment prepared to cross-compile programs for Android (i.e., install the Android SDK). Note that, unlike desktop environments, the Android environment ships with a limited set of native libraries, and so only those libraries are available to CMake when building with the Android NDK (see: https://developer.android.com/ndk/guides/stable_apis.)

Once you're ready and have cloned `llama.cpp`, invoke the following in the project directory:

```
$ cmake \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-28 \
  -DCMAKE_C_FLAGS="-march=armv8.7a" \
  -DCMAKE_CXX_FLAGS="-march=armv8.7a" \
  -DGGML_OPENMP=OFF \
  -DGGML_LLAMAFILE=OFF \
  -B build-android
```

Notes:
  - While later versions of Android NDK ship with OpenMP, it must still be installed by CMake as a dependency, which is not supported at this time
  - `llamafile` does not appear to support Android devices (see: https://github.com/Mozilla-Ocho/llamafile/issues/325)

The above command should configure `llama.cpp` with the most performant options for modern devices. Even if your device is not running `armv8.7a`, `llama.cpp` includes runtime checks for available CPU features it can use.

Feel free to adjust the Android ABI for your target. Once the project is configured:

```
$ cmake --build build-android --config Release -j{n}
$ cmake --install build-android --prefix {install-dir} --config Release
```

After installing, go ahead and download the model of your choice to your host system. Then:

```
$ adb shell "mkdir /data/local/tmp/llama.cpp"
$ adb push {install-dir} /data/local/tmp/llama.cpp/
$ adb push {model}.gguf /data/local/tmp/llama.cpp/
$ adb shell
```

In the `adb shell`:

```
$ cd /data/local/tmp/llama.cpp
$ LD_LIBRARY_PATH=lib ./bin/llama-simple -m {model}.gguf -c {context-size} -p "{your-prompt}"
```

That's it!

Be aware that Android will not find the library path `lib` on its own, so we must specify `LD_LIBRARY_PATH` in order to run the installed executables. Android does support `RPATH` in later API levels, so this could change in the future. Refer to the previous section for information about `context-size` (very important!) and running other `examples`.
```c
/*
Note:杨小兵-2025-01-11

1、上述内容详细指导用户如何使用 Android NDK 在主机系统上为 Android 设备交叉编译 `llama.cpp`。首先，用户需确保已安装 Android SDK 并准备好交叉编译环境，然后克隆 `llama.cpp` 仓库并使用 CMake 配置项目，指定工具链文件、目标架构（如 `arm64-v8a`）、Android 平台版本以及编译标志，同时禁用 OpenMP 和 `llamafile`。配置完成后，通过 CMake 构建并安装项目。接着，用户需要下载所选的模型文件，并使用 `adb` 工具将编译好的可执行文件和模型推送到 Android 设备的指定目录。最后，在设备上通过设置 `LD_LIBRARY_PATH` 运行 `llama-simple` 可执行文件，加载模型并输入提示语进行交互。内容还提醒用户调整 `context-size` 以避免内存过载，并说明未来 Android 可能支持自动查找库路径，从而简化运行过程。
*/
```
