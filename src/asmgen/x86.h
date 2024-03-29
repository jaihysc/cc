/* Assembly generator, x86 specifics */
#ifndef ASMGEN_X86_H
#define ASMGEN_X86_H

/* All the registers in x86-64 */
#define X86_REGISTER_COUNT 16 /* Number of registers to store variables */
#define X86_REGISTERS  \
    X86_REGISTER(al)   \
    X86_REGISTER(bl)   \
    X86_REGISTER(cl)   \
    X86_REGISTER(dl)   \
    X86_REGISTER(sil)  \
    X86_REGISTER(dil)  \
    X86_REGISTER(bpl)  \
    X86_REGISTER(spl)  \
    X86_REGISTER(r8b)  \
    X86_REGISTER(r9b)  \
    X86_REGISTER(r10b) \
    X86_REGISTER(r11b) \
    X86_REGISTER(r12b) \
    X86_REGISTER(r13b) \
    X86_REGISTER(r14b) \
    X86_REGISTER(r15b) \
                       \
    X86_REGISTER(ah)   \
    X86_REGISTER(bh)   \
    X86_REGISTER(ch)   \
    X86_REGISTER(dh)   \
                       \
    X86_REGISTER(ax)   \
    X86_REGISTER(bx)   \
    X86_REGISTER(cx)   \
    X86_REGISTER(dx)   \
    X86_REGISTER(si)   \
    X86_REGISTER(di)   \
    X86_REGISTER(bp)   \
    X86_REGISTER(sp)   \
    X86_REGISTER(r8w)  \
    X86_REGISTER(r9w)  \
    X86_REGISTER(r10w) \
    X86_REGISTER(r11w) \
    X86_REGISTER(r12w) \
    X86_REGISTER(r13w) \
    X86_REGISTER(r14w) \
    X86_REGISTER(r15w) \
                       \
    X86_REGISTER(eax)  \
    X86_REGISTER(ebx)  \
    X86_REGISTER(ecx)  \
    X86_REGISTER(edx)  \
    X86_REGISTER(esi)  \
    X86_REGISTER(edi)  \
    X86_REGISTER(ebp)  \
    X86_REGISTER(esp)  \
    X86_REGISTER(r8d)  \
    X86_REGISTER(r9d)  \
    X86_REGISTER(r10d) \
    X86_REGISTER(r11d) \
    X86_REGISTER(r12d) \
    X86_REGISTER(r13d) \
    X86_REGISTER(r14d) \
    X86_REGISTER(r15d) \
                       \
    X86_REGISTER(rax)  \
    X86_REGISTER(rbx)  \
    X86_REGISTER(rcx)  \
    X86_REGISTER(rdx)  \
    X86_REGISTER(rsi)  \
    X86_REGISTER(rdi)  \
    X86_REGISTER(rbp)  \
    X86_REGISTER(rsp)  \
    X86_REGISTER(r8)   \
    X86_REGISTER(r9)   \
    X86_REGISTER(r10)  \
    X86_REGISTER(r11)  \
    X86_REGISTER(r12)  \
    X86_REGISTER(r13)  \
    X86_REGISTER(r14)  \
    X86_REGISTER(r15)

#define X86_REGISTER(reg__) reg_ ## reg__,
typedef enum {reg_none = -1, X86_REGISTERS} Register;
#undef X86_REGISTER
#define X86_REGISTER(reg__) #reg__ ,
const char* reg_strings[] = {X86_REGISTERS};
#undef X86_REGISTER

/* Refers to a location:
   - various sizes of a register
     e.g., loc_a refers to: al, ah, ax eax, rax
   - stack */
