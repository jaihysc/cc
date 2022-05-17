/*
 Reads in intermediate language (imm2)
 Generated output x86-64 assembly (imm3)
*/

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include "common.h"

#define MAX_INSTRUCTION_LEN 256 /* Includes null terminator */
#define MAX_ARG_LEN 2048   /* Includes null terminator */
#define MAX_ARGS 256

/* ============================================================ */
/* Parser global configuration */

int g_debug_print_buffers = 0;

/* ============================================================ */
/* x86 Registers */

/* All the registers in x86-64 (just 8 byte ones for now) */
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
typedef enum {reg_none = -2, reg_stack = -1, X86_REGISTERS} Register;
#undef X86_REGISTER
#define X86_REGISTER(reg__) #reg__ ,
const char* reg_strings[] = {X86_REGISTERS};
#undef X86_REGISTER

/* Refers to the various sizes of a register (GenericRegister)
   e.g., reg_a refers to: al, ah, ax eax, rax */
typedef enum {
    reg_a,
    reg_b,
    reg_c,
    reg_d
} GRegister;

/* Returns the name to access a given register with the indicates size
   -1 for upper byte (ah), 1 for lower byte (al) */
static Register reg_get(GRegister greg, int bytes) {
    const Register reg_1l[] = {reg_al, reg_bl, reg_cl, reg_dl};
    const Register reg_1h[] = {reg_ah, reg_bh, reg_ch, reg_dh};
    const Register reg_4[] = {reg_eax, reg_ebx, reg_ecx, reg_edx};
    const Register reg_8[] = {reg_rax, reg_rbx, reg_rcx, reg_rdx};
    switch (bytes) {
        case 1:
            return reg_1l[greg];
        case -1:
            return reg_1h[greg];
        case 4:
            return reg_4[greg];
        case 8:
            return reg_8[greg];
        default:
            ASSERT(0, "Bad byte size");
    }
}

/* Converts given asm_register into its corresponding cstr*/
static const char* reg_str(Register reg) {
    ASSERT(reg >= 0, "Invalid register");
    return reg_strings[reg];
}

/* Returns the cstr of the register corresponding to the provided
   register with the indicated size */
static const char* reg_get_str(GRegister greg, int bytes) {
    return reg_str(reg_get(greg, bytes));
}

/* ============================================================ */
/* Assembly manipulation */

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

/* ============================================================ */
/* Parser data structure + functions */

#define ERROR_CODES            \
    ERROR_CODE(noerr)          \
    ERROR_CODE(insbufexceed)   \
    ERROR_CODE(argbufexceed)   \
    ERROR_CODE(scopelenexceed) \
    ERROR_CODE(invalidins)     \
    ERROR_CODE(invalidinsop)   \
    ERROR_CODE(badargs)        \
    ERROR_CODE(badmain)        \
    ERROR_CODE(writefailed)    \
    ERROR_CODE(seekfailed)     \
    ERROR_CODE(unknownsym)

#define ERROR_CODE(name__) ec_ ## name__,
typedef enum {ERROR_CODES} ErrorCode;
#undef ERROR_CODE
#define ERROR_CODE(name__) #name__,
char* errcode_str[] = {ERROR_CODES};
#undef ERROR_CODE
#undef ERROR_CODES

typedef int SymbolId;
typedef struct {
    Type type;
    char name[MAX_ARG_LEN + 1]; /* +1 for null terminator */
    Register reg;
} Symbol;

/* Returns type for given symbol */
static Type symbol_type(Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->type;
}

/* Returns name for given symbol */
static const char* symbol_name(Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->name;
}

/* Returns where symbol is stored */
static Register symbol_location(Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->reg;
}

/* Returns 1 if symbol is located on the stack, 0 otherwise */
static int symbol_on_stack(Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->reg == reg_stack;
}

/* Returns 1 if symbol is a constant */
static int symbol_is_constant(Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->reg == reg_none;
}

/* Turns this symbol into a special symbol for representing constants */
static void symbol_make_constant(Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    sym->reg = reg_none;
}

/* Returns bytes for symbol */
static int symbol_bytes(Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return type_bytes(sym->type);
}

typedef vec_t(Symbol) vec_Symbol;

