/* Represents a symbol in the program */
#ifndef SYMBOL_H
#define SYMBOL_H

#include "errorcode.h"
#include "type.h"

#define MAX_SYMBOL_LEN 255 /* Max characters symbol name (exclude null terminator) */

typedef enum {
    sl_normal = 0,
    sl_access /* Represents access to memory location */
} SymbolClass;

typedef enum {
    vc_none = 0,
    vc_lval,
    vc_nlval
} ValueCategory;

typedef struct Symbol Symbol;
struct Symbol {
    SymbolClass class;
    char token[MAX_SYMBOL_LEN + 1];
    Type type;
    ValueCategory valcat;

    /* Only for class sl_access */
    Symbol* ptr;
    Symbol* ptr_idx;
};

/* Creates symbol at given memory location */
ErrorCode symbol_construct(Symbol* sym, const char* token, Type* type);

void symbol_destruct(Symbol* sym);

/* Converts symbol to class representing access to memory location
   ptr: Is a symbol which when indexed yields this symbol
   idx: Is a symbol which indexes into ptr to yield this
        symbol, leave as symid_invalid to default to 0
   e.g., int* p; int a = p[2];
   If this symbol is a, ptr is p, idx is 2 */
void symbol_sl_access(Symbol* sym, Symbol* ptr, Symbol* idx);

/* Returns SymbolClass for symbol */
SymbolClass symbol_class(Symbol* sym);

/* Returns token for symbol */
char* symbol_token(Symbol* sym);

/* Returns type for symbol */
Type* symbol_type(Symbol* sym);

/* Returns ValueCategory for symbol */
ValueCategory symbol_valcat(Symbol* sym);

/* Sets ValueCategory for symbol */
void symbol_set_valcat(Symbol* sym, ValueCategory valcat);

/* Returns the symbol for the pointer, which when indexed yields this symbol
   e.g., int* p; int a = p[2];
   If this symbol is a, the returned symbol is p */
Symbol* symbol_ptr_sym(Symbol* sym);

/* Returns the symbol of the index, which when used to index symbol_ptr_sym
   yields this symbol
   The index is always in bytes */
Symbol* symbol_ptr_index(Symbol* sym);

#endif
