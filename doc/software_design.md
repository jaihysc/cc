# Software Design

## Files

* `src` Source code for the compiler
* `test` Source code of programs for testing the compiler
* `testu` Unit tests for the compiler

## Objects

Classes are emulated with manual constructors and destructors. All classes have an explicit destructor call in case it is necessary in the future.

```c
Lexer lex;
ErrorCode code = lexer_construct(&lex);

/* ... */

lexer_destruct(&lex);
```