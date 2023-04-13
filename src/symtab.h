/* Compiler symbol table */
#ifndef SYMTAB_H
#define SYMTAB_H

#include "symbol.h"
#include "errorcode.h"

#define MAX_SCOPES 32 /* Max number of scopes */
#define MAX_SCOPE_LEN 500   /* Max symbols per scope */

/* The category(purpose) a symbol serves
   e.g., label for end of loop body */
typedef enum {
    /* Label placed right before condition check to loop loop/exit */
    sc_lab_loopbodyend = 0,
    /* Label placed after loop */
    sc_lab_loopend,
    sc_count
} SymbolCat;

typedef struct {
    /* First scope is most global
       First symbol element is earliest in occurrence
       Constants always use the first scope (at index 0) */
    Symbol symbol[MAX_SCOPES][MAX_SCOPE_LEN];
    int i_scope; /* Index one past end of latest(deepest) scope. */
    int i_symbol[MAX_SCOPES]; /* Index one past last element for each scope */

    /* Number to uniquely identify every scope created */
    int scope_num;
    /* Number to create unique temporary/label/etc values */
    int temp_num;
    int label_num;

    /* Stack for symbol category, push and pop with
       symtab_push_cat()
       symtab_pop_cat()
       Use MAX_SCOPES as maximum for now, only need to push at most once
       per scope */
    SymbolId cat[sc_count][MAX_SCOPES];
    int i_cat[sc_count]; /* Points one past last element */
} Symtab;

ErrorCode symtab_construct(Symtab* stab);


/* Categories */


/* Pushes symbol onto symbol category stack */
void symtab_push_cat(Symtab* stab, SymbolCat cat, SymbolId sym);

/* Pops symbol from top of symbol category stack */
void symtab_pop_cat(Symtab* stab, SymbolCat cat);

/* Returns the last symbol for symbol category (top of stack) */
SymbolId symtab_last_cat(Symtab* stab, SymbolCat cat);


/* Scopes */


/* Sets up a new symbol scope to be used */
ErrorCode symtab_push_scope(Symtab* stab);

/* Removes current symbol scope, now uses the last scope */
void symtab_pop_scope(Symtab* stab);

/* Finds provided token in symbol table, closest scope first
   Returns symid_invalid if not found */
SymbolId symtab_find(Symtab* stab, const char* token);

/* Returns symbol at provided SymbolId */
Symbol* symtab_get(Symtab* stab, SymbolId sym_id);


/* Short forms to reduce typing */


/* Returns token for SymbolId */
const char* symtab_get_token(Symtab* stab, SymbolId sym_id);

/* Returns type for SymbolId */
Type symtab_get_type(Symtab* stab, SymbolId sym_id);

/* Returns ValueCategory for SymbolId */
ValueCategory symtab_get_valcat(Symtab* stab, SymbolId sym_id);

/* Returns number guaranteed to be unique for each scope for SymbolId*/
int symtab_get_scope_num(Symtab* stab, SymbolId sym_id);


/* Add to symbol table */


/* Creates symbol with provided information in symbol table
    token: Set null to create unnamed symbol
    Has special handling for constants, always added to scope index 0
   Stores SymbolId of added symbol at pointer,
   or symid_invalid if it already exists */
ErrorCode symtab_add(Symtab* stab, SymbolId* symid_ptr,
        const char* token, Type type, ValueCategory valcat);

/* Creates a new temporary for the current scope in symbol table */
ErrorCode symtab_add_temporary(Symtab* stab, SymbolId* symid_ptr, Type type);

/* Creates a new label for the current scope in symbol table */
ErrorCode symtab_add_label(Symtab* stab, SymbolId* symid_ptr);


/* Debug */


void debug_print_symtab(Symtab* stab);

#endif
