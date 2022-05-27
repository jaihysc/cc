/* Assembly generator, struct Statement */
#ifndef ASMGEN_STATEMENT_H
#define ASMGEN_STATEMENT_H

struct Statement {
    ILIns ins;
    SymbolId arg[MAX_ARGS];
    int argc;
};

/* Returns ILIns for statement */
static ILIns stat_ins(Statement* stat) {
    ASSERT(stat != NULL, "Statement is null");
    return stat->ins;
}

/* Returns arg at index i for statement */
static ILIns stat_arg(Statement* stat, int i) {
    ASSERT(stat != NULL, "Statement is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < stat->argc, "Index out of range");
    return stat->arg[i];
}

/* Returns the number of arguments in statement */
static int stat_argc(Statement* stat) {
    ASSERT(stat != NULL, "Statement is null");
    return stat->argc;
}

/* Returns the number of symbols the instruction of this statement
   uses and puts the used symbols into provided symbol buffer.
   At most MAX_ARGS */
static int stat_use(Statement* stat, SymbolId* out_sym_id) {
    switch (stat->ins) {
        /* 2 use, argument index 1 and 2 */
        case il_add:
        case il_ce:
        case il_cl:
        case il_cle:
        case il_cne:
        case il_div:
        case il_mod:
        case il_mul:
        case il_sub:
            out_sym_id[0] = stat->arg[1];
            out_sym_id[1] = stat->arg[2];
            return 2;

        /* 1 use, argument index 0 */
        case il_ret:
            ASSERT(stat->argc == 1,
                    "ret requires 1 arg (0 arg handling not implemented)");
            out_sym_id[0] = stat->arg[0];
            return 1;

        /* 1 use, argument index 1 */
        case il_mov:
        case il_not:
        case il_jnz:
        case il_jz:
            out_sym_id[0] = stat->arg[1];
            return 1;

        /* No use */
        case il_lab:
        case il_def:
        case il_func:
        case il_jmp:
            return 0;

        default:
            ASSERT(0, "Unimplemented");
            return 0;
    }
}

/* Returns the number of symbols the instruction of this statement
   defines (sets) and puts the defined symbols into provided symbol buffer.
   At most 1 */
static int stat_def(Statement* stat, SymbolId* out_sym_id) {
    switch (stat->ins) {
        case il_add:
        case il_ce:
        case il_cl:
        case il_cle:
        case il_cne:
        case il_div:
        case il_mod:
        case il_mov:
        case il_mul:
        case il_not:
        case il_sub:
            out_sym_id[0] = stat->arg[0];
            return 1;

        case il_def:
        case il_func:
        case il_jmp:
        case il_jnz:
        case il_jz:
        case il_lab:
        case il_ret:
            return 0;

        default:
            ASSERT(0, "Unimplemented");
            return 0;
    }
}

#endif