#define SYMBOL_LOCATIONS \
    SYMBOL_LOCATION(a)   \
    SYMBOL_LOCATION(b)   \
    SYMBOL_LOCATION(c)   \
    SYMBOL_LOCATION(d)   \
    SYMBOL_LOCATION(si)  \
    SYMBOL_LOCATION(di)  \
    SYMBOL_LOCATION(bp)  \
    SYMBOL_LOCATION(sp)  \
    SYMBOL_LOCATION(8)   \
    SYMBOL_LOCATION(9)   \
    SYMBOL_LOCATION(10)  \
    SYMBOL_LOCATION(11)  \
    SYMBOL_LOCATION(12)  \
    SYMBOL_LOCATION(13)  \
    SYMBOL_LOCATION(14)  \
    SYMBOL_LOCATION(15)
#define SYMBOL_LOCATION(loc__) loc_ ## loc__,
/* a starts at 0 so all registers can be stored as an array */
typedef enum {
    loc_none = -3, loc_constant = -2, loc_stack = -1,
    SYMBOL_LOCATIONS} Location;
#undef SYMBOL_LOCATION
#define SYMBOL_LOCATION(loc__) #loc__,
const char* loc_strings[] = {SYMBOL_LOCATIONS};
#undef SYMBOL_LOCATION

/* Returns 1 if a register of the given bytes exists, 0 otherwise */
static int reg_has_size(int bytes) {
    switch (bytes) {
        case 1:
        case 2:
        case 4:
        case 8:
            return 1;
        default:
            return 0;
    }
}

/* Returns the number of bytes of a register */
static int reg_bytes(Register reg) {
    switch (reg) {
        case reg_al: case reg_ah: case reg_bl: case reg_bh: case reg_cl:
        case reg_ch: case reg_dl: case reg_dh: case reg_sil: case reg_dil:
        case reg_bpl: case reg_spl: case reg_r8b: case reg_r9b: case reg_r10b:
        case reg_r11b: case reg_r12b: case reg_r13b: case reg_r14b:
        case reg_r15b:
            return 1;
        case reg_ax: case reg_bx: case reg_cx: case reg_dx: case reg_si:
        case reg_di: case reg_bp: case reg_sp: case reg_r8w: case reg_r9w:
        case reg_r10w: case reg_r11w: case reg_r12w: case reg_r13w:
        case reg_r14w: case reg_r15w:
            return 2;
        case reg_eax: case reg_ebx: case reg_ecx: case reg_edx: case reg_esi:
        case reg_edi: case reg_ebp: case reg_esp: case reg_r8d: case reg_r9d:
        case reg_r10d: case reg_r11d: case reg_r12d: case reg_r13d:
        case reg_r14d: case reg_r15d:
            return 4;
        case reg_rax: case reg_rbx: case reg_rcx: case reg_rdx: case reg_rsi:
        case reg_rdi: case reg_rbp: case reg_rsp: case reg_r8: case reg_r9:
        case reg_r10: case reg_r11: case reg_r12: case reg_r13: case reg_r14:
        case reg_r15:
            return 8;
    default:
            ASSERT(0, "Unrecognized register");
            return 0;
    }
}

/* Returns the name to access a given register with the indicates size
   -1 for upper byte (ah), 1 for lower byte (al) */
static Register reg_get(Location loc, int bytes) {
    ASSERT(loc >= 0, "Out of range");
    const Register reg_1l[] = {
        reg_al, reg_bl, reg_cl, reg_dl, reg_sil, reg_dil, reg_bpl, reg_spl,
        reg_r8b, reg_r9b, reg_r10b, reg_r11b, reg_r12b, reg_r13b, reg_r14b,
        reg_r15b
    };
    const Register reg_1h[] = {reg_ah, reg_bh, reg_ch, reg_dh};
    const Register reg_2[] = {
        reg_ax, reg_bx, reg_cx, reg_dx, reg_si, reg_di, reg_bp, reg_sp,
        reg_r8w, reg_r9w, reg_r10w, reg_r11w, reg_r12w, reg_r13w, reg_r14w,
        reg_r15w
    };
    const Register reg_4[] = {
        reg_eax, reg_ebx, reg_ecx, reg_edx, reg_esi, reg_edi, reg_ebp, reg_esp,
        reg_r8d, reg_r9d, reg_r10d, reg_r11d, reg_r12d, reg_r13d, reg_r14d,
        reg_r15d
    };
    const Register reg_8[] = {
        reg_rax, reg_rbx, reg_rcx, reg_rdx, reg_rsi, reg_rdi, reg_rbp, reg_rsp,
        reg_r8, reg_r9, reg_r10, reg_r11, reg_r12, reg_r13, reg_r14,
        reg_r15
    };
    switch (bytes) {
        case 1:
            return reg_1l[loc];
        case -1:
            ASSERT(loc <= loc_d, "Out of range");
            return reg_1h[loc];
        case 2:
            return reg_2[loc];
        case 4:
            return reg_4[loc];
        case 8:
            return reg_8[loc];
        default:
            ASSERT(0, "Bad byte size");
    }
}

