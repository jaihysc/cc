/* Assembly generator, struct Symbol */
#ifndef ASMGEN_SYMBOL_H
#define ASMGEN_SYMBOL_H

typedef int SymbolId;
typedef struct {
    Type type;
    char name[MAX_ARG_LEN + 1]; /* +1 for null terminator */
    /* Location this variable is assigned to */
    Location loc;

    /* Offset from stack pointer */
    int offset_override; /* Value offset overridden to */
    int override_offset; /* 1 to override offset, 0 to not */
} Symbol;

/* Constructs a symbol at the give memory location */
static void symbol_construct(
        Symbol* sym, const Type* type, const char* name, Location loc) {
    sym->type = *type;
    strcopy(name, sym->name);
    sym->loc = loc;
    sym->override_offset = 0;
}

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

/* Returns 1 if the offset to reach this symbol in stack is overridden,
   0 otherwise */
static int symbol_offset_overridden(const Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->override_offset;
}

/* Returns the value the offset of this symbol in stack is overridden to */
static int symbol_offset_override(const Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    ASSERT(sym->override_offset, "Offset is not overridden");
    return sym->offset_override;
}

/* Overrides the offset of this symbol in stack */
static void symbol_override_offset(Symbol* sym, int offset) {
    ASSERT(sym != NULL, "Symbol is null");
    sym->offset_override = offset;
    sym->override_offset = 1;
}

#endif
