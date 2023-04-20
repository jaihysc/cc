/* Compiler symbol table */
#ifndef SYMTAB_H
#define SYMTAB_H

#include "errorcode.h"
#include "symbol.h"
#include "vec.h"

/* The category(purpose) a symbol serves
   e.g., label for end of loop body */
typedef enum {
    /* Label placed right before condition check to loop loop/exit */
    sc_lab_loopbodyend = 0,
    /* Label placed after loop */
    sc_lab_loopend,
    sc_count
} SymbolCat;

typedef vec_t(Symbol*) Symtab_Scope;

typedef struct {
    /* Holds all symbols ever added */
    hvec_t(Symbol) symbol;

    /* Holds all constants symbols ever added */
    hvec_t(Symbol) constant;

    /* First scope is most global
       First symbol element is earliest in occurrence
       Points to a symbol in the symbol vector */
    Symtab_Scope* scopes;
    int scopes_size;
    int scopes_capacity;

    /* Stack for symbol category, push and pop with
       symtab_push_cat()
       symtab_pop_cat() */
    vec_t(Symbol*) cat[sc_count];

    /* Number to create unique temporary/label/etc values */
    int temp_num;
    int label_num;
} Symtab;

ErrorCode symtab_construct(Symtab* stab);

void symtab_destruct(Symtab* stab);


/* Categories */


/* Pushes symbol onto symbol category stack */
ErrorCode symtab_push_cat(Symtab* stab, SymbolCat cat, Symbol* sym);

/* Pops symbol from top of symbol category stack */
void symtab_pop_cat(Symtab* stab, SymbolCat cat);

/* Returns the last symbol for symbol category (top of stack) */
Symbol* symtab_last_cat(Symtab* stab, SymbolCat cat);


/* Scopes */


/* Sets up a new symbol scope to be used */
ErrorCode symtab_push_scope(Symtab* stab);

/* Removes current symbol scope, now uses the last scope */
void symtab_pop_scope(Symtab* stab);

/* Finds provided token in symbol table, closest scope first
   Returns NULL if not found */
Symbol* symtab_find(Symtab* stab, const char* token);


/* Add to symbol table */


/* Creates symbol with provided information in symbol table
   Stores Symbol* of added symbol at pointer,
   or ec_symtab_dupname if it already exists */
ErrorCode symtab_add(Symtab* stab, Symbol** sym_ptr,
        const char* token, Type type);

/* Adds constant to symbol table
   Stores Symbol* of added constant at pointer */
ErrorCode symtab_add_constant(Symtab* stab, Symbol** sym_ptr,
        const char* token, Type type);

/* Creates a new temporary for the current scope in symbol table */
ErrorCode symtab_add_temporary(Symtab* stab, Symbol** symid_ptr, Type type);

/* Creates a new label for the current scope in symbol table */
ErrorCode symtab_add_label(Symtab* stab, Symbol** symid_ptr);


/* Debug */


void debug_print_symtab(Symtab* stab);

#endif