typedef struct {
    ErrorCode ecode;

    FILE* rf; /* Input file */
    FILE* of; /* Generated code goes in this file */

    /* For one function only for now */

    /* First symbol element is earliest in occurrence */
    vec_Symbol symbol;
    int stack_bytes; /* Bytes stack needs */

    /* Instruction, argument */
    char ins[MAX_INSTRUCTION_LEN];
    int ins_len;
    char arg[MAX_ARG_LEN];
    char* arg_table[MAX_ARGS]; /* Points to beginning of each argument */
    int arg_count; /* Number of arguments */
} Parser;

static void parser_construct(Parser* p) {
    vec_construct(&p->symbol);
}

static void parser_destruct(Parser* p) {
    vec_destruct(&p->symbol);
}

/* Return 1 if error is set, else 0 */
static int parser_has_error(Parser* p) {
    return p->ecode != ec_noerr;
}

/* Gets error in parser */
static ErrorCode parser_get_error(Parser* p) {
    return p->ecode;
}

/* Sets error in parser */
static void parser_set_error(Parser* p, ErrorCode ecode) {
    p->ecode = ecode;
    LOGF("Error set %d %s\n", ecode, errcode_str[ecode]);
}

/* Writes provided assembly using format string and va_args into output */
static void parser_output_asm(Parser* p, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (vfprintf(p->of, fmt, args) < 0) {
        parser_set_error(p, ec_writefailed);
    }
    va_end(args);
}

/* Writes comment using format string and va_args into output */
static void parser_output_comment(Parser* p, const char* fmt, ...) {
    parser_output_asm(p, "                                           ; ");
    va_list args;
    va_start(args, fmt);
    if (vfprintf(p->of, fmt, args) < 0) {
        parser_set_error(p, ec_writefailed);
    }
    va_end(args);
}

static SymbolId symtab_add(Parser* p, Type type, const char* name);
static Symbol* symtab_get(Parser* p, SymbolId sym_id);

/* Returns 1 if name is within symbol table, 0 otherwise */
static int symtab_contains(Parser* p, const char* name) {
    for (int i = 0; i < vec_size(&p->symbol); ++i) {
        Symbol* symbol = &vec_at(&p->symbol, i);
        if (strequ(symbol->name, name)) {
            return 1;
        }
    }
    return 0;
}

/* Retrieves symbol with given name from symbol table
   Null if not found */
static SymbolId symtab_find(Parser* p, const char* name) {
    for (int i = 0; i < vec_size(&p->symbol); ++i) {
        Symbol* symbol = &vec_at(&p->symbol, i);
        if (strequ(symbol->name, name)) {
            return i;
        }
    }

    /* Special handling for constants, they always exist, thus add
       them to the table when not found to make them exist
       '-' for negative numbers */
    if (('0' <= name[0] && name[0] <= '9') || name[0] == '-') {
        /* TODO calculate size of constant, assume integer for now */
        Type type = {.typespec = ts_i32};
        SymbolId sym_id = symtab_add(p, type, name);
        symbol_make_constant(symtab_get(p, sym_id));
        return sym_id;
    }

    if (g_debug_print_buffers) {
        LOGF("Cannot find %s\n", name);
    }
    return -1;
}

static Symbol* symtab_get(Parser* p, SymbolId sym_id) {
    ASSERT(sym_id >= 0, "Invalid symbol id");
    ASSERT(sym_id < vec_size(&p->symbol), "Symbol id out of range");
    return &vec_at(&p->symbol, sym_id);
}

/* Returns offset from base pointer to access symbol on the stack */
static int symtab_get_offset(Parser* p, SymbolId sym_id) {
    ASSERT(sym_id >= 0, "Symbol not found");

    int offset = 0;
    for (int i = 0; i <= sym_id; ++i) {
        Symbol* sym = &vec_at(&p->symbol, i);
        if (symbol_on_stack(sym)) {
            offset -= symbol_bytes(sym);
        }
    }
    return offset;
}

/* Adds symbol created with given arguments to symbol table
   Returns the added symbol */
static SymbolId symtab_add(Parser* p, Type type, const char* name) {
    ASSERTF(!symtab_contains(p, name), "Duplicate symbol %s", name);

    if (!vec_push_backu(&p->symbol)) {
        parser_set_error(p, ec_scopelenexceed);
        return -1;
    }
    Symbol* sym = &vec_back(&p->symbol);
    sym->type = type;
    strcopy(name, sym->name);
    sym->reg = reg_stack;
    return vec_size(&p->symbol) - 1;
}

