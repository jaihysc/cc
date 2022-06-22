/* Assembly generator, struct PasmStatement (Pseudo-assembly statement) */
#ifndef ASMGEN_PASMSTATEMENT_H
#define ASMGEN_PASMSTATEMENT_H

/* Stores the addressing mode of the operands in a PasmStatement */
typedef enum  {
    /* For each hex character:
       stored backwards since it is easier to work with. i.e.,
       0000 0000
       ~~~~ ~~~~
       ^    Operand 1
       Operand 2

       1 = r
       2 = s
       3 = o

       r = Register
       s = SymbolId
       o = OFFSET() - See INSSEL_MACRO_REPLACE */
    pm_r = 0x1,
    pm_s = 0x2,
    pm_o = 0x3,
    pm_rr = 0x11,
    pm_rs = 0x21,
    pm_ro = 0x31,
    pm_sr = 0x12,
    pm_ss = 0x22,
    pm_so = 0x32,
    pm_or = 0x13,
    pm_os = 0x23,
} PasmModeEnum;
/* Perform bit manipulations on fixed size unsigned value to avoid issues with
   signs and size */
typedef uint32_t PasmMode;

struct PasmStatement {
    PasmIns pins;
    PasmMode mode;
    SymbolId op[MAX_ASMINS_REG];
    int op_count;

    /* Live symbols on entry to this statement */
    vec_t(SymbolId) live_in;
    /* Live symbols after this statement */
    vec_t(SymbolId) live_out;
    /* Flags for operands */
    ISMRFlag flag[MAX_ASMINS_REG];
};

/* Initializes values in pseudo-assembly statement */
static void pasmstat_construct(PasmStatement* stat, PasmIns pins) {
    ASSERT(stat != NULL, "PasmStatement is null");
    stat->pins = pins;
    stat->mode = 0;
    stat->op_count = 0;
    vec_construct(&stat->live_in);
    vec_construct(&stat->live_out);
    for (int i = 0; i < MAX_ASMINS_REG; ++i) {
        stat->flag[i] = 0;
    }
}

static void pasmstat_destruct(PasmStatement* stat) {
    vec_destruct(&stat->live_out);
    vec_destruct(&stat->live_in);
}

/* Returns AsmIns for pseudo-assembly statement */
static AsmIns pasmstat_ins(const PasmStatement* stat) {
    ASSERT(stat != NULL, "PasmStatement is null");
    return pasmins_asm(stat->pins);
}

/* Returns PasmIns for pseudo-assembly statement */
static PasmIns pasmstat_pins(const PasmStatement* stat) {
    ASSERT(stat != NULL, "PasmStatement is null");
    return stat->pins;
}

/* Returns the addressing mode for pseudo-assembly statement */
static PasmMode pasmstat_mode(const PasmStatement* stat) {
    ASSERT(stat != NULL, "PasmStatement is null");
    return stat->mode;
}

/* Returns the addressing mode for operand at index i of pseudo-assembly
   statement, 0 if unspecified */
