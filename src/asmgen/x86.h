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
#define ASMINSS   \
    ASMINS(add)   \
    ASMINS(cmp)   \
    ASMINS(idiv)  \
    ASMINS(imul)  \
    ASMINS(jmp)   \
    ASMINS(jnz)   \
    ASMINS(jz)    \
    ASMINS(leave) \
    ASMINS(mov)   \
    ASMINS(movsx) \
    ASMINS(movzx) \
    ASMINS(pop)   \
    ASMINS(push)  \
    ASMINS(ret)   \
    ASMINS(setb)  \
    ASMINS(setbe) \
    ASMINS(sete)  \
    ASMINS(setl)  \
    ASMINS(setle) \
    ASMINS(setne) \
    ASMINS(setz)  \
    ASMINS(sub)   \
    ASMINS(test)  \
    ASMINS(xor)

#define ASMINS(name__) asmins_ ## name__,
typedef enum {ASMINSS} AsmIns;
#undef ASMINS

/* Returns x86 instruction string for AsmIns */
static const char* asmins_str(AsmIns asmins) {
#define ASMINS(name__) #name__,
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

#endif
