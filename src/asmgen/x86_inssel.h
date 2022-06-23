/* Assembly generator, x86 instruction selectors 1 and 2 */
#ifndef ASMGEN_X86_INSSEL_H
#define ASMGEN_X86_INSSEL_H

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

                   [s|a|i|l][u|U][0|1|2|3|4|5|6|7|8|9] ...

                   s = Register/Memory
                   a = Array
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
   INSSEL_MACRO_REPLACE1(asmins__, op1__, op1_flag__)
   INSSEL_MACRO_REPLACE2(asmins__, op1__, op1_flag__,
                                   op2__, op2_flag__)
     Each INSSEL_MACRO_REPLACE replaces to one pseudo-assembly instruction.
     The number at the end corresponds to the number of operands

     asmins__: Name of the AsmIns this macro replaces with, no prefix,
               i.e., mov instead of asmins_mov
     op1__: Describes the operand, can be a register address mode (see below)
     op2__  to address registers, e.g., rax or a memory address mode
            (see below) to address memory, e.g., [rax+rbx]

            Register address mode:
            NEW(i__) Mode: 0
              New virtual register - operand corresponds to index in IL
              argument for SymbolId. The new register will have the same type
              as the symbol at the SymbolId.
            VIRTUAL(i__) Mode: 1
              Virtual register - operand is an index into IL argument for
              SymbolId, the register/memory location of the symbol will be
              output when generating assembly with spill code generated as
              necessary.
            LOCATION(loc__) Mode: 2
              Physical register location - operand corresponds to an enum.
              Location which will be converted to a register by using the the
              size of the first symbol with storage in the IL instruction.
              As a result, this can only be used if the IL has
              at least 1 argument with a byte size.
            PHYSICAL(reg__) Mode: 3
              Physical register - operand corresponds to an enum Register.

            Memory address mode:
            OFFSET(base__, offset__) Mode: 4
              Read at offset in memory - base__ is an index into IL argument
              for SymbolId, the symbol must be in memory.
              offset__ is an index into IL argument for SymbolId, the symbol's
              value will be used as an offset.
              The formed assembly operand is [base__ + offset__], for example
              if base__ is at [rbp-30] and offset__ is rax: the formed operand
              is [rbp+rax-30].

            The addressing mode are stored as follows:
            0000 0000 0000 0000 0000 0000 0000 0000
            ~~~~                ~~~~~~~~~ ~~~~~~~~~
            Mode                Param 2   Param 1

     op1_flags__: Macros SIZE_OVERRIDE() and DEREFERENCE and provided
     op2_flags__ */

