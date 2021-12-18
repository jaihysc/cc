# Tests

To run the tests, run `runtest.py`.

## Defining tests

Test cases are defined in `test/`. This tests the entire compilation process, it was chosen to not test the intermediate output to give it flexibility to change in the future.

The input file in each test case is a c source file with extension `.c`. A file of the same name (excluding .c) with extension `.py` is ran to validate the compiled program's output.

The python file has access to the object `_prog` of type Program.

```
class Program

run(validate, args=[])
    validate: function of parameter (ProgramResult, Validator), called if program successfully ran to check results
    args: list
    Runs the program with the specified args
```

```
class ProgramResult

stdout -> str
    What the program printed to stdout, "" if nothing

exitcode -> number
    What the program exited with
```

```
class Validator

expect(b)
    b: boolean
    Standard usage is to enter an expression into expect, e.g., expect(r.exitcode == 10)
```

