/* Assembly generator, struct PasmStatement */
#ifndef ASMGEN_PASMSTATEMENT_H
#define ASMGEN_PASMSTATEMENT_H

struct PasmStatement {
    AsmIns ins;
    /* 0 = Register
       1 = SymbolId */
    int op_type[MAX_ASM_OP];
    union {
        Register reg;
        SymbolId id;
    } op[MAX_ASM_OP];
    int op_count;

    /* Live symbols on entry to this statement */
    vec_t(SymbolId) live_in;
    /* Live symbols after this statement */
    vec_t(SymbolId) live_out;
    /* Overrides byte size of operand, negative number for no override */
    int size_override[MAX_ASM_OP];
};

/* Initializes values in pseudo-assembly statement */
static void pasmstat_construct(PasmStatement* stat, AsmIns ins, int op_count) {
    ASSERT(stat != NULL, "PasmStatement is null");
    stat->ins = ins;
    stat->op_count = op_count;
    vec_construct(&stat->live_in);
    vec_construct(&stat->live_out);
    for (int i = 0; i < MAX_ASM_OP; ++i) {
        stat->size_override[i] = -1;
    }
}

static void pasmstat_destruct(PasmStatement* stat) {
    vec_destruct(&stat->live_out);
    vec_destruct(&stat->live_in);
}

/* Returns AsmIns for pseudo-assembly statement */
static AsmIns pasmstat_ins(const PasmStatement* stat) {
    ASSERT(stat != NULL, "PasmStatement is null");
    return stat->ins;
}