/* Dumps contents stored in parser */
static void debug_dump(Parser* p) {
    LOGF("Symbol table: [%d]\n", vec_size(&p->symbol));
    for (int i = 0; i < vec_size(&p->symbol); ++i) {
        Symbol* sym = &vec_at(&p->symbol, i);
        Type type = symbol_type(sym);
        LOGF("  %s", type_specifiers_str(type.typespec));
        for (int j = 0; j < type.pointers; ++j) {
            LOG("*");
        }
        LOGF(" %s %d\n", symbol_name(sym), symbol_location(sym));
    }
}


/* Instruction handlers */

/* Helpers */
static void cg_arithmetic(
        Parser* p,
        SymbolId dest_id, SymbolId op1_id, SymbolId op2_id, const char* ins);
static void cg_cmpsetcc(
        Parser* p,
        SymbolId dest_id, SymbolId op1_id, SymbolId op2_id, const char* set);
static void cg_testjmpcc(
        Parser* p,
        SymbolId label_id, SymbolId op1_id, const char* jmp);
static void cg_divide(
        Parser* p, SymbolId op1_id, SymbolId op2_id,
        const char* ins);
static void cg_mov_tor(Parser* p, SymbolId source, GRegister dest);
static void cg_mov_fromr(Parser* p, GRegister source, SymbolId dest);
static void cg_ref_symbol(Parser* p, SymbolId sym_id);
static void cg_extract_param(const char* str, char* type, char* name);
static void cg_validate_equal_size2(Parser* p,
        SymbolId lval_id, SymbolId rval_id);
static void cg_validate_equal_size3(Parser* p,
        SymbolId lval_id, SymbolId op1_id, SymbolId op2_id);

/* See strbinfind for ordering requirements */
#define INSTRUCTIONS  \
    INSTRUCTION(add)  \
    INSTRUCTION(ce)   \
    INSTRUCTION(cl)   \
    INSTRUCTION(cle)  \
    INSTRUCTION(cne)  \
    INSTRUCTION(def)  \
    INSTRUCTION(div)  \
    INSTRUCTION(func) \
    INSTRUCTION(jmp)  \
    INSTRUCTION(jnz)  \
    INSTRUCTION(jz)   \
    INSTRUCTION(lab)  \
    INSTRUCTION(mod)  \
    INSTRUCTION(mov)  \
    INSTRUCTION(mul)  \
    INSTRUCTION(not)  \
    INSTRUCTION(ret)  \
    INSTRUCTION(sub)
typedef void(*InstructionHandler) (Parser*, char**, int);
/* pparg is pointer to array of size, each element is pointer to null terminated argument string */
#define INSTRUCTION_HANDLER(name__) void handler_ ## name__ (Parser* p, char** pparg, int arg_count)

static INSTRUCTION_HANDLER(add) {
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
    SymbolId lval_id = symtab_find(p, pparg[0]);
    SymbolId op1_id = symtab_find(p, pparg[1]);
    SymbolId op2_id = symtab_find(p, pparg[2]);

    cg_arithmetic(p, lval_id, op1_id, op2_id, "add");
}

static INSTRUCTION_HANDLER(ce) {
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
    SymbolId lval_id = symtab_find(p, pparg[0]);
    SymbolId op1_id = symtab_find(p, pparg[1]);
    SymbolId op2_id = symtab_find(p, pparg[2]);

    cg_cmpsetcc(p, lval_id, op1_id, op2_id, "sete");
}

static INSTRUCTION_HANDLER(cl) {
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
    SymbolId lval_id = symtab_find(p, pparg[0]);
    SymbolId op1_id = symtab_find(p, pparg[1]);
    SymbolId op2_id = symtab_find(p, pparg[2]);

    cg_cmpsetcc(p, lval_id, op1_id, op2_id, "setl");
}

static INSTRUCTION_HANDLER(cle) {
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
    SymbolId lval_id = symtab_find(p, pparg[0]);
    SymbolId op1_id = symtab_find(p, pparg[1]);
    SymbolId op2_id = symtab_find(p, pparg[2]);

    cg_cmpsetcc(p, lval_id, op1_id, op2_id, "setle");
}

static INSTRUCTION_HANDLER(cne) {
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
    SymbolId lval_id = symtab_find(p, pparg[0]);
    SymbolId op1_id = symtab_find(p, pparg[1]);
    SymbolId op2_id = symtab_find(p, pparg[2]);

    cg_cmpsetcc(p, lval_id, op1_id, op2_id, "setne");
}

