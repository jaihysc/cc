/* Assembly generator, x86 instruction selector macros */
#ifndef ASMGEN_X86_INSSEL_MACRO_H
#define ASMGEN_X86_INSSEL_MACRO_H

/* Defines the macros used for expanding IL instructions to pseudo-assembly

   ILINS_MACRO(ilins__, cases__)
     A instruction selector macro is for an IL instruction, each macro
     contains various cases depending on the arguments for the IL instruction.

     ilins__: Name of the ILIns this macro and cases is for, no prefix,
              i.e., mov instead of ilins_mov
     cases__: Define cases for the macro with INSSEL_MACRO_CASE

   INSSEL_MACRO_CASE(constraint__, replaces__)
     Cases are sorted in increasing cost, i.e., the lowest cost is written
     first and the highest written last.

     constraint__: A string which constrains when this case can be applied,
                   using a string allows for more complex constraints.
                   e.g., Apply only to a constant 1 (for x86 inc)
                   s = Register/Memory
                   i = Immediate
                   l = Label
                   Example: si for arg 1 to be register/memory and arg2 to
                   be immediate
     replaces__: Define pseudo-assembly which this macro replaces to using
                 INSSEL_MACRO_REPLACE

   INSSEL_MACRO_REPLACE1(asmins__, op1_t__, op1__)
   INSSEL_MACRO_REPLACE2(asmins__, op1_t__, op1__, op2_t__, op2__)
     Each INSSEL_MACRO_REPLACE replaces to one pseudo-assembly instruction.
     The number at the end corresponds to the number of operands

     asmins__: Name of the AsmIns this macro replaces with, no prefix,
               i.e., mov instead of asmins_mov
     rop_t__: Type of the operands for performing the replacement,
              in order from first operand to last
              0 = New virtual register - operand corresponds to index in
                  IL argument for SymbolId. The new register will have the
                  same type as the symbol at the SymbolId
                  at the SymbolId
              1 = Virtual register - operand corresponds to index in IL
                  argument for SymbolId
              2 = Physical register - operand corresponds to an enum Location
              Use the provided macros REGISTER_PHYSICAL and REGISTER_VIRTUAL
              to declare the types

     If the operand is a physical register, operand should be of type
     Location. If virtual register, operand should be an integer corresponding
     to the argument (Symbol) index in the IL instructions, e.g., 0 to refer to
     the first symbol in the IL instruction.

     op1__: Operand 1
     op2__: Operand 2 */