/* Converts Register into a Location */
static Location reg_loc(Register reg) {
    switch (reg) {
        case reg_al:
        case reg_ah:
        case reg_ax:
        case reg_eax:
        case reg_rax:
            return loc_a;

        case reg_bl:
        case reg_bh:
        case reg_bx:
        case reg_ebx:
        case reg_rbx:
            return loc_b;

        case reg_cl:
        case reg_ch:
        case reg_cx:
        case reg_ecx:
        case reg_rcx:
            return loc_c;

        case reg_dl:
        case reg_dh:
        case reg_dx:
        case reg_edx:
        case reg_rdx:
            return loc_d;

        case reg_sil:
        case reg_si:
        case reg_esi:
        case reg_rsi:
            return loc_si;

        case reg_dil:
        case reg_di:
        case reg_edi:
        case reg_rdi:
            return loc_di;

        case reg_bpl:
        case reg_bp:
        case reg_ebp:
        case reg_rbp:
            return loc_bp;

        case reg_spl:
        case reg_sp:
        case reg_esp:
        case reg_rsp:
            return loc_sp;

        case reg_r8b:
        case reg_r8w:
        case reg_r8d:
        case reg_r8:
            return loc_8;

        case reg_r9b:
        case reg_r9w:
        case reg_r9d:
        case reg_r9:
            return loc_9;

        case reg_r10b:
        case reg_r10w:
        case reg_r10d:
        case reg_r10:
            return loc_10;

        case reg_r11b:
        case reg_r11w:
        case reg_r11d:
        case reg_r11:
            return loc_11;

        case reg_r12b:
        case reg_r12w:
        case reg_r12d:
        case reg_r12:
            return loc_12;

        case reg_r13b:
        case reg_r13w:
        case reg_r13d:
        case reg_r13:
            return loc_13;

        case reg_r14b:
        case reg_r14w:
        case reg_r14d:
        case reg_r14:
            return loc_14;

        case reg_r15b:
        case reg_r15w:
        case reg_r15d:
        case reg_r15:
            return loc_15;

        case reg_none:
            return loc_none;

        default:
            ASSERT(0, "Invalid register");
    }
}

/* Converts given asm_register into its corresponding cstr*/
static const char* reg_str(Register reg) {
    ASSERT(reg >= 0, "Invalid register");
    return reg_strings[reg];
}

/* Converts given Location to its corresponding c string */
static const char* loc_str(Location loc) {
    ASSERT(loc >= -3, "Invalid register");
    if (loc == loc_none) {
        return "none";
    }
    if (loc == loc_constant) {
        return "constant";
    }
    if (loc == loc_stack) {
        return "stack";
    }
    return loc_strings[loc];
}

/* Returns 1 if location is a register, 0 otherwise */
static int loc_is_register(Location loc) {
    switch (loc) {
        case loc_a: case loc_b: case loc_c: case loc_d:
        case loc_bp: case loc_sp: case loc_si: case loc_di:
        case loc_8: case loc_9: case loc_10: case loc_11:
        case loc_12: case loc_13: case loc_14: case loc_15:
            return 1;
        default:
            return 0;
    }
}

/* Returns the cstr of the register corresponding to the provided
   location with the indicated size */
