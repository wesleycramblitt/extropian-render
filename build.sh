#!/bin/bash
# Build extropian-render (library, tests, and demo)
set -euo pipefail
cd "$(dirname "$0")"

BUILD_DIR="${1:-build}"
CONFIG="${2:-Release}"

echo "=== Configuring ($CONFIG) ==="
cmake -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE="$CONFIG"

echo "=== Building ==="
cmake --build "$BUILD_DIR" -j "$(nproc)"

echo "=== Done ==="
echo "Library:  $BUILD_DIR/libexd-render.a"
echo "Tests:    $BUILD_DIR/tests/test-render-unit"
echo "          $BUILD_DIR/tests/test-render-gl"
echo "Demo:     $BUILD_DIR/demo/extropian-render-demo"
