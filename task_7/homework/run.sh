#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
TEST_DIR="$SCRIPT_DIR/test_files"

cleanup() {
    kill $SERVER_PID 2>/dev/null || true
    for pid in "${CLIENT_PIDS[@]}"; do
        kill $pid 2>/dev/null || true
    done
    wait 2>/dev/null || true
    rm -rf "$TEST_DIR" "$SCRIPT_DIR/tmp" 2>/dev/null || true
}

trap cleanup EXIT INT TERM

if [ ! -f "$BUILD_DIR/server" ] || [ ! -f "$BUILD_DIR/client" ]; then
    echo "Error: build project please"
    exit 1
fi

mkdir -p "$TEST_DIR"
echo "Файл 1" > "$TEST_DIR/file1.txt"
echo "Файл 2" > "$TEST_DIR/file2.txt"
dd if=/dev/urandom of="$TEST_DIR/file3.bin" bs=1024 count=5 2>/dev/null

rm -rf "$SCRIPT_DIR/tmp" 2>/dev/null || true

cd "$BUILD_DIR"
./server & SERVER_PID=$!
sleep 2

./client "$TEST_DIR/file1.txt" > client1.log 2>&1 &
CLIENT_PIDS+=($!)

sleep 1

./client "$TEST_DIR/file2.txt" "$TEST_DIR/file3.bin" > client2.log 2>&1 &
CLIENT_PIDS+=($!)
    
for pid in "${CLIENT_PIDS[@]}"; do
    wait $pid 2>/dev/null || true
done