static INSTRUCTION_HANDLER(def) {
    if (arg_count != 1) {
        parser_set_error(p, ec_badargs);
        return;
    }

    char type[MAX_ARG_LEN];
    char name[MAX_ARG_LEN];
    cg_extract_param(pparg[0], type, name);

    Symbol* sym = symtab_get(p, symtab_add(p, type_from_str(type), name));
    p->stack_bytes += symbol_bytes(sym);
}

static INSTRUCTION_HANDLER(div) {
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
    /* Division needs special handling, since it behaves differently
       Dividend in ax, Result in ax */
    SymbolId lval_id = symtab_find(p, pparg[0]);
    SymbolId op1_id = symtab_find(p, pparg[1]);
    SymbolId op2_id = symtab_find(p, pparg[2]);
    cg_validate_equal_size3(p, lval_id, op1_id, op2_id);

    cg_divide(p, op1_id, op2_id, "idiv");
    cg_mov_fromr(p, reg_a, lval_id);
}

static INSTRUCTION_HANDLER(func) {
    if (arg_count < 2) {
        parser_set_error(p, ec_badargs);
        goto exit;
    }

    /* Special handling of main function */
    if (strequ(pparg[0], "main")) {
        if (arg_count != 4) {
            parser_set_error(p, ec_badmain);
            goto exit;
        }

        if (parser_has_error(p)) goto exit;
        char type[MAX_ARG_LEN];
        char name[MAX_ARG_LEN];

        /* Symbol argc */
        cg_extract_param(pparg[2], type, name);
        SymbolId argc_id = symtab_add(p, type_from_str(type), name);
        if (parser_has_error(p)) goto exit;

        /* Symbol argv */
        cg_extract_param(pparg[3], type, name);
        SymbolId argv_id = symtab_add(p, type_from_str(type), name);
        if (parser_has_error(p)) goto exit;

        /* Wait on obtaining pointers until all symbols added
           as add may invalidate pointer */
        symtab_get(p, argc_id)->reg = reg_edi;
        symtab_get(p, argv_id)->reg = reg_esi;

        /* Generate a _start function which calls main */
        parser_output_asm(p, "%s",
            "\n"
            "    global _start\n"
            "_start:\n"
            "    mov             rdi, QWORD [rsp]\n" /* argc */
            "    mov             rsi, QWORD [rsp+8]\n" /* argv */
            "    call            f@main\n"
            "    mov             rdi, rax\n" /* exit call */
            "    mov             rax, 60\n"
            "    syscall\n"
        );
        if (parser_has_error(p)) goto exit;
    }

    /* Function labels always with prefix f@ */
    parser_output_asm(p,
        "f@%s:\n"
        /* Function prologue */
        "push rbp\n"
        "mov rbp,rsp\n",
        pparg[0]
    );
    /* Reserve stack space */
    if (p->stack_bytes != 0) {
        parser_output_asm(p, "sub rsp,%d\n", p->stack_bytes);
    }
exit:
    return;
}

static INSTRUCTION_HANDLER(jmp) {
    if (arg_count != 1) {
        parser_set_error(p, ec_badargs);
        return;
    }

    SymbolId label_id = symtab_find(p, pparg[0]);
    Symbol* label = symtab_get(p, label_id);

    parser_output_asm(p, "jmp %s\n", symbol_name(label));
}

static INSTRUCTION_HANDLER(jnz) {
    if (arg_count != 2) {
        parser_set_error(p, ec_badargs);
        return;
    }

    SymbolId label_id = symtab_find(p, pparg[0]);
    SymbolId op1_id = symtab_find(p, pparg[1]);

    cg_testjmpcc(p, label_id, op1_id, "jnz");
}

static INSTRUCTION_HANDLER(jz) {
    if (arg_count != 2) {
        parser_set_error(p, ec_badargs);
        return;
    }

    SymbolId label_id = symtab_find(p, pparg[0]);
    SymbolId op1_id = symtab_find(p, pparg[1]);

    cg_testjmpcc(p, label_id, op1_id, "jz");
}

static INSTRUCTION_HANDLER(lab) {
    if (arg_count != 1) {
        parser_set_error(p, ec_badargs);
        return;
    }
    parser_output_asm(p, "%s:\n", pparg[0]);
}

