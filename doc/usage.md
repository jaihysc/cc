# Usage

## Options

Pass these on the command line, if running using `cc.sh`, prefix with `-P` to indicate the option is for the parser, e.g., `-Pdprint-symtab`.

| Flag | Description |
|-|-|
| `-dprint-cfg` | Prints out the Control Flow Graph (CFG) |
| `-dprint-parse-recursion` | Shows the recursive matching of language productions as the input C source file is parsed |
| `-dprint-tree` | Prints out the Abstract Syntax Tree (AST) |
| `-dprint-symtab` | Prints out the symbol table when it about to be cleared |

## Tests

To run the tests, run `runtest.py`. The test results are printed at the end with the names and output of any failed tests.

Test cases are defined in `test/`. The input file in each test case is a c source file with extension `.c`. A file of the same name with extension `.py` is ran to validate the compiled program's output. For example `integer_arithmetic.c` and `integer_arithmetic.py`.
