#!/bin/sh

cd "$(dirname "$0")" || exit 1

# create out directory
if [ ! -d "out" ]; then
  mkdir out || exit 2
fi

cc_flags="-Wall -Wextra -Wshadow -pedantic -Wmissing-prototypes -Wpointer-arith -Wcast-qual -Wswitch-default\
    -Wswitch -Wbad-function-cast -Wlogical-op -Wredundant-decls -Wsign-conversion -Wdouble-promotion -Wmisleading-indentation\
    -Wduplicated-cond -Wduplicated-branches\
    -Wno-unused-parameter"

cc $cc_flags src/parse.c          -o out/parse
cc $cc_flags src/asmgen/asm_gen.c -o out/asm_gen

cp "src/cc.sh" out/ || exit 3

exit 0