static INSTRUCTION_HANDLER(mod) {
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }

    /* Mod is division but returning different part of result
       Dividend in ax, Remainder in dx */
    SymbolId lval_id = symtab_find(p, pparg[0]);
    SymbolId op1_id = symtab_find(p, pparg[1]);
    SymbolId op2_id = symtab_find(p, pparg[2]);
    cg_validate_equal_size3(p, lval_id, op1_id, op2_id);

    cg_divide(p, op1_id, op2_id, "idiv");
    cg_mov_fromr(p, reg_d, lval_id);
}

static INSTRUCTION_HANDLER(mov) {
    if (arg_count != 2) {
        parser_set_error(p, ec_badargs);
        return;
    }

    SymbolId lval_id = symtab_find(p, pparg[0]);
    SymbolId rval_id = symtab_find(p, pparg[1]);
    cg_validate_equal_size2(p, lval_id, rval_id);

    cg_mov_tor(p, rval_id, reg_a);
    cg_mov_fromr(p, reg_a, lval_id);
}

static INSTRUCTION_HANDLER(mul) {
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
    SymbolId lval_id = symtab_find(p, pparg[0]);
    SymbolId op1_id = symtab_find(p, pparg[1]);
    SymbolId op2_id = symtab_find(p, pparg[2]);

    cg_arithmetic(p, lval_id, op1_id, op2_id, "imul");
}

static INSTRUCTION_HANDLER(not) {
    if (arg_count != 2) {
        parser_set_error(p, ec_badargs);
        return;
    }

    SymbolId lval_id = symtab_find(p, pparg[0]);
    SymbolId rval_id = symtab_find(p, pparg[1]);
    cg_validate_equal_size2(p, lval_id, rval_id);

    Symbol* rval = symtab_get(p, rval_id);
    GRegister reg = reg_a; /* Where to store intermediate value */
    int bytes = symbol_bytes(rval);
    const char* reg_name = reg_get_str(reg_a, bytes);
    const char* reg_lower_name = reg_get_str(reg_a, 1);

    cg_mov_tor(p, rval_id, reg);
    parser_output_asm(p, "test %s,%s\n", reg_name, reg_name);
    parser_output_asm(p, "setz %s\n", reg_lower_name);

    /* Setz only sets 1 byte, thus the the remaining register
       has to be cleared to obtain 1 or 0 */
    if (bytes > 1) {
        parser_output_asm(p, "movzx %s,%s\n", reg_name, reg_lower_name);
    }
    cg_mov_fromr(p, reg, lval_id);
}

static INSTRUCTION_HANDLER(ret) {
    if (arg_count != 1) {
        parser_set_error(p, ec_badargs);
        goto exit;
    }

    SymbolId sym_id = symtab_find(p, pparg[0]);
    Symbol* sym = symtab_get(p, sym_id);
    if (sym == NULL) {
        parser_set_error(p, ec_unknownsym);
        goto exit;
    }

    int bytes = symbol_bytes(sym);
    /* Return integer types in rax */
    parser_output_asm(p, "mov %s,", reg_get_str(reg_a, bytes));
    cg_ref_symbol(p, sym_id);
    parser_output_asm(p, "\n");

    /* Function epilogue */
    parser_output_asm(p,
        "leave\n"
        "ret\n"
    );

exit:
    return;
}

static INSTRUCTION_HANDLER(sub) {
    if (arg_count != 3) {
        parser_set_error(p, ec_badargs);
        return;
    }
    SymbolId lval_id = symtab_find(p, pparg[0]);
    SymbolId op1_id = symtab_find(p, pparg[1]);
    SymbolId op2_id = symtab_find(p, pparg[2]);

    cg_arithmetic(p, lval_id, op1_id, op2_id, "sub");
}

/* Generates the necessary instructions to implement each arithmetic operation
   Generates very inefficient code, but goal is to get something functioning
   right now.
   1. Moves both operands into registers
   2. Performs operation using instruction
   3. Moves register where op_1 used to be to destination */
static void cg_arithmetic(
        Parser* p,
        SymbolId dest_id, SymbolId op1_id, SymbolId op2_id, const char* ins) {
    cg_validate_equal_size3(p, dest_id, op1_id, op2_id);

    GRegister op1_reg = reg_a;
    GRegister op2_reg = reg_c;
    Symbol* dest = symtab_get(p, dest_id);
    int bytes = symbol_bytes(dest);

    cg_mov_tor(p, op1_id, op1_reg);
    cg_mov_tor(p, op2_id, op2_reg);
    parser_output_asm(p, "%s %s,%s\n",
        ins,
        reg_get_str(op1_reg, bytes), reg_get_str(op2_reg, bytes));
    cg_mov_fromr(p, op1_reg, dest_id);
}

