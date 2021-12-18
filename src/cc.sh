#!/bin/bash

cd "$(dirname "$0")" || exit 1

if [ -z "$1" ] then
    echo "No input files"
    exit 2
fi

gcc -E -P -x c "$1" -o imm1 || exit 5
./parse imm1 || exit 6
./asm_gen imm2 || exit 7
nasm -felf64 imm3 -o imm4 || exit 8
ld imm4 || exit 9

exit 0

