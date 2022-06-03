/* Assembly generator, constants */
#ifndef ASMGEN_CONSTANT_H
#define ASMGEN_CONSTANT_H

#define MAX_INSTRUCTION_LEN 256 /* Includes null terminator */
#define MAX_ARG_LEN 2048   /* Includes null terminator */
#define MAX_ARGS 256 /* Maximum arguments for il instruction */
#define MAX_BLOCK_LINK 2 /* Maximum links out of block to to other blocks */
/* Spaces from start of line to start of assembly instruction */
#define ASM_INS_INDENT 8
/* Spaces from start of line to start of assembly operand */
#define ASM_OP_INDENT 24

#endif
