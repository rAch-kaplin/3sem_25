#!/bin/bash

check_md5() {
    local test_name="$1"
    local block_size="$2"
    local block_count="$3"

    echo "=== $test_name ==="
    rm -f input.txt output.txt

    dd if=/dev/urandom of=input.txt bs="$block_size" count="$block_count" status=none
    ./build/shm_run 2>&1

    parent_md5=$(md5sum input.txt | cut -d' ' -f1)
    child_md5=$(md5sum output.txt | cut -d' ' -f1)

    if [ "$parent_md5" = "$child_md5" ]; then
        echo "OK"
        return 0
    else
        echo "FAIL"
        echo "Parent MD5: $parent_md5"
        echo "Child MD5:  $child_md5"
        rm -f input.txt output.txt
        return 1
    fi
}

# Run tests
check_md5 "Test 1: 8196 bytes" 8196 1
check_md5 "Test 2: 4 MB" 1048576 4          # 1MB * 4 = 4MB
check_md5 "Test 3: 2 GB" 1048576 2048       # 1MB * 2048 = 2GB

rm -f input.txt output.txt
