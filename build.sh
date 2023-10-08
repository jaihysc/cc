#!/bin/sh

help_menu() {
    echo "Usage: build.sh [debug|release]"
}

cd "$(dirname "$0")" || exit 1

# create out directory
if [ ! -d "out" ]; then
    mkdir out || exit 2
fi

# Build mode
if [ -z "$1" ]; then
    build_type="Debug"
elif [ "$1" = "debug" ]; then
    build_type="Debug"
elif [ "$1" = "release" ]; then
    build_type="Release"
else
    help_menu
    exit 10
fi

cd out || exit 3
cmake ../ -DCMAKE_BUID_TYPE="${build_type}" || exit 4
cmake --build . || exit 5

exit 0
