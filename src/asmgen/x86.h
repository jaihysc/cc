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
    ASMINS(add,                   \
        ADDRESS_MODE(RM, IMM)     \
        ADDRESS_MODE(RM, R)       \
        ADDRESS_MODE(R, RM))      \
    ASMINS(cmp,                   \
        ADDRESS_MODE(RM, IMM)     \
        ADDRESS_MODE(RM, R)       \
        ADDRESS_MODE(R, RM))      \
    ASMINS(idiv,                  \
        ADDRESS_MODE(RM, NONE))   \
    ASMINS(imul,                  \
        ADDRESS_MODE(R, IMM)      \
        ADDRESS_MODE(R, RM))      \
    ASMINS(jmp,                   \
        ADDRESS_MODE(NONE, NONE)) \
    ASMINS(jnz,                   \
        ADDRESS_MODE(NONE, NONE)) \
    ASMINS(jz,                    \
        ADDRESS_MODE(NONE, NONE)) \
    ASMINS(lea,                   \
        ADDRESS_MODE(R, M))       \
    ASMINS(leave,                 \
        ADDRESS_MODE(NONE, NONE)) \
    ASMINS(mov,                   \
        ADDRESS_MODE(RM, IMM)     \
        ADDRESS_MODE(RM, R)       \
        ADDRESS_MODE(R, RM))      \
    ASMINS(movsx,                 \
        ADDRESS_MODE(R, RM))      \
    ASMINS(movzx,                 \
        ADDRESS_MODE(R, RM))      \
    ASMINS(pop,                   \
        ADDRESS_MODE(RM, NONE))   \
    ASMINS(push,                  \
        ADDRESS_MODE(RM, NONE))   \
    ASMINS(ret,                   \
        ADDRESS_MODE(NONE, NONE)) \
    ASMINS(setb,                  \
        ADDRESS_MODE(RM, NONE))   \
    ASMINS(setbe,                 \
        ADDRESS_MODE(RM, NONE))   \
    ASMINS(sete,                  \
        ADDRESS_MODE(RM, NONE))   \
    ASMINS(setl,                  \
        ADDRESS_MODE(RM, NONE))   \
    ASMINS(setle,                 \
        ADDRESS_MODE(RM, NONE))   \
    ASMINS(setne,                 \
        ADDRESS_MODE(RM, NONE))   \
    ASMINS(setz,                  \
        ADDRESS_MODE(RM, NONE))   \
    ASMINS(sub,                   \
        ADDRESS_MODE(RM, IMM)     \
        ADDRESS_MODE(RM, R)       \
        ADDRESS_MODE(R, RM))      \
    ASMINS(test,                  \
        ADDRESS_MODE(RM, IMM)     \
        ADDRESS_MODE(RM, R))      \
    ASMINS(xor,                   \
        ADDRESS_MODE(RM, IMM)     \
        ADDRESS_MODE(RM, R)       \
        ADDRESS_MODE(R, RM))










#define ASMINS(name__, modes__) asmins_ ## name__,
typedef enum {ASMINSS asmins_count} AsmIns;
#undef ASMINS

