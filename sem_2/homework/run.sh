#!/bin/bash

echo "Generating test file..."
dd if=/dev/urandom of=parent.txt bs=1048576 count=4096 status=none

echo "Running duplex_pipe..."
./build/duplex_pipe

parent_md5=$(md5sum parent.txt | cut -d' ' -f1)
child_md5=$(md5sum child.txt | cut -d' ' -f1)

if [ "$parent_md5" = "$child_md5" ]; then
    echo "OK"
else
    echo "FAIL"
    echo "Parent: $parent_md5" 
    echo "Child:  $child_md5"
    exit 1
fi
