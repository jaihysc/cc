# Tests

To run the tests, run `runtest.py`. The test results are printed at the end with the names and output of any failed tests.

## Defining tests

Test cases are defined in `test/`. This tests the entire compilation process, it was chosen to not test the intermediate output to give it flexibility to change in the future.

The input file in each test case is a c source file with extension `.c`. A file of the same name (excluding .c) with extension `.py` is ran to validate the compiled program's output.

## Interface

Run `pydoc` on `test/runtest.py` for up to date documentation.

