# Common

Shared implementation details among the different stages.

## Concepts

### Symbols

Symbols are stored alongside its attributes (token, type, ...) in the symbol table (symtab for short), the storage used changes depending on scope.

To reduce extra code which must be written to handle constants, symbol tables have special handling for constants. Constants can be added to symbol tables multiple times, looking up a constant in a symbol table will add the constant to the table if it does not exist and return the symbol, or if it does exist, return the existing symbol. This allows existing code for working with symbols such as `symbol_type` and `symbol_bytes` to be reused for constants as well.

During parsing, tokens are added to the symbol table alongside its attributes, indexed via a `SymbolId`. A symbol's attributes are as follows:

```
Symbol
  Index of token in parse token buffer
  Type
```
