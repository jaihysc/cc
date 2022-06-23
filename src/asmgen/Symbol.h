/* Assembly generator, struct Symbol */
#ifndef ASMGEN_SYMBOL_H
#define ASMGEN_SYMBOL_H

typedef int SymbolId;
typedef struct {
    Type type;
    char name[MAX_ARG_LEN + 1]; /* +1 for null terminator */
    /* Location this variable is assigned to */
    Location loc;
} Symbol;

/* Returns 1 if symbol name is considered a constant, 0 otherwise */
static int name_isconstant(const char* name) {
    /* '-' for negative numbers */
    return (('0' <= name[0] && name[0] <= '9') || name[0] == '-');
}

/* Returns type for given symbol */
static Type symbol_type(const Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->type;
}

/* Returns name for given symbol */
static const char* symbol_name(const Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->name;
}

/* Returns where symbol is stored */
static Location symbol_location(const Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->loc;
}

/* Sets symbol location */
static void symbol_set_location(Symbol* sym, Location loc) {
    ASSERT(sym != NULL, "Symbol is null");
    sym->loc = loc;
}

/* Returns register where symbol is */
static Register symbol_register(const Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return reg_get(sym->loc, type_bytes(sym->type));
}

/* Returns 1 if symbol is located in a register, 0 otherwise */
static int symbol_in_register(const Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->loc >= 0;
}

/* Returns 1 if symbol is located on the stack, 0 otherwise */
static int symbol_on_stack(const Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->loc == loc_stack;
}

/* Returns 1 if symbol is a constant */
static int symbol_is_constant(const Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->loc == loc_constant;
}

/* Turns this symbol into a special symbol for representing constants */
static void symbol_make_constant(Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    sym->loc = loc_constant;
}

/* Returns 1 if symbol is a label */
static int symbol_is_label(const Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return type_equal(sym->type, type_label);
}

/* Returns 1 if symbol is a variable which requires storage */
static int symbol_is_var(const Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return !symbol_is_label(sym) &&
        !symbol_is_constant(sym) &&
        !type_is_function(&sym->type);
}

/* Returns bytes for symbol */
static int symbol_bytes(const Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return type_bytes(sym->type);
}

#endif
