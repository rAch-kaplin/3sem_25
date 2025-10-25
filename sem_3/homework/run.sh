#!/bin/bash

rm -f input.txt output.txt *.txt

echo "=== Test 1: 8196 bytes ==="
dd if=/dev/urandom of=input.txt bs=8196 count=1 status=none
./build/shm_run
parent_md5=$(md5sum input.txt | cut -d' ' -f1)
child_md5=$(md5sum output.txt | cut -d' ' -f1)
if [ "$parent_md5" = "$child_md5" ]; then
    echo "OK"
else
    echo "FAIL"
    exit 1
fi

echo "=== Test 2: 4 MB ==="
dd if=/dev/urandom of=input.txt bs=1M count=4 status=none
./build/shm_run
parent_md5=$(md5sum input.txt | cut -d' ' -f1)
child_md5=$(md5sum output.txt | cut -d' ' -f1)
if [ "$parent_md5" = "$child_md5" ]; then
    echo "OK"
else
    echo "FAIL"
    exit 1
fi

echo "=== Test 3: 2 GB ==="
dd if=/dev/urandom of=input.txt bs=1M count=2048 status=none
./build/shm_run
parent_md5=$(md5sum input.txt | cut -d' ' -f1)
child_md5=$(md5sum output.txt | cut -d' ' -f1)
if [ "$parent_md5" = "$child_md5" ]; then
    echo "OK"
else
    echo "FAIL"
    exit 1
fi

echo "All tests passed!"