/* Generates the necessary instructions to implement logical operators:
   1. Moves both operands into registers
   2. Compares (op1 cmp op2)
   3. Performs the indicated conditional set using the results from comparison
   4. Zero extends result (0 or 1) to bytes of destination
   5. Moves the result to destination */
static void cg_cmpsetcc(
        Parser* p,
        SymbolId dest_id, SymbolId op1_id, SymbolId op2_id, const char* set) {
    cg_validate_equal_size3(p, dest_id, op1_id, op2_id);

    GRegister op1_reg = reg_a;
    GRegister op2_reg = reg_c;
    Symbol* dest = symtab_get(p, dest_id);
    int bytes = symbol_bytes(dest);
    const char* op1_reg_name = reg_get_str(op1_reg, bytes);
    const char* op1_reg_lower_name = reg_get_str(op1_reg, 1);

    cg_mov_tor(p, op1_id, op1_reg);
    cg_mov_tor(p, op2_id, op2_reg);
    parser_output_asm(p, "cmp %s,%s\n",
        op1_reg_name, reg_get_str(op2_reg, bytes));
    parser_output_asm(p, "%s %s\n", set, op1_reg_lower_name);
    if (bytes > 1) {
        parser_output_asm(p, "movzx %s,%s\n", op1_reg_name, op1_reg_lower_name);
    }
    cg_mov_fromr(p, op1_reg, dest_id);
}

/* Generates the necessary instructions to implement conditional jump
   1. Moves operand into register
   2. Tests (op1, op1)
   3. Performs the conditional jump to label using the results from test */
static void cg_testjmpcc(
        Parser* p,
        SymbolId label_id, SymbolId op1_id, const char* jmp) {
    GRegister reg = reg_a;
    Symbol* label = symtab_get(p, label_id);
    Symbol* op1 = symtab_get(p, op1_id);
    const char* reg_name = reg_get_str(reg, symbol_bytes(op1));

    cg_mov_tor(p, op1_id, reg);
    parser_output_asm(p, "test %s,%s\n", reg_name, reg_name);
    parser_output_asm(p, "%s %s\n", jmp, symbol_name(label));
}

/* Places op1 and op2 in the appropriate locations for op1 / op2
   ins is the instruction to apply to perform the division */
static void cg_divide(
        Parser* p, SymbolId op1_id, SymbolId op2_id,
        const char* ins) {
    GRegister op2_reg = reg_c;

    Symbol* op1 = symtab_get(p, op1_id);
    int bytes = symbol_bytes(op1);

    cg_mov_tor(p, op1_id, reg_a);
    cg_mov_tor(p, op2_id, op2_reg);
    /* dx has to be zeroed, other it is interpreted as part of dividend */
    parser_output_asm(p, "xor %s,%s\n",
            reg_get_str(reg_d, bytes),
            reg_get_str(reg_d, bytes));

    parser_output_asm(p, "%s %s\n", ins, reg_get_str(op2_reg, bytes));
}

/* Generates code for reg/mem -> reg */
static void cg_mov_tor(Parser* p, SymbolId source, GRegister dest) {
    Symbol* sym = symtab_get(p, source);
    int bytes = symbol_bytes(sym);

    parser_output_asm(p, "mov %s,", reg_get_str(dest, bytes));
    cg_ref_symbol(p, source);
    parser_output_asm(p, "\n");
}

/* Generates code for reg -> reg/mem */
static void cg_mov_fromr(Parser* p, GRegister source, SymbolId dest) {
    Symbol* sym = symtab_get(p, dest);
    int bytes = symbol_bytes(sym);

    parser_output_asm(p, "mov ");
    cg_ref_symbol(p, dest);
    parser_output_asm(p, ",%s\n", reg_get_str(source, bytes));
}

/* Generates assembly code to reference symbol
   Example: "dword [rbp+10]" */
static void cg_ref_symbol(Parser* p, SymbolId sym_id) {
    Symbol* sym = symtab_get(p, sym_id);

    if (symbol_on_stack(sym)) {
        char sign = '+';
        int abs_offset = symtab_get_offset(p, sym_id);
        if (abs_offset < 0) {
            sign = '-';
            abs_offset = -abs_offset;
        }
        const char* size_dir = asm_size_directive(symbol_bytes(sym));
        parser_output_asm(p, "%s [rbp%c%d]", size_dir, sign, abs_offset);
    }
    else if (symbol_is_constant(sym)) {
        parser_output_asm(p, "%s", symbol_name(sym));
    }
    else {
        /* Symbol in register */
        parser_output_asm(p, "%s", reg_str(symbol_location(sym)));
    }
}