/* Returns 1 if operand at index i is a Register, 0 if not */
static int pasmstat_is_reg(const PasmStatement* stat, int i) {
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

/* Interprets operand at index i as Register and returns the Register
   for pseudo-assembly statement */
static Register pasmstat_op_reg(const PasmStatement* stat, int i) {
    ASSERT(stat != NULL, "PasmStatement is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < stat->op_count, "Index out of range");
    ASSERT(pasmstat_is_reg(stat, i), "Incorrect op type");
    return stat->op[i].reg;
}

/* Treats operand at index i as a Register, sets Register at index i for
   pseudo-assembly statement */
static void pasmstat_set_op_reg(PasmStatement* stat, int i, Register reg) {
    ASSERT(stat != NULL, "PasmStatement is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < stat->op_count, "Index out of range");
    stat->op[i].reg = reg;
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

/* Returns size override for operand index i for pseudo-assembly statement */
static int pasmstat_size_override(PasmStatement* stat, int i) {
    ASSERT(stat != NULL, "PasmStatement is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < stat->op_count, "Index out of range");
    return stat->size_override[i];
}

/* Sets size override for operand index i for pseudo-assembly statement */
static void pasmstat_set_size_override(PasmStatement* stat, int i, int size) {
    ASSERT(stat != NULL, "PasmStatement is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < stat->op_count, "Index out of range");
    stat->size_override[i] = size;
}

/* Returns the number of operands in pseudo-assembly statement */
static int pasmstat_op_count(const PasmStatement* stat) {
    ASSERT(stat != NULL, "PasmStatement is null");
    return stat->op_count;
}

/* Returns number of live symbols before this pseudo-assembly statement */
static int pasmstat_live_in_count(PasmStatement* stat) {
    ASSERT(stat != NULL, "PasmStatement is null");
    return vec_size(&stat->live_in);
}

/* Returns SymbolId of live symbol before this pseudo-assembly statement
   at index */
static SymbolId pasmstat_live_in(PasmStatement* stat, int i) {
    ASSERT(stat != NULL, "PasmStatement is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < pasmstat_live_in_count(stat), "Index out of range");
    return vec_at(&stat->live_in, i);
}

/* Sets the symbols which are live before this pseudo-assembly statement
   by copying count from provided pointer
   Returns 1 if successful, 0 if not */
static int pasmstat_set_live_in(
        PasmStatement* stat, const SymbolId* syms, int count) {
    ASSERT(stat != NULL, "PasmStatement is null");
    for (int i = 0; i < count; ++i) {
        ASSERT(syms != NULL, "Provided syms is null");
        if (!vec_push_back(&stat->live_in, syms[i])) return 0;
    }
    return 1;
}

/* Returns number of live symbols after this pseudo-assembly statement */
static int pasmstat_live_out_count(PasmStatement* stat) {
    ASSERT(stat != NULL, "PasmStatement is null");
    return vec_size(&stat->live_out);
}

/* Returns SymbolId of live symbol after this pseudo-assembly statement
   at index */
static SymbolId pasmstat_live_out(PasmStatement* stat, int i) {
    ASSERT(stat != NULL, "PasmStatement is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < pasmstat_live_out_count(stat), "Index out of range");
    return vec_at(&stat->live_out, i);
}

/* Sets the symbols which are live after this pseudo-assembly statement
   by copying count from provided pointer
   Returns 1 if successful, 0 if not */
static int pasmstat_set_live_out(
        PasmStatement* stat, const SymbolId* syms, int count) {
    ASSERT(stat != NULL, "PasmStatement is null");
    for (int i = 0; i < count; ++i) {
        ASSERT(syms != NULL, "Provided syms is null");
        if (!vec_push_back(&stat->live_out, syms[i])) return 0;
    }
    return 1;
}

/* Sets out_sym_id with Symbols which are used by the provided pseudo-assembly
   statement
   Returns the number of symbols
   At most MAX_ASM_OP */
static int pasmstat_use(const PasmStatement* stat, SymbolId* out_sym_id) {
    switch (stat->ins) {
        /* Uses 1, def 1 */
        case asmins_add:
        case asmins_imul:
        case asmins_mov:
        case asmins_movsx:
        case asmins_movzx:
        case asmins_sub:
        case asmins_xor:
            if (pasmstat_is_sym(stat, 1)) {
                out_sym_id[0] = pasmstat_op_sym(stat, 1);
                return 1;
            }
            return 0;

        /* Uses 1 */
        case asmins_idiv:
        case asmins_push:
            if (pasmstat_is_sym(stat, 0)) {
                out_sym_id[0] = pasmstat_op_sym(stat, 0);
                return 1;
            }
            return 0;

        /* Uses 2 */
        case asmins_cmp:
        case asmins_test:
            {
                int used = 0;
                for (int i = 0; i < 2; ++i) {
                    if (pasmstat_is_sym(stat, i)) {
                        out_sym_id[used] = pasmstat_op_sym(stat, i);
                        ++used;
                    }
                }
                return used;
            }

        /* Uses none */
        case asmins_jmp:
        case asmins_jnz:
        case asmins_jz:
        case asmins_leave:
        case asmins_pop:
        case asmins_ret:
        case asmins_setb:
        case asmins_setbe:
        case asmins_sete:
        case asmins_setl:
        case asmins_setle:
        case asmins_setne:
        case asmins_setz:
            return 0;

        default:
            ASSERT(0, "Unimplemented");
            return 0;
    }
}

/* Sets out_sym_id with Symbols which are defined by the provided
   pseudo-assembly statement
   Returns the number of symbols
   At most 1 */
static int pasmstat_def(const PasmStatement* stat, SymbolId* out_sym_id) {
    switch (stat->ins) {
        /* Uses 1, def 1 */
        case asmins_add:
        case asmins_imul:
        case asmins_mov:
        case asmins_movsx:
        case asmins_movzx:
        case asmins_sub:
        case asmins_xor:
        /* Def 1 */
        case asmins_pop:
        case asmins_setb:
        case asmins_setbe:
        case asmins_sete:
        case asmins_setl:
        case asmins_setle:
        case asmins_setne:
        case asmins_setz:
            if (pasmstat_is_sym(stat, 0)) {
                out_sym_id[0] = pasmstat_op_sym(stat, 0);
                return 1;
            }
            return 0;

        /* Def none */
        case asmins_cmp:
        case asmins_idiv:
        case asmins_jmp:
        case asmins_jnz:
        case asmins_jz:
        case asmins_leave:
        case asmins_push:
        case asmins_ret:
        case asmins_test:
            return 0;

        default:
            ASSERT(0, "Unimplemented");
            return 0;
    }
}

#endif
