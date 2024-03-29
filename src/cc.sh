#!/bin/bash

# Preprocessor, Parse, Asmgen, Assembler, Linker
pp_flags=()
parse_flags=()
ag_flags=()
asm_flags=()
ln_flags=()

input_files=()
handle_flags=true

fail() {
    echo -e "$1"
    exit "$2"
}

for arg in "$@"
do
    # Handle flags
    if [[ "$handle_flags" == true ]] && [[ $arg =~ ^-.* ]]; then
        if [[ $arg == "--" ]]; then
            handle_flags=false
        elif [[ $arg == "--help" ]]; then
            echo "Usage: cc [Prefix][Flag] file..."
            echo "Prefixes"
            echo "-E prEprocessor"
            echo "-P Parser"
            echo "-S aSsembly generator"
            echo "-A Assembler"
            echo "-L Linker"
            echo "Pass the prefix to indicate the compilation stage to pass the flag to, followed by flag for the stage"
            echo "Example: -EE -Ex \"-PZd ef gh\" to pass the flags -E -x to the preprocessor and -Zd ef gh to the parser"
            exit 1
        elif [[ "$arg" =~ ^-E.* ]]; then
            pp_flags+=("-${arg:2}") # Discard the prefix
        elif [[ "$arg" =~ ^-P.* ]]; then
            parse_flags+=("-${arg:2}")
        elif [[ "$arg" =~ ^-S.* ]]; then
            ag_flags+=("-${arg:2}")
        elif [[ "$arg" =~ ^-A.* ]]; then
            asm_flags+=("-${arg:2}")
        elif [[ "$arg" =~ ^-L.* ]]; then
            ln_flags+=("-${arg:2}")
        else
            echo "Unrecognized prefix $arg"
        fi
    else
        # Remaining treated as files
        input_files+=("$arg")
    fi
done

if [ ${#input_files[@]} -eq 0 ]; then
    echo "No input files"
    exit 2
elif [ ${#input_files[@]} -eq 1 ]; then
    file=${input_files[0]}
    gcc -E -x c               "${pp_flags[@]}"    "$file"                   -o "$(dirname "$file")/imm1"  || fail "Preprocessor error"       5
    "$(dirname "$0")/parse"   "${parse_flags[@]}" "$(dirname "$file")/imm1" -o "$(dirname "$file")/imm2"  || fail "Parser error"             6
    "$(dirname "$0")/asmgen"  "${ag_flags[@]}"    "$(dirname "$file")/imm2" -o "$(dirname "$file")/imm3"  || fail "Assembly generator error" 7
    nasm -felf64              "${asm_flags[@]}"   "$(dirname "$file")/imm3" -o "$(dirname "$file")/imm4"  || fail "Assembler error"          8
    ld                        "${ln_flags[@]}"    "$(dirname "$file")/imm4" -o "$(dirname "$file")/a.out" || fail "Linker error"             9
else
    echo "Only 1 input file supported"
    exit 3
fi

exit 0

