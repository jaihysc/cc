#include "symtab.h"

#include "common.h"
#include "globals.h"

ErrorCode symtab_construct(Symtab* stab) {
    cmemzero(stab, sizeof(Symtab));
    return ec_noerr;
}

void symtab_push_cat(Symtab* stab, SymbolCat cat, SymbolId sym) {
    if (stab->i_cat[cat] >= MAX_SCOPES) {
        ASSERTF(0, "Pushed too many symbol for symbol category %d", cat);
    }
    int i = stab->i_cat[cat];
    ++stab->i_cat[cat];
    stab->cat[cat][i] = sym;
}

void symtab_pop_cat(Symtab* stab, SymbolCat cat) {
    ASSERTF(stab->i_cat[cat] > 0,
            "No symbol in symbol category %d to pop", cat);
    --stab->i_cat[cat];
}

SymbolId symtab_last_cat(Symtab* stab, SymbolCat cat) {
    ASSERTF(stab->i_cat[cat] > 0,
            "No symbol on symbol category %d stack", cat);
    return stab->cat[cat][stab->i_cat[cat] - 1];
}

ErrorCode symtab_push_scope(Symtab* stab) {
    if (g_debug_print_parse_recursion) {
        LOGF("Push scope. Depth %d, Number %d\n", stab->i_scope, stab->scope_num);
    }

    if (stab->i_scope >= MAX_SCOPES) {
        return ec_scopedepexceed;
    }
    /* Scope may have been previously used, reset index for symbol */
    stab->i_symbol[stab->i_scope] = 0;
    ++stab->i_scope;
    ++stab->scope_num;
    return ec_noerr;
}

void symtab_pop_scope(Symtab* stab) {
    ASSERT(stab->i_scope > 0, "No scope after popping first scope");

    if (g_debug_print_symtab) {
        debug_print_symtab(stab);
    }

    --stab->i_scope;
    if (g_debug_print_parse_recursion) {
        LOGF("Pop scope at depth %d\n", stab->i_scope);
    }
    if (g_debug_print_buffers) {
        debug_print_symtab(stab);
    }
}

/* Finds provided token within indicated scope
   Returns symid_invalid if not found */
static SymbolId symtab_find_scoped(Symtab* stab, int i_scope, const char* token) {
    for (int i = 0; i < stab->i_symbol[i_scope]; ++i) {
        Symbol* sym = stab->symbol[i_scope] + i;
        if (strequ(symbol_token(sym), token)) {
            SymbolId id;
            id.scope = i_scope;
            id.index = i;
            return id;
        }
    }
    return symid_invalid;
}

SymbolId symtab_find(Symtab* stab, const char* token) {
    for (int i_scope = stab->i_scope - 1; i_scope >= 0; --i_scope) {
        SymbolId found_id = symtab_find_scoped(stab, i_scope, token);
        if (symid_valid(found_id)) {
            return found_id;
        }
    }
    return symid_invalid;
}

Symbol* symtab_get(Symtab* stab, SymbolId sym_id) {
    ASSERT(symid_valid(sym_id), "Invalid symbol id");
    ASSERT(sym_id.index < stab->i_symbol[sym_id.scope], "Symbol id out of range");
    return stab->symbol[sym_id.scope] + sym_id.index;
}

const char* symtab_get_token(Symtab* stab, SymbolId sym_id) {
    return symbol_token(symtab_get(stab, sym_id));
}

Type symtab_get_type(Symtab* stab, SymbolId sym_id) {
    return symbol_type(symtab_get(stab, sym_id));
}

ValueCategory symtab_get_valcat(Symtab* stab, SymbolId sym_id) {
    return symbol_valcat(symtab_get(stab, sym_id));
}

int symtab_get_scope_num(Symtab* stab, SymbolId sym_id) {
    return symbol_scope_num(symtab_get(stab, sym_id));
}

/* Creates symbol with provided information in symbol table
    token: Set null to create unnamed symbol
   Stores SymbolId of added symbol at pointer,
   or symid_invalid if it already exists */
static ErrorCode symtab_add_scoped(Symtab* stab, SymbolId* symid_ptr,
        int i_scope, const char* token, Type type, ValueCategory valcat) {
    ASSERT(stab->i_scope > 0, "Invalid scope");

    if (token != NULL) {
        /* Normal symbols can not have duplicates in same scope */
        if (symid_valid(symtab_find_scoped(stab, i_scope, token))) {
            ERRMSGF("Symbol already exists %s\n", token);
            *symid_ptr = symid_invalid;
            return ec_symtab_dupname;
        }
    }

    SymbolId id;
    id.scope = i_scope;
    id.index = stab->i_symbol[i_scope];
    if (id.index >= MAX_SCOPE_LEN) {
        *symid_ptr = symid_invalid;
        return ec_scopelenexceed;
    }

    Symbol* sym = stab->symbol[i_scope] + id.index;
    ++stab->i_symbol[i_scope];

    /* Indicate global scope for parser_output_il */
    int scope_num;
    if (i_scope == 0) {
        scope_num = 0;
    }
    else {
        scope_num = stab->scope_num;
    }

    symbol_construct(sym, token, type, valcat, scope_num);
    *symid_ptr = id;
    return ec_noerr;
}

ErrorCode symtab_add(Symtab* stab, SymbolId* symid_ptr,
        const char* token, Type type, ValueCategory valcat) {
    if (token != NULL) {
        /* Special handling for constants: Duplicates can exist */
        if ('0' <= token[0] && token[0] <= '9') {
            SymbolId const_id = symtab_find_scoped(stab, 0, token);
            if (symid_valid(const_id)) {
                ASSERT(type_equal(symtab_get_type(stab, const_id), type),
                        "Same constant name with different types");
                *symid_ptr = const_id;
                return ec_noerr;
            }
            /* Use index 0 for adding constants */
            /* Note this is a little wasteful since it is rechecking
               if the symbol exists */
            return symtab_add_scoped(stab, symid_ptr, 0, token, type, valcat);
        }
    }

    /* Add new symbol to current scope */
    ASSERT(stab->i_scope > 0, "No scope exists");
    int curr_scope = stab->i_scope - 1;
    return symtab_add_scoped(stab, symid_ptr, curr_scope, token, type, valcat);
}

ErrorCode symtab_add_temporary(Symtab* stab, SymbolId* symid_ptr, Type type) {
    AAPPENDI(token, "__t", stab->temp_num);
    ++stab->temp_num;

    return symtab_add(stab, symid_ptr, token, type, vc_nlval);
}

ErrorCode symtab_add_label(Symtab* stab, SymbolId* symid_ptr) {
    AAPPENDI(token, "__l", stab->label_num);
    ++stab->label_num;

    ASSERT(stab->i_scope >= 2, "No function scope");
    /* Scope at index 1 is function scope */
    return symtab_add_scoped(stab, symid_ptr, 1, token, type_label, vc_nlval);
}

void debug_print_symtab(Symtab* stab) {
    LOG("Symbol table:\n");
    LOGF("Scopes: [%d]\n", stab->i_scope);

    for (int i = 0; i < stab->i_scope; ++i) {
        LOGF("  %d [%d]\n", i, stab->i_symbol[i]);

        for (int j = 0; j < stab->i_symbol[i]; ++j) {
            Symbol* sym = stab->symbol[i] + j;
            Type type = symbol_type(sym);
            LOGF("    %d %s", j, type_specifiers_str(type.typespec));

            for (int k = 0; k < type.pointers; ++k) {
                LOG("*");
            }
            LOGF(" %s\n", symbol_token(sym));
        }
    }
}
