# Common

Shared implementation details among the different stages.

# Symbols

To reduce extra code which must be written to handle constants, symbol tables have special handling for constants. Looking up a constant in a symbol table will add the constant to the table if it does not exist and return the symbol, or if it does exist, return the existing symbol. This allows existing code for working with symbols such as `symbol_type` and `symbol_bytes` to be reused for constants as well.

