#include "ilstatement.h"

#include "common.h"

#define INSTRUCTION(name__) #name__,
const char* il2_string[] = {INSTRUCTIONS};
#undef INSTRUCTION

const char* il2_str(ILIns ins) {
    ASSERT(ins >= 0, "Invalid ILIns");
    return il2_string[ins];
}

ILIns il2_from_str(const char* str) {
    return strbinfind(
           str,
           strlength(str),
           il2_string,
           ARRAY_SIZE(il2_string));
}

int il2_isjump(ILIns ins) {
    switch (ins) {
        case il2_jmp:
        case il2_jnz:
        case il2_jz:
            return 1;
        default:
            return 0;
    }
}

int il2_incfg(ILIns ins) {
    switch (ins) {
        case il2_def:
        case il2_func:
        case il2_lab:
            return 0;
        default:
            return 1;
    }
}

ILIns ilstat_ins(const ILStatement* stat) {
    ASSERT(stat != NULL, "ILStatement is null");
    return stat->ins;
}

Symbol* ilstat_arg(const ILStatement* stat, int i) {
    ASSERT(stat != NULL, "ILStatement is null");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < stat->argc, "Index out of range");
    return stat->arg[i];
}

int ilstat_argc(const ILStatement* stat) {
    ASSERT(stat != NULL, "ILStatement is null");
    return stat->argc;
}