static const char* reg_get_str(Location loc, int bytes) {
    return reg_str(reg_get(loc, bytes));
}

/* Returns the size directive used to access bytes from memory location */
static const char* asm_size_directive(int bytes) {
    switch (bytes) {
        case 1:
            return "BYTE";
        case 2:
            return "WORD";
        case 4:
            return "DWORD";
        case 8:
            return "QWORD";
        default:
            ASSERT(0, "Bad byte size");
            return "";
    }
}

#define MAX_ASM_OP 2 /* Maximum operands for assembly instruction */
#define MAX_ASMINS_REG 3 /* Maximum registers used per assembly instruction */
#define ASMINSS                   \
    ASMINS(none)                  \
                                  \
    ASMINS(add)                   \
    ASMINS(call)                  \
    ASMINS(cdq)                   \
    ASMINS(cmp)                   \
    ASMINS(cqo)                   \
    ASMINS(cwd)                   \
    ASMINS(div)                   \
    ASMINS(idiv)                  \
    ASMINS(imul)                  \
    ASMINS(jmp)                   \
    ASMINS(jnz)                   \
    ASMINS(jz)                    \
    ASMINS(lea)                   \
    ASMINS(leave)                 \
    ASMINS(mov)                   \
    ASMINS(movsx)                 \
    ASMINS(movzx)                 \
    ASMINS(pop)                   \
    ASMINS(push)                  \
    ASMINS(ret)                   \
    ASMINS(setb)                  \
    ASMINS(setbe)                 \
    ASMINS(sete)                  \
    ASMINS(setl)                  \
    ASMINS(setle)                 \
    ASMINS(setne)                 \
    ASMINS(setz)                  \
    ASMINS(sub)                   \
    ASMINS(test)                  \
    ASMINS(xchg)                  \
    ASMINS(xor)

#define ASMINS(name__) asmins_ ## name__,
typedef enum {ASMINSS asmins_count} AsmIns;
#undef ASMINS

/* Returns x86 instruction string for AsmIns */
static const char* asmins_str(AsmIns asmins) {
#define ASMINS(name__) #name__,
    const char* strings[] = {ASMINSS};
#undef ASMINS
#undef ASMINSS
    ASSERT(asmins >= 0, "Invalid AsmIns");
    return strings[asmins];
}

/* Returns 1 if asmins has the effect of pushing onto a stack, 0 if not */
static int asmins_is_push(AsmIns asmins) {
    return asmins == asmins_push;
}

/* Returns 1 if asmins has the effect of popping from a stack, 0 if not */
static int asmins_is_pop(AsmIns asmins) {
    return asmins == asmins_pop;
}

/* Returns 1 if asmins has the effect of copying from a source to a
   destination, 0 if not */
static int asmins_is_copy(AsmIns asmins) {
    return asmins == asmins_mov;
}

/* Returns the index of the source operand for a copy instruction */
static int asmins_copy_src_index(void) {
    return 1;
}

/* Returns the index of the destination operand for a copy instruction */
static int asmins_copy_dest_index(void) {
    return 0;
}

/* X86_PASMINS(asmins__, pasm_mode__, mode__)
     asmins__: The assembly instruction (AsmIns) this pseudo-assembly
               instruction maps to
     pasm_mode__: The pseudo-assembly addressing mode - PasmMode
                  (See PasmStatement for PasmMode)
     mode__: The x86 addressing mode for each operand

   ADDRESS_MODE(m1__, m2__, m3__)
     m1__, m2__, m3__: The addressing mode for each operand, respectively
     Use the macros NONE, IMM, R, M, RM which corresponds to x86 imm, r, m, r/m
*/

