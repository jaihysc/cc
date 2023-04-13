#include "symbol.h"

#include <stddef.h>

#include "common.h"

SymbolId symid_invalid = {.index = -1};

/* Maps symbol type to string (index with symbol type) */
#define SYMBOL_TYPE(name__) #name__,
const char* symbol_type_str[] = {SYMBOL_TYPES};
#undef SYMBOL_TYPES

int symid_valid(SymbolId sym_id) {
    return sym_id.index != -1;
}

int symid_equal(SymbolId a, SymbolId b) {
    return a.index == b.index && a.scope == b.scope;
}

ErrorCode symbol_construct(
        Symbol* sym,
        const char* token,
        Type type,
        ValueCategory valcat,
        int scope_num) {
    sym->class = sl_normal;

    // Copy token into symbol
    int i = 0;
    while (token[i] != '\0') {
        if (i >= MAX_SYMBOL_LEN) {
            sym->token[0] = '\0';
            return ec_symbol_nametoolong;
        }
        sym->token[i] = token[i];
        ++i;
    }

    sym->type = type;
    sym->valcat = valcat;
    sym->scope_num = scope_num;
    return ec_noerr;
}

void symbol_sl_access(Symbol* sym, SymbolId ptr, SymbolId idx) {
    ASSERT(sym != NULL, "Symbol is null");
    sym->class = sl_access;
    sym->ptr = ptr;
    sym->ptr_idx = idx;
}

SymbolClass symbol_class(Symbol* sym) {
    return sym->class;
}

char* symbol_token(Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->token;
}

Type symbol_type(Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->type;
}

void symbol_set_type(Symbol* sym, const Type* type) {
    ASSERT(sym != NULL, "Symbol is null");
    sym->type = *type;
}

ValueCategory symbol_valcat(Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->valcat;
}

int symbol_scope_num(Symbol* sym) {
    return sym->scope_num;
}

SymbolId symbol_ptr_sym(Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->ptr;
}

SymbolId symbol_ptr_index(Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->ptr_idx;
}

const char* st_str(SymbolType st) {
    return symbol_type_str[(int)st];
}

