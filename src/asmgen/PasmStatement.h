/* Assembly generator, struct PasmStatement */
#ifndef ASMGEN_PASMSTATEMENT_H
#define ASMGEN_PASMSTATEMENT_H

struct PasmStatement {
    AsmIns ins;
    /* 0 = Location
       1 = SymbolId */
    int op_type[MAX_ASM_OP];
    union {
        Location loc;
        SymbolId id;
    } op[MAX_ASM_OP];
    int op_count;
};

/* Initializes values in pseudo-assembly statement */
static void pasmstat_construct(PasmStatement* stat, AsmIns ins, int op_count) {
    ASSERT(stat != NULL, "PasmStatement is null");
    stat->ins = ins;
    stat->op_count = op_count;
}

/* Returns AsmIns for pseudo-assembly statement */
static AsmIns pasmstat_ins(const PasmStatement* stat) {
    ASSERT(stat != NULL, "PasmStatement is null");
    return stat->ins;
}

/* Returns 1 if operand at index i is a location, 0 if not */
static int pasmstat_is_loc(const PasmStatement* stat, int i) {
    ASSERT(stat != NULL, "PasmStatement is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < stat->op_count, "Index out of range");
    return stat->op_type[i] == 0;
}

/* Returns 1 if operand at index i is a SymbolId, 0 if not */
static int pasmstat_is_sym(const PasmStatement* stat, int i) {
    ASSERT(stat != NULL, "PasmStatement is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < stat->op_count, "Index out of range");
    return stat->op_type[i] == 1;
}

/* Interprets operand at index i as Location and returns the Location
   for pseudo-assembly statement */
static Location pasmstat_op_loc(const PasmStatement* stat, int i) {
    ASSERT(stat != NULL, "PasmStatement is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < stat->op_count, "Index out of range");
    ASSERT(pasmstat_is_loc(stat, i), "Incorrect op type");
    return stat->op[i].loc;
}

/* Treats operand at index i as a Location, sets Location at index i for
   pseudo-assembly statement */
static void pasmstat_set_op_loc(PasmStatement* stat, int i, Location loc) {
    ASSERT(stat != NULL, "PasmStatement is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < stat->op_count, "Index out of range");
    stat->op[i].loc = loc;
    stat->op_type[i] = 0;
}

/* Interprets operand at index i as SymbolId and returns the SymbolId
   for pseudo-assembly statement */
static SymbolId pasmstat_op_sym(const PasmStatement* stat, int i) {
    ASSERT(stat != NULL, "PasmStatement is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < stat->op_count, "Index out of range");
    ASSERT(pasmstat_is_sym(stat, i), "Incorrect op type");
    return stat->op[i].id;
}

/* Treats operand at index i as a Symbolid, sets SymbolId at index i for
   pseudo-assembly statement */
static void pasmstat_set_op_sym(PasmStatement* stat, int i, SymbolId id) {
    ASSERT(stat != NULL, "PasmStatement is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < stat->op_count, "Index out of range");
    ASSERT(id >= 0, "Invalid SymbolId");
    stat->op[i].id = id;
    stat->op_type[i] = 1;
}

/* Returns the number of operands in pseudo-assembly statement */
static int pasmstat_op_count(const PasmStatement* stat) {
    ASSERT(stat != NULL, "PasmStatement is null");
    return stat->op_count;
}

#endif