#define PASMINSS                            \
    PASMINS(add, ss,                        \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))          \
    PASMINS(call,,                          \
        ADDRESS_MODE(NONE, NONE, NONE))     \
    PASMINS(cdq,,                           \
        ADDRESS_MODE(NONE, NONE, NONE))     \
    PASMINS(cmp, ss,                        \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))          \
    PASMINS(cqo,,                           \
        ADDRESS_MODE(NONE, NONE, NONE))     \
    PASMINS(cwd,,                           \
        ADDRESS_MODE(NONE, NONE, NONE))     \
    PASMINS(div, s,                         \
        ADDRESS_MODE(RM, NONE, NONE))       \
    PASMINS(idiv, s,                        \
        ADDRESS_MODE(RM, NONE, NONE))       \
    PASMINS(imul, ss,                       \
        ADDRESS_MODE(R, IMM, NONE)          \
        ADDRESS_MODE(R, RM, NONE))          \
    PASMINS(jmp,,                           \
        ADDRESS_MODE(NONE, NONE, NONE))     \
    PASMINS(jnz,,                           \
        ADDRESS_MODE(NONE, NONE, NONE))     \
    PASMINS(jz,,                            \
        ADDRESS_MODE(NONE, NONE, NONE))     \
    PASMINS(lea, ss,                        \
        ADDRESS_MODE(R, M, NONE))           \
    PASMINS(leave,,                         \
        ADDRESS_MODE(NONE, NONE, NONE))     \
    PASMINS(mov, ss,                        \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))          \
    PASMINS(mov, so,                        \
        ADDRESS_MODE(R, M, R))              \
    PASMINS(mov, os,                        \
        ADDRESS_MODE(M, R, IMM)             \
        ADDRESS_MODE(M, R, R))              \
    PASMINS(movsx, ss,                      \
        ADDRESS_MODE(R, RM, NONE))          \
    PASMINS(movzx, ss,                      \
        ADDRESS_MODE(R, RM, NONE))          \
    PASMINS(pop, s,                         \
        ADDRESS_MODE(RM, NONE, NONE))       \
    PASMINS(push, s,                        \
        ADDRESS_MODE(RM, NONE, NONE))       \
    PASMINS(ret,,                           \
        ADDRESS_MODE(NONE, NONE, NONE))     \
    PASMINS(setb, s,                        \
        ADDRESS_MODE(RM, NONE, NONE))       \
    PASMINS(setbe, s,                       \
        ADDRESS_MODE(RM, NONE, NONE))       \
    PASMINS(sete, s,                        \
        ADDRESS_MODE(RM, NONE, NONE))       \
    PASMINS(setl, s,                        \
        ADDRESS_MODE(RM, NONE, NONE))       \
    PASMINS(setle, s,                       \
        ADDRESS_MODE(RM, NONE, NONE))       \
    PASMINS(setne, s,                       \
        ADDRESS_MODE(RM, NONE, NONE))       \
    PASMINS(setz, s,                        \
        ADDRESS_MODE(RM, NONE, NONE))       \
    PASMINS(sub, ss,                        \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))          \
    PASMINS(test, ss,                       \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE))          \
    PASMINS(xchg, ss,                       \
        ADDRESS_MODE(R, RM, NONE)           \
        ADDRESS_MODE(M, R, NONE))           \
    PASMINS(xor, ss,                        \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))

/* Special PASMINSS (Used by instruction selector 2) */
#define SPASMINSS                                        \
    /* Put parameter in appropriate location for call */ \
    SPASMINS(call_param)                                 \
    /* Handle caller save registers, return value */     \
    SPASMINS(call_cleanup)                               \
                                                         \
    /* Divides divident, divisor */                      \
    /* Usage: divide <divident>, <divisor> */            \
    /* A div_cleanupq or div_cleanupr must follow to */  \
    /* output the quotient or the remainder */           \
    SPASMINS(divide)                                     \
    /* For the two below: Outputs quotient/remainder */  \
    /* into destination */                               \
    /* Usage: div_cleanupq <dest> */                     \
    /*        div_cleanupr <dest> */                     \
    SPASMINS(div_cleanupq) /* Load quotient */           \
    SPASMINS(div_cleanupr) /* Load remainder */

