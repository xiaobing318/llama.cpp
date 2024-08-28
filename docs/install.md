# Install pre-built version of llama.cpp（安装预构建的llama.cpp版本）

## Homebrew（一种包管理器）

On Mac and Linux, the homebrew package manager can be used via（在Mac和linux操作系统平台上，homebrew包管理器可以通过下列的命令使用）

```sh
# brew是包管理器的名称（程序名称）
# 这行命令的作用就是使用brew来下载安装llama.cpp
brew install llama.cpp
```
The formula is automatically updated with new `llama.cpp` releases. More info: https://github.com/ggerganov/llama.cpp/discussions/7668（该公式会随着新发布的 `llama.cpp` 版本自动更新。更多信息：https://github.com/ggerganov/llama.cpp/discussions/7668）

## Nix（一种包管理器）

On Mac and Linux, the Nix package manager can be used via（在Mac和linux中，Nix包管理器可以通过使用下列的命令安装llama.cpp项目）

```sh
nix profile install nixpkgs#llama-cpp
```
For flake enabled installs.

Or

```sh
nix-env --file '<nixpkgs>' --install --attr llama-cpp
```

For non-flake enabled installs.

This expression is automatically updated within the [nixpkgs repo](https://github.com/NixOS/nixpkgs/blob/nixos-24.05/pkgs/by-name/ll/llama-cpp/package.nix#L164).

## Flox

On Mac and Linux, Flox can be used to install llama.cpp within a Flox environment via

```sh
flox install llama-cpp
```

Flox follows the nixpkgs build of llama.cpp.
