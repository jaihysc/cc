#!/bin/sh

help_menu() {
    echo "Usage: build.sh [debug|release]"
}

cd "$(dirname "$0")" || exit 1

# create out directory
if [ ! -d "out" ]; then
    mkdir out || exit 2
fi
if [[ ! -d out/src ]]; then
    mkdir out/src || exit 2
fi
if [[ ! -d out/testu ]]; then
    mkdir out/testu || exit 2
fi

cc_flags="-Wall -Wextra -Wshadow -pedantic -Wmissing-prototypes -Wpointer-arith -Wcast-qual -Wswitch-default\
    -Wswitch -Wbad-function-cast -Wlogical-op -Wredundant-decls -Wconversion -Wsign-conversion -Wdouble-promotion -Wmisleading-indentation\
    -Wduplicated-cond -Wduplicated-branches -Wunreachable-code -Wuninitialized -Wmaybe-uninitialized -Wundef\
    -Wno-unused-parameter"

cc_debug_flags="-g"
cc_release_flags="-O3"

# Build mode
if [ -z "$1" ]; then
    # Default to debug mode
    cc_flags="${cc_flags} ${cc_debug_flags}"
elif [ "$1" = "debug" ]; then
    cc_flags="${cc_flags} ${cc_debug_flags}"
elif [ "$1" = "release" ]; then
    cc_flags="${cc_flags} ${cc_release_flags}"
else
    help_menu
    exit 10
fi

export cc_flags
make

cp "src/cc.sh" out/ || exit 3

exit 0