/* Extracts the type and the name from str into name and type
   Assumes type and name is of sufficient size */
static void cg_extract_param(const char* str, char* type, char* name) {
    char c;
    int i = 0;
    /* Extract type */
    {
        while ((c = str[i]) != ' ' && c != '\0') {
            type[i] = str[i];
            ++i;
        }
        type[i] = '\0';
    }
    /* Extract name */
    {
        if (c == '\0') { /* No name provided */
            name[0] = '\0';
            return;
        }
        ++i; /* Skip the space which was encountered */
        int i_out = 0;
        while ((c = str[i]) != '\0') {
            name[i_out] = str[i];
            ++i;
            ++i_out;
        }
        name[i_out] = '\0';
    }
}

/* Asserts the given symbols are the same size */
static void cg_validate_equal_size2(Parser* p,
        SymbolId lval_id, SymbolId rval_id) {
    Symbol* lval = symtab_get(p, lval_id);
    Symbol* rval = symtab_get(p, rval_id);
    ASSERT(symbol_bytes(lval) == symbol_bytes(rval), "Non equal type sizes");
}

/* Asserts the given symbols are the same size */
static void cg_validate_equal_size3(Parser* p,
        SymbolId lval_id, SymbolId op1_id, SymbolId op2_id) {
    Symbol* lval = symtab_get(p, lval_id);
    Symbol* op1 = symtab_get(p, op1_id);
    Symbol* op2 = symtab_get(p, op2_id);
    ASSERT(symbol_bytes(lval) == symbol_bytes(op1), "Non equal type sizes");
    ASSERT(symbol_bytes(op1) == symbol_bytes(op2), "Non equal type sizes");
}

/* Index of string is instruction handler index */
#define INSTRUCTION(name__) #name__,
const char* instruction_handler_index[] = {INSTRUCTIONS};
#undef INSTRUCTION
#define INSTRUCTION(name__) &handler_ ## name__,
const InstructionHandler instruction_handler_table[] = {INSTRUCTIONS};
#undef INSTRUCTION

/* Repositions file to begin reading from the beginning */
static void seek_to_start(Parser* p) {
    if (fseek(p->rf, 0, SEEK_SET) != 0) {
        parser_set_error(p, ec_seekfailed);
        return;
    }
}

/* Reads one instruction and its arguments,
   the result is stored in the parser
   Returns 1 if successfully read, 0 if EOF or error */
static int read_instruction(Parser* p) {
    /* Read instruction */
    p->ins_len = 0;
    char c;
    while(1) {
        c = getc(p->rf);
        if (c == EOF) {
            p->ins[p->ins_len] = '\0';
            return 0;
        }
        if (c == '\n') {
            p->ins[p->ins_len] = '\0';
            return 1;
        }
        if (c == ' ') {
            p->ins[p->ins_len] = '\0';
            break;
        }
        /* Also need space for null terminator */
        if (p->ins_len >= MAX_INSTRUCTION_LEN - 1) {
            p->ins[p->ins_len] = '\0';
            parser_set_error(p, ec_insbufexceed);
            return 0;
        }
        p->ins[p->ins_len] = c;
        ++p->ins_len;
    }

    /* Read argument
       Arguments are stored together in one buffer
       With \0 between them, arg_table[i] points to the beginning
       of each argument */
    /* Add argument to argument list upon seeing its beginning,
       guaranteed one argument since space seen */
    p->arg_count = 0;
    if (p->arg_count >= MAX_ARGS) {
        parser_set_error(p, ec_argbufexceed);
        return 0;
    }
    p->arg_table[0] = p->arg;
    ++p->arg_count;

    int i = 0;
    while(1) {
        c = getc(p->rf);
        if (c == EOF) {
            p->arg[i] = '\0';
            return 0;
        }
        if (c == '\n') {
            p->arg[i] = '\0';
            return 1;
        }

        if (c == ',') {
            p->arg[i] = '\0';
            if (p->arg_count >= MAX_ARGS) {
                parser_set_error(p, ec_argbufexceed);
                return 0;
            }
            p->arg_table[p->arg_count] = p->arg + i + 1;
            ++p->arg_count;
        }
        else {
            /* -1 as also need space for null terminator */
            if (i >= MAX_ARG_LEN - 1) {
                parser_set_error(p, ec_argbufexceed);
                p->arg[i] = '\0';
                return 0;
            }
            p->arg[i] = c;
        }
        ++i;
    }
}

