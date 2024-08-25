/*  
    2024-08-25-杨小兵：这段代码是一个Swift Package Manager的配置文件，用于定义和配置名为“llama”的Swift包。
整体上，这个配置文件的作用是确保Swift包“llama”能够在支持的平台上正确构建和运行，同时处理不同平台的特定需求。

    这个文件由 Swift Package Manager 使用，它是 Swift 语言官方的包管理工具，集成在 Swift 编译器中，主要用于自动化下载、编译和链接依赖的过程。
### 具体的处理过程
1. **创建 `Package.swift` 文件**：
   - 通常在 Swift 项目的根目录下创建这个文件。
   - 文件定义了包的名字、Swift 工具的版本、目标平台、产品（库或可执行文件）、依赖和目标等。

2. **定义包配置**：
   - 包括库的名称、类型（动态或静态）、以及包含的源代码文件。
   - 还可以指定不同平台的特定设置，比如为 macOS、iOS、watchOS 或 tvOS 指定不同的编译标志或依赖。

3. **依赖管理**：
   - 在 `Package.swift` 中，可以通过指定其他包的位置和版本来添加依赖。
   - SPM 会自动处理这些依赖，包括解析版本兼容性和下载必要的代码。

4. **编译和构建**：
   - 使用终端命令 `swift build` 来编译项目，SPM 会读取 `Package.swift` 文件，并根据其中定义的设置来编译源代码和依赖。
   - 这包括下载依赖、编译源文件、链接必要的库等步骤。

5. **测试**：
   - 使用 `swift test` 命令运行单元测试。这个命令也依赖于 `Package.swift` 中定义的测试目标和设置。

6. **生成 Xcode 项目**：
   - 可以使用 `swift package generate-xcodeproj` 命令生成一个 Xcode 项目，这样就可以在 Xcode 中编辑和调试 Swift 包了。

7. **更新依赖**：
   - 通过 `swift package update` 命令更新项目的依赖到最新兼容版本。

    这个文件在 Swift 项目中起到了核心的配置作用，使得项目的构建、测试和依赖管理自动化和标准化。通过这种方式，Swift Package Manager 提供了一个强大且
灵活的工具，帮助开发者更高效地管理和维护他们的 Swift 代码库。
*/
// swift-tools-version:5.5

import PackageDescription

var sources = [
    "src/llama.cpp",
    "src/llama-vocab.cpp",
    "src/llama-grammar.cpp",
    "src/llama-sampling.cpp",
    "src/unicode.cpp",
    "src/unicode-data.cpp",
    "ggml/src/ggml.c",
    "ggml/src/ggml-alloc.c",
    "ggml/src/ggml-backend.c",
    "ggml/src/ggml-quants.c",
    "ggml/src/ggml-aarch64.c",
]

var resources: [Resource] = []
var linkerSettings: [LinkerSetting] = []
var cSettings: [CSetting] =  [
    .unsafeFlags(["-Wno-shorten-64-to-32", "-O3", "-DNDEBUG"]),
    .unsafeFlags(["-fno-objc-arc"]),
    // NOTE: NEW_LAPACK will required iOS version 16.4+
    // We should consider add this in the future when we drop support for iOS 14
    // (ref: ref: https://developer.apple.com/documentation/accelerate/1513264-cblas_sgemm?language=objc)
    // .define("ACCELERATE_NEW_LAPACK"),
    // .define("ACCELERATE_LAPACK_ILP64")
]

#if canImport(Darwin)
sources.append("ggml/src/ggml-metal.m")
resources.append(.process("ggml/src/ggml-metal.metal"))
linkerSettings.append(.linkedFramework("Accelerate"))
cSettings.append(
    contentsOf: [
        .define("GGML_USE_ACCELERATE"),
        .define("GGML_USE_METAL")
    ]
)
#endif

#if os(Linux)
    cSettings.append(.define("_GNU_SOURCE"))
#endif

let package = Package(
    name: "llama",
    platforms: [
        .macOS(.v12),
        .iOS(.v14),
        .watchOS(.v4),
        .tvOS(.v14)
    ],
    products: [
        .library(name: "llama", targets: ["llama"]),
    ],
    targets: [
        .target(
            name: "llama",
            path: ".",
            exclude: [
               "cmake",
               "examples",
               "scripts",
               "models",
               "tests",
               "CMakeLists.txt",
               "Makefile"
            ],
            sources: sources,
            resources: resources,
            publicHeadersPath: "spm-headers",
            cSettings: cSettings,
            linkerSettings: linkerSettings
        )
    ],
    cxxLanguageStandard: .cxx11
)
