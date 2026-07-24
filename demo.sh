#!/bin/bash
# Build (if needed) and run the extropian-render demo
set -euo pipefail
cd "$(dirname "$0")"

BUILD_DIR="${1:-build}"

# Ensure the demo is built
if [ ! -x "$BUILD_DIR/demo/extropian-render-demo" ]; then
    echo "=== Demo not built — building now ==="
    cmake -B "$BUILD_DIR" -G Ninja 2>/dev/null
    cmake --build "$BUILD_DIR" --target extropian-render-demo -j "$(nproc)"
fi

echo "=== Running demo ==="
cd "$BUILD_DIR"
exec ./demo/extropian-render-demo
