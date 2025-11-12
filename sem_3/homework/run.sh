#!/bin/bash

test_method() {
    local method=$1
    local test_name=$2
    local block_size=$3
    local block_count=$4

    echo "=== $test_name ($method) ==="
    rm -f input.txt output.txt

    dd if=/dev/urandom of=input.txt bs="$block_size" count="$block_count" status=none
    ./build/${method}_run 2>&1

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

for bin in shm_run fifo_run mq_run; do
    [ ! -f "./build/$bin" ] && echo "Error: ./build/$bin not found!" && exit 1
done

for method in shm fifo mq; do
    echo "========== Testing $method =========="
    test_method $method "Test 1: 8KB" 8196 1
    test_method $method "Test 2: 4MB" 1048576 4
    test_method $method "Test 3: 2GB" 1048576 2048
    echo ""
done

rm -f input.txt output.txt
