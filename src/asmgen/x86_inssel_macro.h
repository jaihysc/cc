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

                   [] denodes a group, each group has 1 letter, each letter
                   separated by |. The first group [s|i|l] is required.

                   [s|i|l][u|U][0|1|2|3|4|5|6|7|8|9] ...

                   s = Register/Memory
                   i = Immediate
                   l = Label
                   u = Unsigned
                   U = Signed

                   - A number may follow to constrain the byte size
                   - Multiple constraints are separated by spaces, only one of
                     the constraints needs to match for the case to be valid

                   Example: su1i2 s3s4
                            ~~~        arg1
                               ~~      arg2
                                  ~~   arg3
                                    ~~ arg4
                   To match this constraint: arg1 must be a 1 byte unsigned
                   symbol and arg2 be a 2 byte immediate - or arg1 can be a 3
                   byte symbol and arg2 be a 4 byte symbol
     replaces__: Define pseudo-assembly which this macro replaces to using
                 INSSEL_MACRO_REPLACE

   INSSEL_MACRO_REPLACE0(asmins__)
   INSSEL_MACRO_REPLACE1(asmins__, op1_t__, op1__, op1_flag__)
   INSSEL_MACRO_REPLACE2(asmins__, op1_t__, op1__, op1_flag__,
                                   op2_t__, op2__, op2_flag__)
     Each INSSEL_MACRO_REPLACE replaces to one pseudo-assembly instruction.
     The number at the end corresponds to the number of operands

     asmins__: Name of the AsmIns this macro replaces with, no prefix,
               i.e., mov instead of asmins_mov
     op1_t__: Type of the operands for performing the replacement,
     op2_t__  in order from first operand to last
              0 = New virtual register - operand corresponds to index in
                  IL argument for SymbolId. The new register will have the
                  same type as the symbol at the SymbolId
                  at the SymbolId
              1 = Virtual register - operand is an index into IL argument for
                  SymbolId, the register/memory location of the symbol will
                  be output when generating assembly with spill code generated
                  as necessary
              2 = Physical register location - operand corresponds to an enum
                  Location which will be converted to a register by using the
                  the size of the first symbol with storage in the IL
                  instruction
                  As a result, this can only be used if the IL has
                  at least 1 argument with a byte size
              3 = Physical register - operand corresponds to an enum Register
              Use the provided macros REGISTER_NEW, REGISTER_VIRTUAL,
              REGISTER_PHYSICAL, REGISTER_LOCATION to declare the types

     op1__: Operand
     op2__
     op1_flags__: See declaration of ISMRFlag in fwddecl.h
     op2_flags__  */

