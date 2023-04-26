#include "symtab.h"

#include "common.h"
#include "globals.h"

ErrorCode symtab_construct(Symtab* stab) {
    ASSERT(stab != NULL, "Symtab is null");
    ErrorCode ecode;

    hvec_construct(&stab->symbol);
    hvec_construct(&stab->constant);

    if ((ecode = type_construct(&stab->type_int, ts_int, 0)) != ec_noerr) return ecode;
    if ((ecode = type_construct(&stab->type_label, ts_void, 0)) != ec_noerr) return ecode;

    /* Create special constants */
    Symbol sym_0;
    symbol_construct(&sym_0, "0", &stab->type_int); /* Index 0 */
    if (!hvec_push_back(&stab->constant, sym_0)) return ec_badalloc;

    Symbol sym_1;
    symbol_construct(&sym_1, "1", &stab->type_int); /* Index 0 */
    if (!hvec_push_back(&stab->constant, sym_1)) return ec_badalloc;

    stab->scopes = NULL;
    stab->scopes_size = 0;
    stab->scopes_capacity = 0;

    for (int i = 0; i < sc_count; ++i) {
        vec_construct(&stab->cat[i]);
    }

    stab->temp_num = 0;
    stab->label_num = 0;
    return ec_noerr;
}

void symtab_destruct(Symtab* stab) {
    ASSERT(stab != NULL, "Symtab is null");

    for (int i = 0; i < sc_count; ++i) {
        vec_destruct(&stab->cat[i]);
    }
    /* scopes are always constructed up to capacity */
    for (int i = 0; i < stab->scopes_capacity; ++i) {
        vec_destruct(&stab->scopes[i]);
    }
    cfree(stab->scopes);

    type_destruct(&stab->type_label);
    type_destruct(&stab->type_int);

    for (int i = 0; i < hvec_size(&stab->constant); ++i) {
        Symbol* sym = &hvec_at(&stab->constant, i);
        symbol_destruct(sym);
    }
    hvec_destruct(&stab->constant);

    for (int i = 0; i < hvec_size(&stab->symbol); ++i) {
        Symbol* sym = &hvec_at(&stab->symbol, i);
        symbol_destruct(sym);
    }
    hvec_destruct(&stab->symbol);
}

ErrorCode symtab_push_cat(Symtab* stab, SymbolCat cat, Symbol* sym) {
    ASSERT(stab != NULL, "Symtab is null");

    if (!vec_push_backu(&stab->cat[cat])) return ec_badalloc;

    vec_back(&stab->cat[cat]) = sym;
    return ec_noerr;
}

void symtab_pop_cat(Symtab* stab, SymbolCat cat) {
    ASSERT(stab != NULL, "Symtab is null");
    ASSERTF(vec_size(&stab->cat[cat]) > 0,
            "No symbol in symbol category %d to pop", cat);

    (void)vec_pop_back(&stab->cat[cat]);
}

Symbol* symtab_last_cat(Symtab* stab, SymbolCat cat) {
    ASSERT(stab != NULL, "Symtab is null");
    ASSERTF(vec_size(&stab->cat[cat]) > 0,
            "No symbol on symbol category %d stack", cat);
    return vec_back(&stab->cat[cat]);
}

ErrorCode symtab_push_scope(Symtab* stab) {
    ASSERT(stab != NULL, "Symtab is null");

    if (g_debug_print_parse_recursion) {
        LOGF("Push scope. Depth %d\n", stab->scopes_size);
    }

    /* Resize scopes */
    if (stab->scopes_size >= stab->scopes_capacity) {
        int old_capacity = stab->scopes_capacity;

        stab->scopes_capacity = stab->scopes_capacity * 2 + 1;
        Symtab_Scope* new_buf = cmalloc(
                (size_t)stab->scopes_capacity * sizeof(Symtab_Scope));
        if (new_buf == NULL) return ec_badalloc;

        /* Copy over the already constructed scopes */
        for (int i = 0; i < old_capacity; ++i) {
            new_buf[i] = stab->scopes[i];
        }

        /* Construct the newly allocated scopes */
        for (int i = old_capacity; i < stab->scopes_capacity; ++i) {
            vec_construct(&new_buf[i]);
        }

        cfree(stab->scopes);
        stab->scopes = new_buf;
    }

    /* Clear existing scope */
    vec_clear(&stab->scopes[stab->scopes_size]);

    ++stab->scopes_size;
    return ec_noerr;
}

void symtab_pop_scope(Symtab* stab) {
    ASSERT(stab != NULL, "Symtab is null");
    ASSERT(stab->scopes_size > 0, "No scope after popping first scope");

    if (g_debug_print_symtab) {
        debug_print_symtab(stab);
    }

    --stab->scopes_size;
    if (g_debug_print_parse_recursion) {
        LOGF("Pop scope at depth %d\n", stab->scopes_size);
    }
}

/* Finds provided token within indicated scope
   Returns NULL if not found */
static Symbol* symtab_find_scoped(Symtab* stab, int i_scope, const char* token) {
    ASSERT(stab != NULL, "Symtab is null");

    for (int i = 0; i < vec_size(&stab->scopes[i_scope]); ++i) {
        Symbol* sym = vec_at(&stab->scopes[i_scope], i);
        if (strequ(symbol_token(sym), token)) return sym;
    }
    return NULL;
}

