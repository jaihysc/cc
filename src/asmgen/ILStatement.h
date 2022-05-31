/* Assembly generator, struct ILStatement */
#ifndef ASMGEN_ILSTATEMENT_H
#define ASMGEN_ILSTATEMENT_H

struct ILStatement {
    ILIns ins;
    SymbolId arg[MAX_ARGS];
    int argc;
};

/* Returns ILIns for IL statement */
static ILIns ilstat_ins(const ILStatement* stat) {
    ASSERT(stat != NULL, "ILStatement is null");
    return stat->ins;
}

/* Returns arg at index i for IL statement */
static SymbolId ilstat_arg(const ILStatement* stat, int i) {
    ASSERT(stat != NULL, "ILStatement is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < stat->argc, "Index out of range");
    return stat->arg[i];
}

/* Returns the number of arguments in IL statement */
static int ilstat_argc(const ILStatement* stat) {
    ASSERT(stat != NULL, "ILStatement is null");
    return stat->argc;
}

#endif
