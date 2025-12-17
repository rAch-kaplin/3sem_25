#!/usr/bin/env bash
set -euo pipefail

LOG_FILE="test.log"
exec >"$LOG_FILE" 2>&1

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$ROOT_DIR/build"
TEST_DIR="$ROOT_DIR/test"

cd "$ROOT_DIR"

if [ ! -x "$BUILD_DIR/server" ] || [ ! -x "$BUILD_DIR/client" ]; then
  cmake -S . -B "$BUILD_DIR"
  cmake --build "$BUILD_DIR" -j
fi

mkdir -p "$TEST_DIR"

head -c 100K  </dev/zero >"$TEST_DIR/small.txt"  || true
head -c 500K  </dev/zero >"$TEST_DIR/medium.txt" || true
head -c 1000K </dev/zero >"$TEST_DIR/large.txt"  || true

BIG_FILE="$TEST_DIR/huge_1g.txt"
if [ ! -f "$BIG_FILE" ]; then
  echo "Creating $BIG_FILE (1 GiB)"
  dd if=/dev/zero of="$BIG_FILE" bs=1M count=1024 status=none
fi

"$BUILD_DIR/server" &
SERVER_PID=$!
echo "Server started, PID=$SERVER_PID"

sleep 1

NUM_CLIENTS=5
FILES=(
  "$TEST_DIR/small.txt"
  "$TEST_DIR/medium.txt"
  "$TEST_DIR/large.txt"
  "$BIG_FILE"
)

echo "Starting clients..."
for i in $(seq 1 "$NUM_CLIENTS"); do
  file="${FILES[$(( (i-1) % 4 ))]}"
  echo "Client $i requests $file"
  "$BUILD_DIR/client" "$file" &
done

wait

kill "$SERVER_PID" 2>/dev/null || true
wait "$SERVER_PID" 2>/dev/null || true

echo "Done"
