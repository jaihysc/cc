# Software Design

## Objects

Classes are emulated with manual constructors and destructors. All classes have an explicit destructor call in case it is necessary in the future.

```c
Lexer lex;
ErrorCode code = lexer_construct(&lex);

/* ... */

lexer_destruct(&lex);
```