#define INSSEL_MACROS                                             \
    INSSEL_MACRO(add,                                             \
        INSSEL_MACRO_CASE(sss,                                    \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(add_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(2),                                       \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssi,                                    \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(add_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(2),                                       \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sis,                                    \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(add_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(1),                                       \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sii,                                    \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(add_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(1),                                       \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(ce,                                              \
        INSSEL_MACRO_CASE(sss,                                    \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(0),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp_ss,                         \
                VIRTUAL(1),,                                      \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(sete_s,                         \
                VIRTUAL(0), SIZE_OVERRIDE(1)                      \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssi,                                    \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(0),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp_ss,                         \
                VIRTUAL(1),,                                      \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(sete_s,                         \
                VIRTUAL(0), SIZE_OVERRIDE(1)                      \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sis,                                    \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                NEW(0),,                                          \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(0),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp_ss,                         \
                NEW(0),,                                          \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(sete_s,                         \
                VIRTUAL(0), SIZE_OVERRIDE(1)                      \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(cl,                                              \
        INSSEL_MACRO_CASE(ssUsU,                                  \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(0),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp_ss,                         \
                VIRTUAL(1),,                                      \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setl_s,                         \
                VIRTUAL(0), SIZE_OVERRIDE(1)                      \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssUiU,                                  \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(0),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp_ss,                         \
                VIRTUAL(1),,                                      \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setl_s,                         \
                VIRTUAL(0), SIZE_OVERRIDE(1)                      \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(siUsU,                                  \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                NEW(0),,                                          \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(0),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp_ss,                         \
                NEW(0),,                                          \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setl_s,                         \
                VIRTUAL(0), SIZE_OVERRIDE(1)                      \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssusu,                                  \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(0),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp_ss,                         \
                VIRTUAL(1),,                                      \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setb_s,                         \
                VIRTUAL(0), SIZE_OVERRIDE(1)                      \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssuiu,                                  \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(0),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp_ss,                         \
                VIRTUAL(1),,                                      \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setb_s,                         \
                VIRTUAL(0), SIZE_OVERRIDE(1)                      \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(siusu,                                  \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                NEW(0),,                                          \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(0),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp_ss,                         \
                NEW(0),,                                          \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setb_s,                         \
                VIRTUAL(0), SIZE_OVERRIDE(1)                      \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(cle,                                             \
        INSSEL_MACRO_CASE(ssUsU,                                  \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(0),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp_ss,                         \
                VIRTUAL(1),,                                      \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setle_s,                        \
                VIRTUAL(0), SIZE_OVERRIDE(1)                      \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssUiU,                                  \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(0),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp_ss,                         \
                VIRTUAL(1),,                                      \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setle_s,                        \
                VIRTUAL(0), SIZE_OVERRIDE(1)                      \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(siUsU,                                  \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                NEW(0),,                                          \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(0),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp_ss,                         \
                NEW(0),,                                          \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setle_s,                        \
                VIRTUAL(0), SIZE_OVERRIDE(1)                      \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssusu,                                  \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(0),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp_ss,                         \
                VIRTUAL(1),,                                      \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setbe_s,                        \
                VIRTUAL(0), SIZE_OVERRIDE(1)                      \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssuiu,                                  \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(0),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp_ss,                         \
                VIRTUAL(1),,                                      \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setbe_s,                        \
                VIRTUAL(0), SIZE_OVERRIDE(1)                      \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(siusu,                                  \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                NEW(0),,                                          \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(0),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp_ss,                         \
                NEW(0),,                                          \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setbe_s,                        \
                VIRTUAL(0), SIZE_OVERRIDE(1)                      \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(cne,                                             \
        INSSEL_MACRO_CASE(sss,                                    \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(0),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp_ss,                         \
                VIRTUAL(1),,                                      \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setne_s,                        \
                VIRTUAL(0), SIZE_OVERRIDE(1)                      \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssi,                                    \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(0),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp_ss,                         \
                VIRTUAL(1),,                                      \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setne_s,                        \
                VIRTUAL(0), SIZE_OVERRIDE(1)                      \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sis,                                    \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                NEW(0),,                                          \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(0),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(cmp_ss,                         \
                NEW(0),,                                          \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setne_s,                        \
                VIRTUAL(0), SIZE_OVERRIDE(1)                      \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(div,                                             \
        INSSEL_MACRO_CASE(sss,                                    \
            INSSEL_MACRO_REPLACE1(push_s,                         \
                PHYSICAL(reg_rax),                                \
            )                                                     \
            INSSEL_MACRO_REPLACE1(push_s,                         \
                PHYSICAL(reg_rdx),                                \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                LOCATION(loc_d),,                                 \
                LOCATION(loc_d),                                  \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                LOCATION(loc_a),,                                 \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(idiv_s,                         \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0),,                                      \
                LOCATION(loc_a),                                  \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop_s,                          \
                PHYSICAL(reg_rdx),                                \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop_s,                          \
                PHYSICAL(reg_rax),                                \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssi,                                    \
            INSSEL_MACRO_REPLACE1(push_s,                         \
                PHYSICAL(reg_rax),                                \
            )                                                     \
            INSSEL_MACRO_REPLACE1(push_s,                         \
                PHYSICAL(reg_rdx),                                \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                LOCATION(loc_d),,                                 \
                LOCATION(loc_d),                                  \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                LOCATION(loc_a),,                                 \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                NEW(0),,                                          \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(idiv_s,                         \
                NEW(0),                                           \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0),,                                      \
                LOCATION(loc_a),                                  \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop_s,                          \
                PHYSICAL(reg_rdx),                                \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop_s,                          \
                PHYSICAL(reg_rax),                                \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sis,                                    \
            INSSEL_MACRO_REPLACE1(push_s,                         \
                PHYSICAL(reg_rax),                                \
            )                                                     \
            INSSEL_MACRO_REPLACE1(push_s,                         \
                PHYSICAL(reg_rdx),                                \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                LOCATION(loc_d),,                                 \
                LOCATION(loc_d),                                  \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                LOCATION(loc_a),,                                 \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(idiv_s,                         \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0),,                                      \
                LOCATION(loc_a),                                  \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop_s,                          \
                PHYSICAL(reg_rdx),                                \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop_s,                          \
                PHYSICAL(reg_rax),                                \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sii,                                    \
            INSSEL_MACRO_REPLACE1(push_s,                         \
                PHYSICAL(reg_rax),                                \
            )                                                     \
            INSSEL_MACRO_REPLACE1(push_s,                         \
                PHYSICAL(reg_rdx),                                \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                LOCATION(loc_d),,                                 \
                LOCATION(loc_d),                                  \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                LOCATION(loc_a),,                                 \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                NEW(0),,                                          \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(idiv_s,                         \
                NEW(0),                                           \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0),,                                      \
                LOCATION(loc_a),                                  \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop_s,                          \
                PHYSICAL(reg_rdx),                                \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop_s,                          \
                PHYSICAL(reg_rax),                                \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(jmp,                                             \
        INSSEL_MACRO_CASE(l,                                      \
            INSSEL_MACRO_REPLACE1(jmp_,                           \
                VIRTUAL(0),                                       \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(jnz,                                             \
        INSSEL_MACRO_CASE(ls,                                     \
            INSSEL_MACRO_REPLACE2(test_ss,                        \
                VIRTUAL(1),,                                      \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(jnz_,                           \
                VIRTUAL(0),                                       \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(li,                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                NEW(1),,                                          \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(test_ss,                        \
                NEW(1),,                                          \
                NEW(1),                                           \
            )                                                     \
            INSSEL_MACRO_REPLACE1(jnz_,                           \
                VIRTUAL(0),                                       \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(jz,                                              \
        INSSEL_MACRO_CASE(ls,                                     \
            INSSEL_MACRO_REPLACE2(test_ss,                        \
                VIRTUAL(1),,                                      \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(jz_,                            \
                VIRTUAL(0),                                       \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(li,                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                NEW(1),,                                          \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(test_ss,                        \
                NEW(1),,                                          \
                NEW(1),                                           \
            )                                                     \
            INSSEL_MACRO_REPLACE1(jz_,                            \
                VIRTUAL(0),                                       \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(mad,                                             \
        INSSEL_MACRO_CASE(ss,                                     \
            INSSEL_MACRO_REPLACE2(lea_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(1),                                       \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(mfi,                                             \
        INSSEL_MACRO_CASE(sas sai,                                \
            INSSEL_MACRO_REPLACE2(mov_so,                         \
                VIRTUAL(0),,                                      \
                OFFSET(1, 2),                                     \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sss ssi,                                \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                NEW(1),,                                          \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(add_ss,                         \
                NEW(1),,                                          \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0),,                                      \
                NEW(1), DEREFERENCE                               \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(mod,                                             \
        INSSEL_MACRO_CASE(sss,                                    \
            INSSEL_MACRO_REPLACE1(push_s,                         \
                PHYSICAL(reg_rax),                                \
            )                                                     \
            INSSEL_MACRO_REPLACE1(push_s,                         \
                PHYSICAL(reg_rdx),                                \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                LOCATION(loc_d),,                                 \
                LOCATION(loc_d),                                  \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                LOCATION(loc_a),,                                 \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(idiv_s,                         \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0),,                                      \
                LOCATION(loc_d),                                  \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop_s,                          \
                PHYSICAL(reg_rdx),                                \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop_s,                          \
                PHYSICAL(reg_rax),                                \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssi,                                    \
            INSSEL_MACRO_REPLACE1(push_s,                         \
                PHYSICAL(reg_rax),                                \
            )                                                     \
            INSSEL_MACRO_REPLACE1(push_s,                         \
                PHYSICAL(reg_rdx),                                \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                LOCATION(loc_d),,                                 \
                LOCATION(loc_d),                                  \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                LOCATION(loc_a),,                                 \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                NEW(0),,                                          \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(idiv_s,                         \
                NEW(0),                                           \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0),,                                      \
                LOCATION(loc_d),                                  \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop_s,                          \
                PHYSICAL(reg_rdx),                                \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop_s,                          \
                PHYSICAL(reg_rax),                                \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sis,                                    \
            INSSEL_MACRO_REPLACE1(push_s,                         \
                PHYSICAL(reg_rax),                                \
            )                                                     \
            INSSEL_MACRO_REPLACE1(push_s,                         \
                PHYSICAL(reg_rdx),                                \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                LOCATION(loc_d),,                                 \
                LOCATION(loc_d),                                  \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                LOCATION(loc_a),,                                 \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(idiv_s,                         \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0),,                                      \
                LOCATION(loc_d),                                  \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop_s,                          \
                PHYSICAL(reg_rdx),                                \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop_s,                          \
                PHYSICAL(reg_rax),                                \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sii,                                    \
            INSSEL_MACRO_REPLACE1(push_s,                         \
                PHYSICAL(reg_rax),                                \
            )                                                     \
            INSSEL_MACRO_REPLACE1(push_s,                         \
                PHYSICAL(reg_rdx),                                \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                LOCATION(loc_d),,                                 \
                LOCATION(loc_d),                                  \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                LOCATION(loc_a),,                                 \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                NEW(0),,                                          \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(idiv_s,                         \
                NEW(0),                                           \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0),,                                      \
                LOCATION(loc_d),                                  \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop_s,                          \
                PHYSICAL(reg_rdx),                                \
            )                                                     \
            INSSEL_MACRO_REPLACE1(pop_s,                          \
                PHYSICAL(reg_rax),                                \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(mov,                                             \
        INSSEL_MACRO_CASE(ss,                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(1),                                       \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(si,                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(1),                                       \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(mtc,                                             \
        /* Widening */                                            \
        /* Sign extend signed <- signed */                        \
        INSSEL_MACRO_CASE(                                        \
            sU2sU1 sU4sU1 sU8sU1 sU4sU2 sU8sU2 sU8sU4,            \
            INSSEL_MACRO_REPLACE2(movsx_ss,                       \
                VIRTUAL(0),,                                      \
                VIRTUAL(1),                                       \
            )                                                     \
        )                                                         \
        /* Zero extend signed <- unsigned */                      \
        /* Zero extend unsigned <- signed/unsigned */             \
        INSSEL_MACRO_CASE(sU2su1 sU4su1 sU8su1 sU4su2 sU8su2      \
                          su2s1 su4s1 su8s1 su4s2 su8s2,          \
            INSSEL_MACRO_REPLACE2(movzx_ss,                       \
                VIRTUAL(0),,                                      \
                VIRTUAL(1),                                       \
            )                                                     \
        )                                                         \
            /* mov qword <- dword zero extends */                 \
        INSSEL_MACRO_CASE(sU8su4 su8s4,                           \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(1),                                       \
            )                                                     \
        )                                                         \
        /* Implement narrowing by simply accessing the lower */   \
        /* part of the register */                                \
        INSSEL_MACRO_CASE(s1s2,                                   \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0), SIZE_OVERRIDE(1),                     \
                VIRTUAL(1), SIZE_OVERRIDE(1)                      \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(s1s4,                                   \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0), SIZE_OVERRIDE(1),                     \
                VIRTUAL(1), SIZE_OVERRIDE(1)                      \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(s1s8,                                   \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0), SIZE_OVERRIDE(1),                     \
                VIRTUAL(1), SIZE_OVERRIDE(1)                      \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(s2s4,                                   \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0), SIZE_OVERRIDE(2),                     \
                VIRTUAL(1), SIZE_OVERRIDE(2)                      \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(s2s8,                                   \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0), SIZE_OVERRIDE(2),                     \
                VIRTUAL(1), SIZE_OVERRIDE(2)                      \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(s4s8,                                   \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0), SIZE_OVERRIDE(4),                     \
                VIRTUAL(1), SIZE_OVERRIDE(4)                      \
            )                                                     \
        )                                                         \
        /* No need to actually sign extend if the types */        \
        /* are actually the same size */                          \
        INSSEL_MACRO_CASE(ss,                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(1),                                       \
            )                                                     \
        )                                                         \
        /* Widening / narrowing of constants can be done */       \
        /* by the assembler */                                    \
        INSSEL_MACRO_CASE(si,                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(1),                                       \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(mti,                                             \
        INSSEL_MACRO_CASE(ass asi ais aii,                        \
            INSSEL_MACRO_REPLACE2(mov_os,                         \
                OFFSET(0, 1),,                                    \
                VIRTUAL(2),                                       \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sss ssi sis sii,                        \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                NEW(0),,                                          \
                VIRTUAL(0),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(add_ss,                         \
                NEW(0),,                                          \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                NEW(0), DEREFERENCE,                              \
                VIRTUAL(2),                                       \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(mul,                                             \
        INSSEL_MACRO_CASE(sss,                                    \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(imul_ss,                        \
                VIRTUAL(0),,                                      \
                VIRTUAL(2),                                       \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssi,                                    \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(imul_ss,                        \
                VIRTUAL(0),,                                      \
                VIRTUAL(2),                                       \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sis,                                    \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(imul_ss,                        \
                VIRTUAL(0),,                                      \
                VIRTUAL(1),                                       \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sii,                                    \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(imul_ss,                        \
                VIRTUAL(0),,                                      \
                VIRTUAL(1),                                       \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(not,                                             \
        INSSEL_MACRO_CASE(ss,                                     \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(0),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(test_ss,                        \
                VIRTUAL(1),,                                      \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setz_s,                         \
                VIRTUAL(0), SIZE_OVERRIDE(1)                      \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(si,                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                NEW(0),,                                          \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(xor_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(0),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(test_ss,                        \
                NEW(0),,                                          \
                NEW(0),                                           \
            )                                                     \
            INSSEL_MACRO_REPLACE1(setz_s,                         \
                VIRTUAL(0), SIZE_OVERRIDE(1)                      \
            )                                                     \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(ret,                                             \
        INSSEL_MACRO_CASE(s,                                      \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                LOCATION(loc_a),,                                 \
                VIRTUAL(0),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE0(leave_)                         \
            INSSEL_MACRO_REPLACE0(ret_)                           \
        )                                                         \
        INSSEL_MACRO_CASE(i,                                      \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                LOCATION(loc_a),,                                 \
                VIRTUAL(0),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE0(leave_)                         \
            INSSEL_MACRO_REPLACE0(ret_)                           \
        )                                                         \
    )                                                             \
    INSSEL_MACRO(sub,                                             \
        INSSEL_MACRO_CASE(sss,                                    \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(sub_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(2),                                       \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(ssi,                                    \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(sub_ss,                         \
                VIRTUAL(0),,                                      \
                VIRTUAL(2),                                       \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sis,                                    \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                NEW(0),,                                          \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(sub_ss,                         \
                NEW(0),,                                          \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0),,                                      \
                NEW(0),                                           \
            )                                                     \
        )                                                         \
        INSSEL_MACRO_CASE(sii,                                    \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                NEW(0),,                                          \
                VIRTUAL(1),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(sub_ss,                         \
                NEW(0),,                                          \
                VIRTUAL(2),                                       \
            )                                                     \
            INSSEL_MACRO_REPLACE2(mov_ss,                         \
                VIRTUAL(0),,                                      \
                NEW(0),                                           \
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
#define INSSEL_MACRO_REPLACE0(pasmins__)                                     \
    if (!vec_push_backu(&case__->replace)) goto error;                       \
    replace__ = &vec_back(&case__->replace);                                 \
    replace__->ins = pasmins_ ## pasmins__;                                  \
    replace__->op_count = 0;
#define INSSEL_MACRO_REPLACE1(pasmins__, op1__, op1_flag__)                  \
    INSSEL_MACRO_REPLACE0(pasmins__)                                         \
    replace__->op[0] = op1__;                                                \
    /* Flags may be empty, if so it is initialized to 0 */                   \
    replace__->flag[0] = 0 op1_flag__;                                       \
    replace__->op_count = 1;
#define INSSEL_MACRO_REPLACE2(pasmins__, op1__, op1_flag__,                  \
                                         op2__, op2_flag__)                  \
    INSSEL_MACRO_REPLACE1(pasmins__, op1__, op1_flag__)                      \
    replace__->op[1] = op2__;                                                \
    replace__->flag[1] = 0 op2_flag__;                                       \
    replace__->op_count = 2;

#define NEW(i__) i__ & 0xFF
#define VIRTUAL(i__) (i__ & 0xFF) | 0x10000000
#define LOCATION(loc__) (loc__ & 0xFF) | 0x20000000
#define PHYSICAL(reg__) (reg__ & 0xFF) | 0x30000000
#define OFFSET(base__, offset__) \
         (base__ & 0xFF) | ((offset__ & 0xFF) << 8) | 0x40000000
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

#undef NEW
#undef VIRTUAL
#undef LOCATION
#undef PHYSICAL
#undef OFFSET

#undef SIZE_OVERRIDE
#undef DEREFERENCE


/* Instruction selector special handling */


/* Special handling for call to calculate where to place
   arguments, and registers which must be saved and restored
   Returns 1 if succeeded, 0 if error */
static int inssel_call(Parser* p, Block* blk, const ILStatement* ilstat) {
    PasmStatement pasmstat;

    /* Arguments for call */
    for (int i = 2; i < ilstat_argc(ilstat); ++i) {
        SymbolId id = ilstat_arg(ilstat, i);

        /* We spill variables which get passed as arguments as it is much
           easier to generate the code to load arguments into the correct
           registers for the function call.
           Otherwise we have to be careful about the order which the arguments
           get loaded to avoid overwriting live values, e.g.,:
           mov edi, eax <-- Overwriting live value edi
           mov esi, edi <-- edi has the wrong value */
        symbol_set_location(symtab_get(p, id), loc_stack);

        pasmstat_construct(&pasmstat, pasmins_call_param);
        pasmstat_add_op_sym(&pasmstat, id);
        if (!block_add_pasmstat(blk, pasmstat)) return 0;
    }

    /* Call */
    pasmstat_construct(&pasmstat, pasmins_call_);
    pasmstat_add_op_sym(&pasmstat, ilstat_arg(ilstat, 1));
    if (!block_add_pasmstat(blk, pasmstat)) return 0;

    /* Handle return value for call */
    pasmstat_construct(&pasmstat, pasmins_call_cleanup);
    pasmstat_add_op_sym(&pasmstat, ilstat_arg(ilstat, 0));
    if (!block_add_pasmstat(blk, pasmstat)) return 0;

    return 1;
}


/* Instruction selector 2 */


typedef struct {
    CallData call_dat;
    int i_push; /* Index to insert pushes to caller save */
} InsSel2Data;

static void inssel2data_construct(InsSel2Data* dat) {
    call_construct(&dat->call_dat);
    dat->i_push = -1;
}

/* Handles pseudo-assembly instruction selection for call_param
   dat: Used to store information for instruction selection, pass the
        same object for the duration of the instruction selection process
   idx: Index of current pseudo-assembly statement within block */
static void inssel2_call_param(
        InsSel2Data* dat, Parser* p, Block* blk, int idx) {
    /* If is -1, means this is the first parameter of a function call */
    if (dat->i_push == -1) {
        dat->i_push = idx;
    }

    PasmStatement* pasmstat = block_pasmstat(blk, idx);

    /* Setup arguments for calling convention */
    SymbolId id = pasmstat_op(pasmstat, 0);
    Symbol* sym = symtab_get(p, id);

    /* mov reg, sym */
    Location loc = call_arg_loc(&dat->call_dat, sym);
    SymbolId dest_id = symtab_add_temporaryr(
            p, reg_get(loc, symbol_bytes(sym)));

    pasmstat_construct(pasmstat, pasmins_mov_ss);
    pasmstat_add_op_sym(pasmstat, dest_id);
    pasmstat_add_op_sym(pasmstat, id);
}

/* Handles pseudo-assembly instruction selection for call_clenaup
   dat: Used to store information for instruction selection, pass the
        same object for the duration of the instruction selection process
   idx: Index of current pseudo-assembly statement within block */
static void inssel2_call_cleanup(
        InsSel2Data* dat, Parser* p, Block* blk, int idx) {
    PasmStatement* pasmstat = block_pasmstat(blk, idx);
    /* Save restore live values in caller save registers */
    for (int i = 0; i < pasmstat_live_out_count(pasmstat); ++i) {
        SymbolId id = pasmstat_live_out(pasmstat, i);
        Location loc = symbol_location(symtab_get(p, id));
        if (!call_caller_save(loc)) {
            continue;
        }

        PasmStatement stat;

        ISMRFlag flag = 0;
        ismr_set_size_override(&flag, 8);

        /* push */
        pasmstat_construct(&stat, pasmins_push_s);
        pasmstat_add_op_sym(&stat, id);
        pasmstat_set_flag(&stat, 0, flag);
        block_insert_pasmstat(blk, stat, dat->i_push);

         /* Move insertion point so pushes are arranged in order of from first
            to last, e.g., (push a, push b, push c, ...)*/
        dat->i_push += 1;
        idx += 1; /* Realign index to current statement */

        /* pop */
        pasmstat_construct(&stat, pasmins_pop_s);
        pasmstat_add_op_sym(&stat, id);
        pasmstat_set_flag(&stat, 0, flag);
        /* idx + 1 so is after this statement, this statement is used
           to copy the return value */
        block_insert_pasmstat(blk, stat, idx + 1);

        /* Reload pasmstat as it may have changed */
        pasmstat = block_pasmstat(blk, idx);
    }

    /* Copy the return value to its symbol's location
       after inserting push/pop, as liveness information is
       required and is lost if the memory location is
       reconstructed */
    SymbolId dest_id = pasmstat_op(pasmstat, 0);
    const Symbol* dest_sym = symtab_get(p, dest_id);

    Register reg = reg_get(loc_a, symbol_bytes(dest_sym));
    SymbolId src_id = symtab_add_temporaryr(p, reg);

    pasmstat_construct(pasmstat, pasmins_mov_ss);
    pasmstat_add_op_sym(pasmstat, dest_id);
    pasmstat_add_op_sym(pasmstat, src_id);

    /* Reset index to insert push as now done handling instruction selection
       for this function call */
    dat->i_push = -1;

}

#endif