#define INSSEL_MACROS                                             \
    INSSEL_MACRO(add,                                             \
        INSSEL_MACRO_CASE(sss,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(add,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssi,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(add,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sis,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(add,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sii,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(add,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(ce,                                              \
        INSSEL_MACRO_CASE(sss,                                    \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_VIRTUAL, 1,                              \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
            INSSEL_MACRO_REPLACE1(sete,                           \
                REGISTER_VIRTUAL, 0                               \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssi,                                    \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_VIRTUAL, 1,                              \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
            INSSEL_MACRO_REPLACE1(sete,                           \
                REGISTER_VIRTUAL, 0                               \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sis,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0,                                  \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_NEW, 0,                                  \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
            INSSEL_MACRO_REPLACE1(sete,                           \
                REGISTER_VIRTUAL, 0                               \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(cl,                                              \
        INSSEL_MACRO_CASE(sss,                                    \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_VIRTUAL, 1,                              \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setl,                           \
                REGISTER_VIRTUAL, 0                               \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssi,                                    \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_VIRTUAL, 1,                              \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setl,                           \
                REGISTER_VIRTUAL, 0                               \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sis,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0,                                  \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_NEW, 0,                                  \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setl,                           \
                REGISTER_VIRTUAL, 0                               \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(cle,                                             \
        INSSEL_MACRO_CASE(sss,                                    \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_VIRTUAL, 1,                              \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setle,                          \
                REGISTER_VIRTUAL, 0                               \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssi,                                    \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_VIRTUAL, 1,                              \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setle,                          \
                REGISTER_VIRTUAL, 0                               \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sis,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0,                                  \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_NEW, 0,                                  \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setle,                          \
                REGISTER_VIRTUAL, 0                               \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(cne,                                             \
        INSSEL_MACRO_CASE(sss,                                    \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_VIRTUAL, 1,                              \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setne,                          \
                REGISTER_VIRTUAL, 0                               \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssi,                                    \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_VIRTUAL, 1,                              \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setne,                          \
                REGISTER_VIRTUAL, 0                               \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sis,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0,                                  \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_NEW, 0,                                  \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setne,                          \
                REGISTER_VIRTUAL, 0                               \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(div,                                             \
        INSSEL_MACRO_CASE(sss,                                    \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, loc_a                          \
            )                                                     \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_PHYSICAL, loc_d,                         \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_PHYSICAL, loc_a,                         \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
            INSSEL_MACRO_REPLACE1(idiv,                           \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_PHYSICAL, loc_a                          \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, loc_a                          \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssi,                                    \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, loc_a                          \
            )                                                     \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_PHYSICAL, loc_d,                         \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_PHYSICAL, loc_a,                         \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0,                                  \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
            INSSEL_MACRO_REPLACE1(idiv,                           \
                REGISTER_NEW, 0                                   \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_PHYSICAL, loc_a                          \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, loc_a                          \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sis,                                    \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, loc_a                          \
            )                                                     \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_PHYSICAL, loc_d,                         \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_PHYSICAL, loc_a,                         \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
            INSSEL_MACRO_REPLACE1(idiv,                           \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_PHYSICAL, loc_a                          \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, loc_a                          \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sii,                                    \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, loc_a                          \
            )                                                     \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_PHYSICAL, loc_d,                         \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_PHYSICAL, loc_a,                         \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0,                                  \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
            INSSEL_MACRO_REPLACE1(idiv,                           \
                REGISTER_NEW, 0                                   \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_PHYSICAL, loc_a                          \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, loc_a                          \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(jmp,                                             \
        INSSEL_MACRO_CASE(l,                                      \
            INSSEL_MACRO_REPLACE1(jmp,                            \
                REGISTER_VIRTUAL, 0                               \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(jnz,                                             \
        INSSEL_MACRO_CASE(ls,                                     \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_VIRTUAL, 1,                              \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
            INSSEL_MACRO_REPLACE1(jnz,                            \
                REGISTER_VIRTUAL, 0                               \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(li,                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0,                                  \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_NEW, 0,                                  \
                REGISTER_NEW, 0                                   \
            )                                                     \
            INSSEL_MACRO_REPLACE1(jnz,                            \
                REGISTER_VIRTUAL, 0                               \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(jz,                                              \
        INSSEL_MACRO_CASE(ls,                                     \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_VIRTUAL, 1,                              \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
            INSSEL_MACRO_REPLACE1(jz,                             \
                REGISTER_VIRTUAL, 0                               \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(li,                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0,                                  \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_NEW, 0,                                  \
                REGISTER_NEW, 0                                   \
            )                                                     \
            INSSEL_MACRO_REPLACE1(jz,                             \
                REGISTER_VIRTUAL, 0                               \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(mod,                                             \
        INSSEL_MACRO_CASE(sss,                                    \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, loc_a                          \
            )                                                     \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_PHYSICAL, loc_d,                         \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_PHYSICAL, loc_a,                         \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
            INSSEL_MACRO_REPLACE1(idiv,                           \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, loc_a                          \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssi,                                    \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, loc_a                          \
            )                                                     \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_PHYSICAL, loc_d,                         \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_PHYSICAL, loc_a,                         \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0,                                  \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
            INSSEL_MACRO_REPLACE1(idiv,                           \
                REGISTER_NEW, 0                                   \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, loc_a                          \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sis,                                    \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, loc_a                          \
            )                                                     \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_PHYSICAL, loc_d,                         \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_PHYSICAL, loc_a,                         \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
            INSSEL_MACRO_REPLACE1(idiv,                           \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, loc_a                          \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sii,                                    \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, loc_a                          \
            )                                                     \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_PHYSICAL, loc_d,                         \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_PHYSICAL, loc_a,                         \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0,                                  \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
            INSSEL_MACRO_REPLACE1(idiv,                           \
                REGISTER_NEW, 0                                   \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, loc_d                          \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, loc_a                          \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(mov,                                             \
        INSSEL_MACRO_CASE(ss,                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(si,                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(mul,                                             \
        INSSEL_MACRO_CASE(sss,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(imul,                           \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssi,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(imul,                           \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sis,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(imul,                           \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sii,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(imul,                           \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(not,                                             \
        INSSEL_MACRO_CASE(ss,                                     \
            INSSEL_MACRO_REPLACE2(test,                           \
                REGISTER_VIRTUAL, 1,                              \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setz,                           \
                REGISTER_VIRTUAL, 0                               \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(si,                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0,                                  \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(test,                           \
                REGISTER_NEW, 0,                                  \
                REGISTER_NEW, 0                                   \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setz,                           \
                REGISTER_VIRTUAL, 0                               \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(ret,                                             \
        INSSEL_MACRO_CASE(s,                                      \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_PHYSICAL, loc_a,                         \
                REGISTER_VIRTUAL, 0                               \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(i,                                      \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_PHYSICAL, loc_a,                         \
                REGISTER_VIRTUAL, 0                               \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(sub,                                             \
        INSSEL_MACRO_CASE(sss,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(sub,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssi,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(sub,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sis,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0,                                  \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(sub,                            \
                REGISTER_NEW, 0,                                  \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_NEW, 0                                   \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sii,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0,                                  \
                REGISTER_VIRTUAL, 1                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(sub,                            \
                REGISTER_NEW, 0,                                  \
                REGISTER_VIRTUAL, 2                               \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,                              \
                REGISTER_NEW, 0                                   \
            )                                                     \
        )                                                         \
    )

/* Creates a new instruction selection macro, expands cases__ to add
   cases to the macro */
#define INSSEL_MACRO(ilins__, cases__)       \
    if (!vec_push_backu(macros)) goto error; \
    macro__ = &vec_back(macros);             \
    vec_construct(&macro__->cases);          \
    macro__->ins = il_ ## ilins__;           \
    cases__

/* Creates a case for a macro, with provided match requirement. Expands macro
   replaces__ to add replacement pseudo-assembly to the macro */
#define INSSEL_MACRO_CASE(constraint__, replaces__)   \
    if (!vec_push_backu(&macro__->cases)) goto error; \
    case__ = &vec_back(&macro__->cases);              \
    vec_construct(&case__->replace);                  \
    case__->constraint = #constraint__;               \
    replaces__

/* Creates a pseudo-assembly instruction for a case of a macro with provided
   assembly instruction, operand type, and provided operands */
#define INSSEL_MACRO_REPLACE1(asmins__, op1_t__, op1__)                      \
    if (!vec_push_backu(&case__->replace)) goto error;                       \
    replace__ = &vec_back(&case__->replace);                                 \
    replace__->ins = asmins_ ## asmins__;                                    \
    /* Initialize the other operand type to no operand */                    \
    replace__->op_type[0] = op1_t__;                                         \
    replace__->op_count = 1;                                                 \
    if (op1_t__ <= 1) replace__->op[0].index = op1__;                        \
    if (op1_t__ == 2) replace__->op[0].loc = op1__;
#define INSSEL_MACRO_REPLACE2(asmins__, op1_t__, op1__, op2_t__, op2__)      \
    INSSEL_MACRO_REPLACE1(asmins__, op1_t__, op1__)                          \
    replace__->op_type[1] = op2_t__;                                         \
    replace__->op_count = 2;                                                 \
    if (op2_t__ <= 1) replace__->op[1].index = op2__;                        \
    if (op2_t__ == 2) replace__->op[1].loc = op2__;

#define REGISTER_NEW 0
#define REGISTER_VIRTUAL 1
#define REGISTER_PHYSICAL 2

/* Initializes macros into provided vec of macros
   Returns 1 if succeeded, zero if out of memory */
static int inssel_macro_construct(vec_InsSelMacro* macros) {
    vec_construct(macros);

    InsSelMacro* macro__;
    InsSelMacroCase* case__;
    InsSelMacroReplace* replace__;
    INSSEL_MACROS
    return 1;
error:
    return 0;
}

/* Destructs provided vec of macros */
static void inssel_macro_destruct(vec_InsSelMacro* macros) {
    for (int i = 0; i < vec_size(macros); ++i) {
        InsSelMacro* macro = &vec_at(macros, i);
        for (int j = 0; j < vec_size(&macro->cases); ++j) {
            InsSelMacroCase* c = &vec_at(&macro->cases, j);
            vec_destruct(&c->replace);
        }
        vec_destruct(&macro->cases);
    }
    vec_destruct(macros);
}

#undef INSSEL_MACROS
#undef INSSEL_MACRO
#undef INSSEL_MACRO_CASE
#undef INSSEL_MACRO_REPLACE1
#undef INSSEL_MACRO_REPLACE2
#undef REGISTER_NEW
#undef REGISTER_VIRTUAL
#undef REGISTER_PHYSICAL
#endif