Symbol* symtab_find(Symtab* stab, const char* token) {
    ASSERT(stab != NULL, "Symtab is null");

    for (int i_scope = stab->scopes_size - 1; i_scope >= 0; --i_scope) {
        Symbol* found = symtab_find_scoped(stab, i_scope, token);
        if (found != NULL) return found;
    }
    return NULL;
}

/* Allocates storage for symbol in symbol table
   Stores Symbol* of added symbol at pointer
   or ec_symtab_dupname if it already exists */
static ErrorCode symtab_add_scoped(Symtab* stab, Symbol** sym_ptr,
        int i_scope, const char* token, Type* type) {
    ASSERT(stab != NULL, "Symtab is null");
    ASSERT(stab->scopes_size > 0, "Invalid scope");

    /* Add to vec of symbols in scope */
    if (token != NULL) {
        /* Normal symbols can not have duplicates in same scope */
        if (symtab_find_scoped(stab, i_scope, token) != NULL) {
            ERRMSGF("Symbol already exists %s\n", token);
            *sym_ptr = NULL;
            return ec_symtab_dupname;
        }
    }

    /* Add to vec of symbols */
    if (!hvec_push_backu(&stab->symbol)) return ec_badalloc;
    Symbol* sym = &hvec_back(&stab->symbol);

    if (!vec_push_backu(&stab->scopes[i_scope])) return ec_badalloc;
    vec_back(&stab->scopes[i_scope]) = sym;

    symbol_construct(sym, token, type);
    *sym_ptr = sym;
    return ec_noerr;
}

ErrorCode symtab_add(Symtab* stab, Symbol** sym_ptr,
        const char* token, Type* type) {
    ASSERT(stab != NULL, "Symtab is null");
    ASSERT(token != NULL, "token is null");

    /* Constants should not be added to symbol table */
    ASSERT(token[0] < '0' || '9' < token[0], "Attempted to add constant to symbol table");

    /* Add new symbol to current scope */
    ASSERT(stab->scopes_size > 0, "No scope exists");
    int curr_scope = stab->scopes_size - 1;
    return symtab_add_scoped(stab, sym_ptr, curr_scope, token, type);
}

ErrorCode symtab_add_constant(Symtab* stab, Symbol** sym_ptr,
        const char* token, Type* type) {
    ASSERT(stab != NULL, "Symtab is null");
    ASSERT(token != NULL, "token is null");
    ASSERT('0' <= token[0] && token[0] <= '9', "Attempted to add non-constant to symbol table");

    ErrorCode ecode;
    if (!hvec_push_backu(&stab->constant)) return ec_badalloc;

    Symbol* sym = &hvec_back(&stab->constant);
    if ((ecode = symbol_construct(sym, token, type)) != ec_noerr) return ecode;
    *sym_ptr = sym;
    return ec_noerr;
}

ErrorCode symtab_add_temporary(Symtab* stab, Symbol** sym_ptr, Type* type) {
    ASSERT(stab != NULL, "Symtab is null");
    AAPPENDI(token, "__t", stab->temp_num);
    ++stab->temp_num;

    ErrorCode ecode;
    if ((ecode = symtab_add(stab, sym_ptr, token, type)) != ec_noerr) return ecode;
    symbol_set_valcat(*sym_ptr, vc_nlval);
    return ec_noerr;
}

ErrorCode symtab_add_label(Symtab* stab, Symbol** sym_ptr) {
    ASSERT(stab != NULL, "Symtab is null");
    AAPPENDI(token, "__l", stab->label_num);
    ++stab->label_num;

    ASSERT(stab->scopes_size >= 2, "No function scope");
    /* Scope at index 1 is function scope */
    ErrorCode ecode;
    if ((ecode = symtab_add_scoped(
                    stab, sym_ptr, 1, token, &stab->type_label)) != ec_noerr) return ecode;
    symbol_set_valcat(*sym_ptr, vc_nlval);
    return ec_noerr;
}

Symbol* symtab_constant_zero(Symtab* stab) {
    ASSERT(stab != NULL, "Symtab is null");
    return &hvec_at(&stab->constant, 0);
}

Symbol* symtab_constant_one(Symtab* stab) {
    ASSERT(stab != NULL, "Symtab is null");
    return &hvec_at(&stab->constant, 1);
}

Type* symtab_type_int(Symtab* stab) {
    ASSERT(stab != NULL, "Symtab is null");
    return &stab->type_int;
}

void debug_print_symtab(Symtab* stab) {
    ASSERT(stab != NULL, "Symtab is null");

    LOG("Symbol table:\n");
    LOGF("Scopes: [%d]\n", stab->scopes_size);

    for (int i = 0; i < stab->scopes_size; ++i) {
        int symbols_size = vec_size(&stab->scopes[i]);
        LOGF("  %d [%d]\n", i, symbols_size);

        for (int j = 0; j < symbols_size; ++j) {
            Symbol* sym = vec_at(&stab->scopes[i], j);
            Type* type = symbol_type(sym);
            LOGF("    %d %s", j, ts_str(type_typespec(type)));

            for (int k = 0; k < type_pointer(type); ++k) {
                LOG("*");
            }
            LOGF(" %s\n", symbol_token(sym));
        }
    }

    LOGF("Constants: [%d]\n", hvec_size(&stab->constant));
    for (int i = 0; i < hvec_size(&stab->constant); ++i) {
        Symbol* sym = &hvec_at(&stab->constant, i);
        Type* type = symbol_type(sym);
        LOGF("    %d %s", i, ts_str(type_typespec(type)));
        LOGF(" %s\n", symbol_token(sym));
    }
}
