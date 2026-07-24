#!/bin/bash
# Run all extropian-render tests
set -euo pipefail
cd "$(dirname "$0")"

BUILD_DIR="${1:-build}"

echo "=== Unit tests ==="
"$BUILD_DIR/tests/test-render-unit" "$@" -s

echo ""
echo "=== GL rendering tests ==="
"$BUILD_DIR/tests/test-render-gl" "$@" -s