static PasmMode pasmstat_op_mode(const PasmStatement* stat, int i) {
    ASSERT(stat != NULL, "PasmStatement is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < stat->op_count, "Index out of range");
    return (stat->mode >> 4 * i) & 0xF;
}

/* Sets mode for operand at index i to given value */
static void pasmstat_set_op_mode_(PasmStatement* stat, int i, PasmMode val) {
    stat->mode &= ~(0xFu << i * 4); /* Clear existing value */
    stat->mode |= (val << i * 4); /* Or in new value */
}

/* Interprets operand at index i as SymbolId and returns the SymbolId
   for pseudo-assembly statement */
static SymbolId pasmstat_op(const PasmStatement* stat, int i) {
    ASSERT(stat != NULL, "PasmStatement is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < stat->op_count, "Index out of range");
    return stat->op[i];
}

/* Treats operand at index i as a Symbolid, sets SymbolId at index i for
   pseudo-assembly statement */
static void pasmstat_set_op_sym(PasmStatement* stat, int i, SymbolId id) {
    ASSERT(stat != NULL, "PasmStatement is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < stat->op_count, "Index out of range");
    ASSERT(id >= 0, "Invalid SymbolId");
    pasmstat_set_op_mode_(stat, i, pm_s);
    stat->op[i] = id;
}

/* Adds SymbolId to pseudo-assembly statement */
static void pasmstat_add_op_sym(PasmStatement* stat, SymbolId id) {
    ASSERT(stat != NULL, "PasmStatement is null");
    int i = stat->op_count;
    stat->op_count += 1;
    ASSERT(stat->op_count <= MAX_ASMINS_REG, "Too many operands");
    pasmstat_set_op_sym(stat, i, id);
}

/* Treats operand at index i and i + 1 as addressing memory via offset, sets
   SymbolId for base and index */
static void pasmstat_set_op_offset(
        PasmStatement* stat, int i, SymbolId base, SymbolId index) {
    ASSERT(stat != NULL, "PasmStatement is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i + 1 < stat->op_count, "Index out of range");
    ASSERT(base >= 0, "Invalid SymbolId");
    ASSERT(index >= 0, "Invalid SymbolId");

    pasmstat_set_op_mode_(stat, i, pm_o);
    stat->op[i] = base;
    stat->op[i + 1] = index;
}

/* Adds 2 SymbolId base and index for OFFSET addressing mode */
static void pasmstat_add_op_offset(
        PasmStatement* stat, SymbolId base, SymbolId index) {
    ASSERT(stat != NULL, "PasmStatement is null");
    int i = stat->op_count;
    stat->op_count += 2;
    ASSERT(stat->op_count <= MAX_ASMINS_REG, "Too many operands");
    pasmstat_set_op_offset(stat, i, base, index);
}

/* Returns flag for operand index i for pseudo-assembly statement */
static ISMRFlag pasmstat_flag(const PasmStatement* stat, int i) {
    ASSERT(stat != NULL, "PasmStatement is null");
    ASSERT(i >= 0, "Index out of range");
    return stat->flag[i];
}

/* Sets flag for operand index i for pseudo-assembly statement */
static void pasmstat_set_flag(PasmStatement* stat, int i, ISMRFlag flag) {
    ASSERT(stat != NULL, "PasmStatement is null");
    ASSERT(i >= 0, "Index out of range");
    stat->flag[i] = flag;
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

/* Returns 1 if the operand at index i addresses memory, 0 if not */
static int pasmstat_address_memory(const PasmStatement* stat, int i) {
    switch (pasmstat_op_mode(stat, i)) {
        case pm_o:
            return 1;
        default:
            break;
    }
    return ismr_dereference(pasmstat_flag(stat, i));
}

/* Adds SymbolId to provided array if is not already in array */
static void pasmstat_add_(SymbolId* out_sym_id, int* used, SymbolId id) {
    for (int i = 0; i < *used; ++i) {
        if (out_sym_id[i] == id) {
            return;
        }
    }
    out_sym_id[*used] = id;
    ++(*used);
}

/* Sets out_sym_id with Symbols which are used by the provided pseudo-assembly
   statement
   Returns the number of symbols
   At most MAX_ASMINS_REG */
static int pasmstat_use(const PasmStatement* stat, SymbolId* out_sym_id) {
    /* Symbols used to address memory are always use'd, e.g., [rax],
       where rax is the register of some symbol */

    int used = 0;
    for (int i = 0; i < pasmstat_op_count(stat); ++i) {
        if (pasmstat_address_memory(stat, i)) {
            pasmstat_add_(out_sym_id, &used, pasmstat_op(stat, i));
        }
    }

    /* Access symbols, e.g., rax, [rbp-5], where rax, [rbp-5] is the
       register/location of some symbol */
    switch (pasmstat_ins(stat)) {
        /* Uses 1, def 1 */
        case asmins_lea:
        case asmins_mov:
        case asmins_movsx:
        case asmins_movzx:
            pasmstat_add_(out_sym_id, &used, pasmstat_op(stat, 1));
            break;

        /* Uses 1 */
        case asmins_idiv:
        case asmins_push:
            pasmstat_add_(out_sym_id, &used, pasmstat_op(stat, 0));
            break;

        /* Uses 2, def 1 */
        case asmins_add:
        case asmins_imul:
        case asmins_sub:
        /* Uses 2 */
        case asmins_cmp:
        case asmins_test:
            for (int i = 0; i < 2; ++i) {
                pasmstat_add_(out_sym_id, &used, pasmstat_op(stat, i));
            }
            break;
        /* xor uses nothing if both operands are the same, i.e., xor reg, reg.
           xor mem, mem is not possible, so do not need to handle that case */
        case asmins_xor:
            if (!pasmstat_address_memory(stat, 0) &&
                !pasmstat_address_memory(stat, 1) &&
                pasmstat_op(stat, 0) == pasmstat_op(stat, 1)) {
                return 0;
            }
            for (int i = 0; i < 2; ++i) {
                pasmstat_add_(out_sym_id, &used, pasmstat_op(stat, i));
            }
            break;

        /* Uses none */
        case asmins_call:
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

    ASSERT(used <= MAX_ASMINS_REG,
            "Exceeded maximum number of used symbols");
    return used;
}

/* Sets out_sym_id with Symbols which are defined by the provided
   pseudo-assembly statement
   Returns the number of symbols
   At most 1 */
static int pasmstat_def(const PasmStatement* stat, SymbolId* out_sym_id) {
    /* No def if addressing memory, e.g.,
       mov DWORD [%a], 5
       does not def the symbol a */
    switch (pasmstat_ins(stat)) {
        /* Uses 1, def 1 */
        case asmins_add:
        case asmins_imul:
        case asmins_mov:
        case asmins_movsx:
        case asmins_movzx:
        case asmins_sub:
        case asmins_xor:
        /* Def 1 */
        case asmins_lea:
        case asmins_pop:
        case asmins_setb:
        case asmins_setbe:
        case asmins_sete:
        case asmins_setl:
        case asmins_setle:
        case asmins_setne:
        case asmins_setz:
            if (!pasmstat_address_memory(stat, 0)) {
                out_sym_id[0] = pasmstat_op(stat, 0);
                return 1;
            }
            return 0;

        /* Def none */
        /* FIXME Liveness for call is wrong, the registers could change after
           the call */
        case asmins_call:
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