static void parse(Parser* p) {
    /* Pass one */
    while (read_instruction(p)) {
        /* Maybe think of a better way to implement
           def such that:
           def runs in pass 1
           Pass 2 does not mark def as an error */
        if (strequ(p->ins, "def")) {
            handler_def(p, p->arg_table, p->arg_count);
        }
        if (parser_has_error(p)) return;
    }
    /* Pass two */
    seek_to_start(p);
    if (parser_has_error(p)) return;
    while (read_instruction(p)) {
        if (strequ(p->ins, "def")) {
            continue;
        }

        parser_output_comment(p, "%s", p->ins);
        if (0 < p->arg_count) {
            parser_output_asm(p, " %s", p->arg_table[0]);
        }
        for (int i = 1; i < p->arg_count; ++i) {
            parser_output_asm(p, ",%s", p->arg_table[i]);
        }
        parser_output_asm(p, "\n");

        /* Run handler for instruction */
        int i_handler = strbinfind(
                p->ins,
                p->ins_len,
                instruction_handler_index,
                ARRAY_SIZE(instruction_handler_index));
        if (i_handler < 0) {
            ERRMSGF("Unrecognized instruction %s\n", p->ins);
            parser_set_error(p, ec_invalidins);
            return;
        }
        instruction_handler_table[i_handler](p, p->arg_table, p->arg_count);
        if (parser_has_error(p)) return;
    }
}

/* Parses cli args and processes them */
/* NOTE: will not clean up file handles at exit */
/* Returns non zero if error */
static int handle_cli_arg(Parser* p, int argc, char** argv) {
    int rt_code = 0;
    /* Skip first argv since it is path */
    for (int i = 1; i < argc; ++i) {
        if (strequ(argv[i], "-o")) {
            if (p->of != NULL) {
                ERRMSG("Only one output file can be specified\n");
                rt_code = 1;
                break;
            }

            ++i;
            if (i >= argc) {
                ERRMSG("Expected output file path after -o\n");
                rt_code = 1;
                break;
            }
            p->of = fopen(argv[i], "w");
            if (p->of == NULL) {
                ERRMSGF("Failed to open output file" TOKEN_COLOR " %s\n", argv[i]);
                rt_code = 1;
                break;
            }
        }
        else if (strequ(argv[i], "-Zd4")) {
            g_debug_print_buffers = 1;
        }
        else {
            if (p->rf != NULL) {
                /* Maybe user meant for additional arg to go with flag */
                ERRMSGF("Unrecognized argument" TOKEN_COLOR " %s\n", argv[i]);
                rt_code = 1;
                break;
            }
            p->rf = fopen(argv[i], "r");
            if (p->rf == NULL) {
                ERRMSGF("Failed to open input file" TOKEN_COLOR " %s\n", argv[i]);
                rt_code = 1;
                break;
            }
        }
    }

    return rt_code;
}


int main(int argc, char** argv) {
    int exitcode = 0;

    Parser p = {.rf = NULL, .of = NULL};
    parser_construct(&p);

    exitcode = handle_cli_arg(&p, argc, argv);
    if (exitcode != 0) {
        goto exit;
    }
    if (p.rf == NULL) {
        ERRMSG("No input file\n");
        goto exit;
    }
    if (p.of == NULL) {
        /* Default to opening imm3 */
        p.of = fopen("imm3", "w");
        if (p.of == NULL) {
            ERRMSG("Failed to open output file\n");
            exitcode = 1;
            goto exit;
        }
    }

    parse(&p);

exit:
    /* Indicate to the user cause for exiting if errored during parsing */
    if (parser_has_error(&p)) {
        ErrorCode ecode = parser_get_error(&p);
        ERRMSGF("Error during parsing: %d %s\n", ecode, errcode_str[ecode]);
        exitcode = ecode;
    }
    if (g_debug_print_buffers) {
        debug_dump(&p);
    }

    if (p.rf != NULL) {
        fclose(p.rf);
    }
    if (p.of != NULL) {
        fclose(p.of);
    }
    parser_destruct(&p);
    return exitcode;
}

