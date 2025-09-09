#!/usr/bin/env bash
set -euo pipefail

# Cross‑platform local build + test helper for QCopilot builtin tools
# Usage:
#   ./run_local.sh [-b build_dir] [-t] [-c Debug|Release]
# Options:
#   -b  Build directory (default: build-qcopilot-local)
#   -t  Emit JUnit report to build_dir/test-results/qcopilot-tests.xml
#   -c  CMAKE_BUILD_TYPE (single-config generators). On MSVC multi-config this is passed to ctest via -C.

BUILD_DIR="build-qcopilot-local"
EMIT_JUNIT=false
BUILD_TYPE="Release"

while getopts ":b:tc:" opt; do
  case $opt in
    b) BUILD_DIR="$OPTARG" ;;
    t) EMIT_JUNIT=true ;;
    c) BUILD_TYPE="$OPTARG" ;;
    *) echo "Unknown option: -$OPTARG" >&2; exit 2 ;;
  esac
done

mkdir -p "$BUILD_DIR"

# Configure
cmake -S . -B "$BUILD_DIR" \
  -DLLAMA_BUILD_TESTS=ON \
  -DLLAMA_BUILD_SERVER=ON \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

# Build
cmake --build "$BUILD_DIR" -j

# Test
pushd "$BUILD_DIR" >/dev/null
mkdir -p test-results

if $EMIT_JUNIT; then
  ctest --output-on-failure -R test_ --output-junit test-results/qcopilot-tests.xml -C "$BUILD_TYPE"
else
  ctest --output-on-failure -R test_ -C "$BUILD_TYPE"
fi

popd >/dev/null

echo "Done. Build dir: $BUILD_DIR"

