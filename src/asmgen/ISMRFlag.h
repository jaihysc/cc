/* Assembly generator, struct instruction selection replacement flags */
#ifndef ASMGEN_ISMRFLAG_H
#define ASMGEN_ISMRFLAG_H

/* Can be used to change the behavior of how pseudo-assembly instructions
   are converted to assembly, the bits in the flag have the following
   meanings (each letter corresponds to the bits for a
   description)
   000000000000000000000000000baaaa
   a: Size override - Changes the size of the register which
      is generated, defaults to 0
   b: Dereference - The register is derefenced if 1 - e.g., [rax],
      referenced if 0 - e.g., rax */
typedef uint32_t ISMRFlag; /* InsSelMacroReplace flag */

/* For reading InsSelMacroReplace flags */

/* Returns flag size override */
static int ismr_size_override(ISMRFlag flag) {
    return flag & 0x0F;
}

/* Sets flag size override */
static void ismr_set_size_override(ISMRFlag* flag, int size_override) {
    *flag |= size_override & 0x0F;
}

/* Returns flag dereference */
static int ismr_dereference(ISMRFlag flag) {
    return (flag & 0x10) / 16;
}

#endif