typedef enum {
#define PASMINS(name__, pasm_mode__, mode__) \
    pasmins_ ## name__ ## _ ## pasm_mode__,
#define SPASMINS(name__) pasmins_ ## name__,
    PASMINSS
    SPASMINSS
    /* Number of normal PasmIns, used to identify when the special PasmIns
       is used where it should not */
    pasmins_ncount = pasmins_call_param
#undef SPASMINS
#undef PASMINS
} PasmIns;

/* Converts PasmIns to a string */
static const char* pasmins_str(PasmIns pins) {
#define PASMINS(name__, pasm_mode__, mode__) #name__ "_" #pasm_mode__,
#define SPASMINS(name__) #name__,
    const char* strings[] = {PASMINSS SPASMINSS};
#undef SPASMINS
#undef PASMINS

    ASSERT(pins >= 0, "Invalid pseudo-assembly instruction");
    return strings[pins];
}

/* Converts PasmIns to a AsmIns */
static AsmIns pasmins_asm(PasmIns pins) {
    ASSERT(pins >= 0, "Invalid pseudo-assembly instruction");

    /* Having to check and separately handle special pseudo-assembly
       instructions is annoying in uses such as:
       pasmins_asm(pins) == asmins_lea
       Thus return none so the call still succeeds */
    if (pins >= pasmins_ncount) {
        return asmins_none;
    }

#define PASMINS(name__, pasm_mode__, mode__) asmins_ ## name__,
    const int asmins[] = {PASMINSS};
#undef PASMINS
    return asmins[pins];
}

/* Returns the number of addressing modes for PasmIns */
static int pasmins_mode_count(PasmIns pins) {
    ASSERT(pins >= 0, "Invalid pseudo-assembly instruction");
    ASSERT(pins < pasmins_ncount, "Invalid pseudo-assembly instruction");
    /* Expands out into an expression 0 + 1 + 1 ... */
#define PASMINS(name__, pasm_mode__, mode__) 0 mode__,
#define ADDRESS_MODE(m1__, m2__, m3__) + 1
    const int modes[] = {PASMINSS};
#undef ADDRESS_MODE
#undef PASMINS
    return modes[pins];
}

typedef struct {
    /* Mode for each operand */
    int op[MAX_ASMINS_REG];
} AddressMode;

/* Returns ith addressing mode for PasmIns */
static AddressMode asmins_mode(PasmIns pins, int i) {
    ASSERT(pins >= 0, "Invalid pseudo-assembly instruction");
    ASSERT(pins < pasmins_ncount, "Invalid pseudo-assembly instruction");
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < pasmins_mode_count(pins), "Index out of range");
#define NONE 0
#define IMM 1
#define R 2
#define M 3
#define RM 4
#define PASMINS(name__, pasm_mode__, mode__) mode__
#define ADDRESS_MODE(m1__, m2__, m3__) {{m1__, m2__, m3__}},
    const AddressMode modes[] = {PASMINSS};
#undef ADDRESS_MODE
#undef PASMINS
    /* Count the number of addressing modes for prior instructions to
       find the offer to the desired instruction */
    int index = 0;
    for (int j = 0; j < (int)pins; ++j) {
        index += pasmins_mode_count(j);
    }
    index += i;
    return modes[index];
}

/* Returns 1 if addressing mode at operand i can address register, 0 if not */
static int addressmode_reg(const AddressMode* mode, int i) {
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < MAX_ASMINS_REG, "Index out of range");
    return mode->op[i] == R || mode->op[i] == RM;
}

/* Returns 1 if addressing mode at operand i can address memory, 0 if not */
static int addressmode_mem(const AddressMode* mode, int i) {
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < MAX_ASMINS_REG, "Index out of range");
    return mode->op[i] == M || mode->op[i] == RM;
}

/* Returns 1 if addressing mode at operand i can address immediate, 0 if not */
static int addressmode_imm(const AddressMode* mode, int i) {
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < MAX_ASMINS_REG, "Index out of range");
    return mode->op[i] == IMM;
}

#undef RM
#undef R
#undef IMM
#undef NONE

#endif