/* Returns x86 instruction string for AsmIns */
static const char* asmins_str(AsmIns asmins) {
#define ASMINS(name__, modes__) #name__,
    const char* strings[] = {ASMINSS};
#undef ASMINS
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

typedef struct {
    /* Mode for each operand */
    int op[MAX_ASMINS_REG];
} AddressMode;






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
    ASMINS(add, rr,                         \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(add, rs,                         \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(add, sr,                         \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(add, ss,                         \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(cmp, rr,                         \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(cmp, rs,                         \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(cmp, sr,                         \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(cmp, ss,                         \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(idiv, r,                         \
        ADDRESS_MODE(RM, NONE, NONE))       \
    ASMINS(idiv, s,                         \
        ADDRESS_MODE(RM, NONE, NONE))       \
    ASMINS(imul, rr,                        \
        ADDRESS_MODE(R, IMM, NONE)          \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(imul, rs,                        \
        ADDRESS_MODE(R, IMM, NONE)          \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(imul, sr,                        \
        ADDRESS_MODE(R, IMM, NONE)          \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(imul, ss,                        \
        ADDRESS_MODE(R, IMM, NONE)          \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(jmp,,                            \
        ADDRESS_MODE(NONE, NONE, NONE))     \
    ASMINS(jnz,,                            \
        ADDRESS_MODE(NONE, NONE, NONE))     \
    ASMINS(jz,,                             \
        ADDRESS_MODE(NONE, NONE, NONE))     \
    ASMINS(lea, rs,                         \
        ADDRESS_MODE(R, M, NONE))           \
    ASMINS(lea, ss,                         \
        ADDRESS_MODE(R, M, NONE))           \
    ASMINS(leave,,                          \
        ADDRESS_MODE(NONE, NONE, NONE))     \
    ASMINS(mov, rr,                         \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(mov, rs,                         \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(mov, ro,                         \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(mov, sr,                         \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(mov, ss,                         \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(mov, so,                         \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(mov, or,                         \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(mov, os,                         \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(movsx, rr,                       \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(movsx, rs,                       \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(movsx, sr,                       \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(movsx, ss,                       \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(movzx, rr,                       \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(movzx, rs,                       \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(movzx, sr,                       \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(movzx, ss,                       \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(pop, r,                          \
        ADDRESS_MODE(RM, NONE, NONE))       \
    ASMINS(pop, s,                          \
        ADDRESS_MODE(RM, NONE, NONE))       \
    ASMINS(push, r,                         \
        ADDRESS_MODE(RM, NONE, NONE))       \
    ASMINS(push, s,                         \
        ADDRESS_MODE(RM, NONE, NONE))       \
    ASMINS(ret,,                            \
        ADDRESS_MODE(NONE, NONE, NONE))     \
    ASMINS(setb, r,                         \
        ADDRESS_MODE(RM, NONE, NONE))       \
    ASMINS(setb, s,                         \
        ADDRESS_MODE(RM, NONE, NONE))       \
    ASMINS(setbe, r,                        \
        ADDRESS_MODE(RM, NONE, NONE))       \
    ASMINS(setbe, s,                        \
        ADDRESS_MODE(RM, NONE, NONE))       \
    ASMINS(sete, r,                         \
        ADDRESS_MODE(RM, NONE, NONE))       \
    ASMINS(sete, s,                         \
        ADDRESS_MODE(RM, NONE, NONE))       \
    ASMINS(setl, r,                         \
        ADDRESS_MODE(RM, NONE, NONE))       \
    ASMINS(setl, s,                         \
        ADDRESS_MODE(RM, NONE, NONE))       \
    ASMINS(setle, r,                        \
        ADDRESS_MODE(RM, NONE, NONE))       \
    ASMINS(setle, s,                        \
        ADDRESS_MODE(RM, NONE, NONE))       \
    ASMINS(setne, r,                        \
        ADDRESS_MODE(RM, NONE, NONE))       \
    ASMINS(setne, s,                        \
        ADDRESS_MODE(RM, NONE, NONE))       \
    ASMINS(setz, r,                         \
        ADDRESS_MODE(RM, NONE, NONE))       \
    ASMINS(setz, s,                         \
        ADDRESS_MODE(RM, NONE, NONE))       \
    ASMINS(sub, rr,                         \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(sub, rs,                         \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(sub, sr,                         \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(sub, ss,                         \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(test, rr,                        \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE))          \
    ASMINS(test, rs,                        \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE))          \
    ASMINS(test, sr,                        \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE))          \
    ASMINS(test, ss,                        \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE))          \
    ASMINS(xor, rr,                         \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(xor, rs,                         \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(xor, sr,                         \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))          \
    ASMINS(xor, ss,                         \
        ADDRESS_MODE(RM, IMM, NONE)         \
        ADDRESS_MODE(RM, R, NONE)           \
        ADDRESS_MODE(R, RM, NONE))










#define ASMINS(name__, pasm_mode__, mode__) \
    pasmins_ ## name__ ## _ ## pasm_mode__,
typedef enum {PASMINSS} PasmIns;
#undef ASMINS

/* Converts PasmIns to a AsmIns */
static AsmIns pasmins_asm(PasmIns pins) {
#define ASMINS(name__, pasm_mode__, mode__) asmins_ ## name__,
    const int asmins[] = {PASMINSS};
#undef ASMINS
    return asmins[pins];
}


// TODO Update comments, rename variables, .

/* Returns the number of addressing modes for asmins */
static int asmins_mode_count(PasmIns asmins) {
    /* Expands out into an expression 0 + 1 + 1 ... */
#define ASMINS(name__, pasm_mode__, mode__) 0 mode__,
#define ADDRESS_MODE(m1__, m2__, m3__) + 1
    const int modes[] = {PASMINSS};
#undef ADDRESS_MODE
#undef ASMINS
    return modes[asmins];
}

/* Returns ith addressing mode for asmins */
static AddressMode asmins_mode(PasmIns asmins, int i) {
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < asmins_mode_count(asmins), "Index out of range");
#define NONE 0
#define IMM 1
#define R 2
#define M 3
#define RM 4
#define ASMINS(name__, pasm_mode__, mode__) mode__
#define ADDRESS_MODE(m1__, m2__, m3__) {{m1__, m2__, m3__}},
    const AddressMode modes[] = {PASMINSS};
#undef ADDRESS_MODE
#undef ASMINS
    /* Count the number of addressing modes for prior instructions to
       find the offer to the desired instruction */
    int index = 0;
    for (int j = 0; j < (int)asmins; ++j) {
        index += asmins_mode_count(j);
    }
    index += i;
    return modes[index];
}

/* Returns 1 if addressing mode at operand i can address register, 0 if not */
static int addressmode_reg(const AddressMode* mode, int i) {
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < MAX_ASM_OP, "Index out of range");
    return mode->op[i] == R || mode->op[i] == RM;
}

/* Returns 1 if addressing mode at operand i can address memory, 0 if not */
static int addressmode_mem(const AddressMode* mode, int i) {
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < MAX_ASM_OP, "Index out of range");
    return mode->op[i] == M || mode->op[i] == RM;
}

/* Returns 1 if addressing mode at operand i can address immediate, 0 if not */
static int addressmode_imm(const AddressMode* mode, int i) {
    ASSERT(i >= 0, "Index out of range");
    ASSERT(i < MAX_ASM_OP, "Index out of range");
    return mode->op[i] == IMM;
}

#undef RM
#undef R
#undef IMM
#undef NONE

#undef ASMINSS
#endif
