#include "symbol.h"

#include <stddef.h>

#include "common.h"

SymbolId symid_invalid = {.index = -1};

int symid_valid(SymbolId sym_id) {
    return sym_id.index != -1;
}

int symid_equal(SymbolId a, SymbolId b) {
    return a.index == b.index && a.scope == b.scope;
}

ErrorCode symbol_construct(Symbol* sym, const char* token, Type type) {
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
    sym->token[i] = '\0';

    sym->type = type;
    sym->valcat = vc_none;
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

void symbol_set_valcat(Symbol* sym, ValueCategory valcat) {
    ASSERT(sym != NULL, "Symbol is null");
    sym->valcat = valcat;
}

SymbolId symbol_ptr_sym(Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->ptr;
}

SymbolId symbol_ptr_index(Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->ptr_idx;
}

