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

void symbol_construct(
        Symbol* sym,
        TokenId tok_id,
        Type type,
        ValueCategory valcat,
        int scope_num) {
    sym->class = sl_normal;
    sym->tok_id = tok_id;
    sym->type = type;
    sym->valcat = valcat;
    sym->scope_num = scope_num;
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
    // FIXME
    //return parser_get_token(p, sym->tok_id);
    ASSERT(0, "Unimplemented");
    return NULL;
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

SymbolType st_from_token_id(int index) {
    return -(index + 1);
}

TokenId st_to_token_id(SymbolType type) {
    return -type - 1;
}