#define INSSEL_MACROS                                             \
    INSSEL_MACRO(add,                                             \
        INSSEL_MACRO_CASE(sss,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(add,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssi,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(add,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sis,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(add,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sii,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(add,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(ce,                                              \
        INSSEL_MACRO_CASE(sss,                                    \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_VIRTUAL, 1,,                             \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(sete,                           \
                REGISTER_VIRTUAL, 0, SIZE_OVERRIDE(1)             \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssi,                                    \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_VIRTUAL, 1,,                             \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(sete,                           \
                REGISTER_VIRTUAL, 0, SIZE_OVERRIDE(1)             \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sis,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0,,                                 \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_NEW, 0,,                                 \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(sete,                           \
                REGISTER_VIRTUAL, 0, SIZE_OVERRIDE(1)             \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(cl,                                              \
        INSSEL_MACRO_CASE(ssUsU,                                  \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_VIRTUAL, 1,,                             \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setl,                           \
                REGISTER_VIRTUAL, 0, SIZE_OVERRIDE(1)             \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssUiU,                                  \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_VIRTUAL, 1,,                             \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setl,                           \
                REGISTER_VIRTUAL, 0, SIZE_OVERRIDE(1)             \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(siUsU,                                  \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0,,                                 \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_NEW, 0,,                                 \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setl,                           \
                REGISTER_VIRTUAL, 0, SIZE_OVERRIDE(1)             \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssusu,                                  \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_VIRTUAL, 1,,                             \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setb,                           \
                REGISTER_VIRTUAL, 0, SIZE_OVERRIDE(1)             \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssuiu,                                  \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_VIRTUAL, 1,,                             \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setb,                           \
                REGISTER_VIRTUAL, 0, SIZE_OVERRIDE(1)             \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(siusu,                                  \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0,,                                 \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_NEW, 0,,                                 \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setb,                           \
                REGISTER_VIRTUAL, 0, SIZE_OVERRIDE(1)             \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(cle,                                             \
        INSSEL_MACRO_CASE(ssUsU,                                  \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_VIRTUAL, 1,,                             \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setle,                          \
                REGISTER_VIRTUAL, 0, SIZE_OVERRIDE(1)             \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssUiU,                                  \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_VIRTUAL, 1,,                             \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setle,                          \
                REGISTER_VIRTUAL, 0, SIZE_OVERRIDE(1)             \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(siUsU,                                  \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0,,                                 \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_NEW, 0,,                                 \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setle,                          \
                REGISTER_VIRTUAL, 0, SIZE_OVERRIDE(1)             \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssusu,                                  \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_VIRTUAL, 1,,                             \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setbe,                          \
                REGISTER_VIRTUAL, 0, SIZE_OVERRIDE(1)             \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssuiu,                                  \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_VIRTUAL, 1,,                             \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setbe,                          \
                REGISTER_VIRTUAL, 0, SIZE_OVERRIDE(1)             \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(siusu,                                  \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0,,                                 \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_NEW, 0,,                                 \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setbe,                          \
                REGISTER_VIRTUAL, 0, SIZE_OVERRIDE(1)             \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(cne,                                             \
        INSSEL_MACRO_CASE(sss,                                    \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_VIRTUAL, 1,,                             \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setne,                          \
                REGISTER_VIRTUAL, 0, SIZE_OVERRIDE(1)             \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssi,                                    \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_VIRTUAL, 1,,                             \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setne,                          \
                REGISTER_VIRTUAL, 0, SIZE_OVERRIDE(1)             \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sis,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0,,                                 \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp,                            \
                REGISTER_NEW, 0,,                                 \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setne,                          \
                REGISTER_VIRTUAL, 0, SIZE_OVERRIDE(1)             \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(div,                                             \
        INSSEL_MACRO_CASE(sss,                                    \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, reg_rax,                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, reg_rdx,                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_LOCATION, loc_d,,                        \
                REGISTER_LOCATION, loc_d,                         \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_LOCATION, loc_a,,                        \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(idiv,                           \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_LOCATION, loc_a,                         \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, reg_rdx,                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, reg_rax,                       \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssi,                                    \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, reg_rax,                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, reg_rdx,                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_LOCATION, loc_d,,                        \
                REGISTER_LOCATION, loc_d,                         \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_LOCATION, loc_a,,                        \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0,,                                 \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(idiv,                           \
                REGISTER_NEW, 0,                                  \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_LOCATION, loc_a,                         \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, reg_rdx,                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, reg_rax,                       \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sis,                                    \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, reg_rax,                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, reg_rdx,                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_LOCATION, loc_d,,                        \
                REGISTER_LOCATION, loc_d,                         \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_LOCATION, loc_a,,                        \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(idiv,                           \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_LOCATION, loc_a,                         \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, reg_rdx,                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, reg_rax,                       \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sii,                                    \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, reg_rax,                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, reg_rdx,                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_LOCATION, loc_d,,                        \
                REGISTER_LOCATION, loc_d,                         \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_LOCATION, loc_a,,                        \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0,,                                 \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(idiv,                           \
                REGISTER_NEW, 0,                                  \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_LOCATION, loc_a,                         \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, reg_rdx,                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, reg_rax,                       \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(jmp,                                             \
        INSSEL_MACRO_CASE(l,                                      \
            INSSEL_MACRO_REPLACE1(jmp,                            \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(jnz,                                             \
        INSSEL_MACRO_CASE(ls,                                     \
            INSSEL_MACRO_REPLACE2(test,                           \
                REGISTER_VIRTUAL, 1,,                             \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(jnz,                            \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(li,                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 1,,                                 \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(test,                           \
                REGISTER_NEW, 1,,                                 \
                REGISTER_NEW, 1,                                  \
            )                                                     \
            INSSEL_MACRO_REPLACE1(jnz,                            \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(jz,                                              \
        INSSEL_MACRO_CASE(ls,                                     \
            INSSEL_MACRO_REPLACE2(test,                           \
                REGISTER_VIRTUAL, 1,,                             \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(jz,                             \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(li,                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 1,,                                 \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(test,                           \
                REGISTER_NEW, 1,,                                 \
                REGISTER_NEW, 1,                                  \
            )                                                     \
            INSSEL_MACRO_REPLACE1(jz,                             \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(mad,                                             \
        INSSEL_MACRO_CASE(ss,                                     \
            INSSEL_MACRO_REPLACE2(lea,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(mfi,                                             \
        INSSEL_MACRO_CASE(ssi,                                    \
            /* FIXME I assume the index is 0 for now */           \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 1, DEREFERENCE                  \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(mod,                                             \
        INSSEL_MACRO_CASE(sss,                                    \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, reg_rax,                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, reg_rdx,                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_LOCATION, loc_d,,                        \
                REGISTER_LOCATION, loc_d,                         \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_LOCATION, loc_a,,                        \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(idiv,                           \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_LOCATION, loc_d,                         \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, reg_rdx,                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, reg_rax,                       \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssi,                                    \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, reg_rax,                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, reg_rdx,                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_LOCATION, loc_d,,                        \
                REGISTER_LOCATION, loc_d,                         \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_LOCATION, loc_a,,                        \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0,,                                 \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(idiv,                           \
                REGISTER_NEW, 0,                                  \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_LOCATION, loc_d,                         \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, reg_rdx,                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, reg_rax,                       \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sis,                                    \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, reg_rax,                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, reg_rdx,                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_LOCATION, loc_d,,                        \
                REGISTER_LOCATION, loc_d,                         \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_LOCATION, loc_a,,                        \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(idiv,                           \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_LOCATION, loc_d,                         \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, reg_rdx,                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, reg_rax,                       \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sii,                                    \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, reg_rax,                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(push,                           \
                REGISTER_PHYSICAL, reg_rdx,                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_LOCATION, loc_d,,                        \
                REGISTER_LOCATION, loc_d,                         \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_LOCATION, loc_a,,                        \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0,,                                 \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(idiv,                           \
                REGISTER_NEW, 0,                                  \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_LOCATION, loc_d,                         \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, reg_rdx,                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop,                            \
                REGISTER_PHYSICAL, reg_rax,                       \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(mov,                                             \
        INSSEL_MACRO_CASE(ss,                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(si,                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(mtc,                                             \
        /* Widening */                                            \
        /* Sign extend signed <- signed */                        \
        INSSEL_MACRO_CASE(                                        \
            sU2sU1 sU4sU1 sU8sU1 sU4sU2 sU8sU2 sU8sU4,            \
            INSSEL_MACRO_REPLACE2(movsx,                          \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
        )                                                         \
        /* Zero extend signed <- unsigned */                      \
        /* Zero extend unsigned <- signed/unsigned */             \
        INSSEL_MACRO_CASE(sU2su1 sU4su1 sU8su1 sU4su2 sU8su2      \
                          su2s1 su4s1 su8s1 su4s2 su8s2,          \
            INSSEL_MACRO_REPLACE2(movzx,                          \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
        )                                                         \
            /* mov qword <- dword zero extends */                 \
        INSSEL_MACRO_CASE(sU8su4 su8s4,                           \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
        )                                                         \
        /* Implement narrowing by simply accessing the lower */   \
        /* part of the register */                                \
        INSSEL_MACRO_CASE(s1s2,                                   \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0, SIZE_OVERRIDE(1),            \
                REGISTER_VIRTUAL, 1, SIZE_OVERRIDE(1)             \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(s1s4,                                   \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0, SIZE_OVERRIDE(1),            \
                REGISTER_VIRTUAL, 1, SIZE_OVERRIDE(1)             \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(s1s8,                                   \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0, SIZE_OVERRIDE(1),            \
                REGISTER_VIRTUAL, 1, SIZE_OVERRIDE(1)             \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(s2s4,                                   \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0, SIZE_OVERRIDE(2),            \
                REGISTER_VIRTUAL, 1, SIZE_OVERRIDE(2)             \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(s2s8,                                   \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0, SIZE_OVERRIDE(2),            \
                REGISTER_VIRTUAL, 1, SIZE_OVERRIDE(2)             \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(s4s8,                                   \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0, SIZE_OVERRIDE(4),            \
                REGISTER_VIRTUAL, 1, SIZE_OVERRIDE(4)             \
            )                                                     \
        )                                                         \
        /* No need to actually sign extend if the types */        \
        /* are actually the same size */                          \
        INSSEL_MACRO_CASE(ss,                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
        )                                                         \
        /* Widening / narrowing of constants can be done */       \
        /* by the assembler */                                    \
        INSSEL_MACRO_CASE(si,                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(mti,                                             \
        INSSEL_MACRO_CASE(sis sii,                                \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0,,                                 \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
            /* FIXME I assume the index is 0 for now */           \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0, DEREFERENCE,                     \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(mul,                                             \
        INSSEL_MACRO_CASE(sss,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(imul,                           \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssi,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(imul,                           \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sis,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(imul,                           \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sii,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(imul,                           \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(not,                                             \
        INSSEL_MACRO_CASE(ss,                                     \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(test,                           \
                REGISTER_VIRTUAL, 1,,                             \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setz,                           \
                REGISTER_VIRTUAL, 0, SIZE_OVERRIDE(1)             \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(si,                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0,,                                 \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(test,                           \
                REGISTER_NEW, 0,,                                 \
                REGISTER_NEW, 0,                                  \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setz,                           \
                REGISTER_VIRTUAL, 0, SIZE_OVERRIDE(1)             \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(ret,                                             \
        INSSEL_MACRO_CASE(s,                                      \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_LOCATION, loc_a,,                        \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE0(leave)                          \
            INSSEL_MACRO_REPLACE0(ret)                            \
        )                                                         \
        INSSEL_MACRO_CASE(i,                                      \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_LOCATION, loc_a,,                        \
                REGISTER_VIRTUAL, 0,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE0(leave)                          \
            INSSEL_MACRO_REPLACE0(ret)                            \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(sub,                                             \
        INSSEL_MACRO_CASE(sss,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(sub,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssi,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(sub,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sis,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0,,                                 \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(sub,                            \
                REGISTER_NEW, 0,,                                 \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_NEW, 0,                                  \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sii,                                    \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_NEW, 0,,                                 \
                REGISTER_VIRTUAL, 1,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(sub,                            \
                REGISTER_NEW, 0,,                                 \
                REGISTER_VIRTUAL, 2,                              \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov,                            \
                REGISTER_VIRTUAL, 0,,                             \
                REGISTER_NEW, 0,                                  \
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
#define INSSEL_MACRO_REPLACE0(asmins__)                                      \
    if (!vec_push_backu(&case__->replace)) goto error;                       \
    replace__ = &vec_back(&case__->replace);                                 \
    replace__->ins = asmins_ ## asmins__;                                    \
    replace__->op_count = 0;
#define INSSEL_MACRO_REPLACE1(asmins__, op1_t__, op1__, op1_flag__)          \
    INSSEL_MACRO_REPLACE0(asmins__)                                          \
    replace__->op_type[0] = op1_t__;                                         \
    /* Flags may be empty, if so it is initialized to 0 */                   \
    replace__->flag[0] = 0 op1_flag__;                                       \
    replace__->op_count = 1;                                                 \
    if (op1_t__ <= 1) replace__->op[0].index = op1__;                        \
    if (op1_t__ == 2) replace__->op[0].loc = (Location)op1__;                \
    if (op1_t__ == 3) replace__->op[0].reg = (Register)op1__;
#define INSSEL_MACRO_REPLACE2(asmins__, op1_t__, op1__, op1_flag__,          \
                                        op2_t__, op2__, op2_flag__)          \
    INSSEL_MACRO_REPLACE1(asmins__, op1_t__, op1__, op1_flag__)              \
    replace__->op_type[1] = op2_t__;                                         \
    replace__->flag[1] = 0 op2_flag__;                                       \
    replace__->op_count = 2;                                                 \
    if (op2_t__ <= 1) replace__->op[1].index = op2__;                        \
    if (op2_t__ == 2) replace__->op[1].loc = (Location)op2__;                \
    if (op1_t__ == 3) replace__->op[1].reg = (Register)op2__;

#define REGISTER_NEW 0
#define REGISTER_VIRTUAL 1
#define REGISTER_LOCATION 2
#define REGISTER_PHYSICAL 3
/* The flags expands out as 0 | 0x00 | 0x01
                              ~~~~~~        Flag 1
                                     ~~~~~~ Flag 2 */
#define SIZE_OVERRIDE(byte_size__) | (byte_size__ & 0xF)
#define DEREFERENCE | 0x10

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
#undef REGISTER_LOCATION
#undef REGISTER_PHYSICAL
#undef SIZE_OVERRIDE
#undef DEREFERENCE
#endif
