#!/bin/bash

test_method() {
    local test_name=$1
    local block_size=$2
    local block_count=$3

    echo "=== $test_name ==="
    rm -f input.txt output.txt

    dd if=/dev/urandom of=input.txt bs="$block_size" count="$block_count" status=none
    ./build/run 2>&1

    if [ -f "output.txt" ]; then
        md5_in=$(md5sum input.txt | cut -d' ' -f1)
        md5_out=$(md5sum output.txt | cut -d' ' -f1)

        if [ "$md5_in" = "$md5_out" ]; then
            echo "OK"
        else
            echo "FAIL - MD5 mismatch"
        fi
    else
        echo "FAIL - no output"
    fi
    echo ""
}

if [ ! -f "./build/run" ]; then
    echo "Error: ./build/run not found! Please build the project first."
    exit 1
fi

echo "========== Testing Shared Memory with Signals =========="
test_method "Test 1: 8KB" 8196 1
test_method "Test 2: 4MB" 1048576 4
test_method "Test 3: 2GB" 1048576 2048
echo ""

rm -f input.txt output.txt

