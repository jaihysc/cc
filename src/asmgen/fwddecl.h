/* Assembly generator, forward declarations */
#ifndef ASMGEN_FWDDECL_H
#define ASMGEN_FWDDECL_H

typedef struct Parser Parser;
typedef struct ILStatement ILStatement;
typedef struct PasmStatement PasmStatement;

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

#endif
