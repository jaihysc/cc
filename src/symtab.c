#include "symtab.h"

#include "common.h"
#include "globals.h"

ErrorCode symtab_construct(Symtab* stab) {
    ASSERT(stab != NULL, "Symtab is null");

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
}

ErrorCode symtab_push_cat(Symtab* stab, SymbolCat cat, SymbolId sym) {
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

SymbolId symtab_last_cat(Symtab* stab, SymbolCat cat) {
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
    if (g_debug_print_buffers) {
        debug_print_symtab(stab);
    }
}

/* Finds provided token within indicated scope
   Returns symid_invalid if not found */
static SymbolId symtab_find_scoped(Symtab* stab, int i_scope, const char* token) {
    ASSERT(stab != NULL, "Symtab is null");

    for (int i = 0; i < vec_size(&stab->scopes[i_scope]); ++i) {
        Symbol* sym = &vec_at(&stab->scopes[i_scope], i);
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
    ASSERT(stab != NULL, "Symtab is null");

    for (int i_scope = stab->scopes_size - 1; i_scope >= 0; --i_scope) {
        SymbolId found_id = symtab_find_scoped(stab, i_scope, token);
        if (symid_valid(found_id)) {
            return found_id;
        }
    }
    return symid_invalid;
}

Symbol* symtab_get(Symtab* stab, SymbolId sym_id) {
    ASSERT(stab != NULL, "Symtab is null");
    ASSERT(symid_valid(sym_id), "Invalid symbol id");
    ASSERT(sym_id.index < vec_size(&stab->scopes[sym_id.scope]), "Symbol id out of range");
    return &vec_at(&stab->scopes[sym_id.scope], sym_id.index);
}

const char* symtab_get_token(Symtab* stab, SymbolId sym_id) {
    ASSERT(stab != NULL, "Symtab is null");
    return symbol_token(symtab_get(stab, sym_id));
}

Type symtab_get_type(Symtab* stab, SymbolId sym_id) {
    ASSERT(stab != NULL, "Symtab is null");
    return symbol_type(symtab_get(stab, sym_id));
}

ValueCategory symtab_get_valcat(Symtab* stab, SymbolId sym_id) {
    ASSERT(stab != NULL, "Symtab is null");
    return symbol_valcat(symtab_get(stab, sym_id));
}

/* Allocates storage for symbol in symbol table
   Stores SymbolId of added symbol at pointer
   or ec_symtab_dupname if it already exists */
static ErrorCode symtab_add_scoped(Symtab* stab, SymbolId* symid_ptr,
        int i_scope, const char* token, Type type) {
    ASSERT(stab != NULL, "Symtab is null");
    ASSERT(stab->scopes_size > 0, "Invalid scope");

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
    id.index = vec_size(&stab->scopes[i_scope]);

    if (!vec_push_backu(&stab->scopes[i_scope])) return ec_badalloc;
    Symbol* sym = &vec_back(&stab->scopes[i_scope]);

    symbol_construct(sym, token, type);
    *symid_ptr = id;
    return ec_noerr;
}

ErrorCode symtab_add(Symtab* stab, SymbolId* symid_ptr,
        const char* token, Type type) {
    ASSERT(stab != NULL, "Symtab is null");
    ASSERT(token != NULL, "token is null");

    /* Constants should not be added to symbol table */
    ASSERT(token[0] < '0' || '9' < token[0], "Attempted to add constant to symbol table");

    /* Add new symbol to current scope */
    ASSERT(stab->scopes_size > 0, "No scope exists");
    int curr_scope = stab->scopes_size - 1;
    return symtab_add_scoped(stab, symid_ptr, curr_scope, token, type);
}

ErrorCode symtab_add_temporary(Symtab* stab, SymbolId* symid_ptr, Type type) {
    ASSERT(stab != NULL, "Symtab is null");
    AAPPENDI(token, "__t", stab->temp_num);
    ++stab->temp_num;

    ErrorCode ecode;
    if ((ecode = symtab_add(stab, symid_ptr, token, type)) != ec_noerr) return ecode;
    symbol_set_valcat(symtab_get(stab, *symid_ptr), vc_nlval);
    return ec_noerr;
}

ErrorCode symtab_add_label(Symtab* stab, SymbolId* symid_ptr) {
    ASSERT(stab != NULL, "Symtab is null");
    AAPPENDI(token, "__l", stab->label_num);
    ++stab->label_num;

    ASSERT(stab->scopes_size >= 2, "No function scope");
    /* Scope at index 1 is function scope */
    ErrorCode ecode;
    if ((ecode = symtab_add_scoped(
                    stab, symid_ptr, 1, token, type_label)) != ec_noerr) return ecode;
    symbol_set_valcat(symtab_get(stab, *symid_ptr), vc_nlval);
    return ec_noerr;
}

void debug_print_symtab(Symtab* stab) {
    ASSERT(stab != NULL, "Symtab is null");

    LOG("Symbol table:\n");
    LOGF("Scopes: [%d]\n", stab->scopes_size);

    for (int i = 0; i < stab->scopes_size; ++i) {
        int symbols_size = vec_size(&stab->scopes[i]);
        LOGF("  %d [%d]\n", i, symbols_size);

        for (int j = 0; j < symbols_size; ++j) {
            Symbol* sym = &vec_at(&stab->scopes[i], j);
            Type type = symbol_type(sym);
            LOGF("    %d %s", j, ts_str(type.typespec));

            for (int k = 0; k < type.pointers; ++k) {
                LOG("*");
            }
            LOGF(" %s\n", symbol_token(sym));
        }
    }
}
