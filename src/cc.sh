#!/bin/bash

if [ -z "$1" ]; then
    echo "No input files"
    exit 2
fi

gcc -E -P -x c "$1" -o "$(dirname "$1")/imm1" || exit 5
"$(dirname "$0")/parse"   "$(dirname "$1")/imm1" || exit 6
"$(dirname "$0")/asm_gen" "$(dirname "$1")/imm2" || exit 7
nasm -felf64 "$(dirname "$1")/imm3" -o "$(dirname "$1")/imm4" || exit 8
ld           "$(dirname "$1")/imm4" || exit 9

exit 0

