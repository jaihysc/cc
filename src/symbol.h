/* Represents a symbol in the program */
#ifndef SYMBOL_H
#define SYMBOL_H

#include "errorcode.h"
#include "type.h"

#define MAX_SYMBOL_LEN 255 /* Max characters symbol name (exclude null terminator) */

typedef struct {
    int scope; /* Index of scope */
    int index; /* Index of symbol in scope */
} SymbolId;

/* Used to indicate invalid symbol */
extern SymbolId symid_invalid;

/* Return 1 if the SymbolId is valid, 0 otherwise */
int symid_valid(SymbolId sym_id);

/* Returns 1 if both symid are equal, 0 otherwise */
int symid_equal(SymbolId a, SymbolId b);

typedef enum {
    sl_normal = 0,
    sl_access /* Represents access to memory location */
} SymbolClass;

typedef enum {
    vc_lval = 0,
    vc_nlval
} ValueCategory;

typedef struct {
    SymbolClass class;
    char token[MAX_SYMBOL_LEN + 1];
    Type type;
    ValueCategory valcat;
    int scope_num; /* Guaranteed to be unique for each scope */

    /* Only for class sl_access */
    SymbolId ptr;
    SymbolId ptr_idx;
} Symbol;

/* Creates symbol at given memory location */
ErrorCode symbol_construct(
        Symbol* sym,
        const char* token,
        Type type,
        ValueCategory valcat,
        int scope_num);

/* Converts symbol to class representing access to memory location
   ptr: Is a symbol which when indexed yields this symbol
   idx: Is a symbol which indexes into ptr to yield this
        symbol, leave as symid_invalid to default to 0
   e.g., int* p; int a = p[2];
   If this symbol is a, ptr is p, idx is 2 */
void symbol_sl_access(Symbol* sym, SymbolId ptr, SymbolId idx);

/* Returns SymbolClass for symbol */
SymbolClass symbol_class(Symbol* sym);

/* Returns token for symbol */
char* symbol_token(Symbol* sym);

/* Returns type for symbol */
Type symbol_type(Symbol* sym);

/* Sets type for symbol */
void symbol_set_type(Symbol* sym, const Type* type);

/* Returns ValueCategory for symbol */
ValueCategory symbol_valcat(Symbol* sym);

/* Returns number guaranteed to be unique for each scope */
int symbol_scope_num(Symbol* sym);

/* Returns the symbol for the pointer, which when indexed yields this symbol
   e.g., int* p; int a = p[2];
   If this symbol is a, the returned symbol is p */
SymbolId symbol_ptr_sym(Symbol* sym);

/* Returns the symbol of the index, which when used to index symbol_ptr_sym
   yields this symbol
   The index is always in bytes */
SymbolId symbol_ptr_index(Symbol* sym);

/* Sorted by Annex A */
/* 6.4 Lexical elements */
/* 6.5 Expressions */
/* 6.7 Declarators */
/* 6.8 Statements and blocks */
/* 6.9 External definitions */
#define SYMBOL_TYPES                       \
    SYMBOL_TYPE(root)                      \
                                           \
    SYMBOL_TYPE(identifier)                \
    SYMBOL_TYPE(constant)                  \
    SYMBOL_TYPE(integer_constant)          \
    SYMBOL_TYPE(decimal_constant)          \
    SYMBOL_TYPE(octal_constant)            \
    SYMBOL_TYPE(hexadecimal_constant)      \
                                           \
    SYMBOL_TYPE(primary_expression)        \
    SYMBOL_TYPE(postfix_expression)        \
    SYMBOL_TYPE(argument_expression_list)  \
    SYMBOL_TYPE(unary_expression)          \
    SYMBOL_TYPE(cast_expression)           \
    SYMBOL_TYPE(multiplicative_expression) \
    SYMBOL_TYPE(additive_expression)       \
    SYMBOL_TYPE(shift_expression)          \
    SYMBOL_TYPE(relational_expression)     \
    SYMBOL_TYPE(equality_expression)       \
    SYMBOL_TYPE(and_expression)            \
    SYMBOL_TYPE(exclusive_or_expression)   \
    SYMBOL_TYPE(inclusive_or_expression)   \
    SYMBOL_TYPE(logical_and_expression)    \
    SYMBOL_TYPE(logical_or_expression)     \
    SYMBOL_TYPE(conditional_expression)    \
    SYMBOL_TYPE(assignment_expression)     \
    SYMBOL_TYPE(expression)                \
                                           \
    SYMBOL_TYPE(declaration)               \
    SYMBOL_TYPE(declaration_specifiers)    \
    SYMBOL_TYPE(init_declarator_list)      \
    SYMBOL_TYPE(init_declarator)           \
    SYMBOL_TYPE(storage_class_specifier)   \
    SYMBOL_TYPE(type_specifier)            \
    SYMBOL_TYPE(specifier_qualifier_list)  \
    SYMBOL_TYPE(type_qualifier)            \
    SYMBOL_TYPE(function_specifier)        \
    SYMBOL_TYPE(declarator)                \
    SYMBOL_TYPE(direct_declarator)         \
    SYMBOL_TYPE(pointer)                   \
    SYMBOL_TYPE(parameter_type_list)       \
    SYMBOL_TYPE(parameter_list)            \
    SYMBOL_TYPE(parameter_declaration)     \
    SYMBOL_TYPE(type_name)                 \
    SYMBOL_TYPE(abstract_declarator)       \
    SYMBOL_TYPE(initializer)               \
                                           \
    SYMBOL_TYPE(statement)                 \
    SYMBOL_TYPE(compound_statement)        \
    SYMBOL_TYPE(block_item)                \
    SYMBOL_TYPE(expression_statement)      \
    SYMBOL_TYPE(selection_statement)       \
    SYMBOL_TYPE(iteration_statement)       \
    SYMBOL_TYPE(jump_statement)            \
                                           \
    SYMBOL_TYPE(function_definition)

#define SYMBOL_TYPE(name__) st_ ## name__,
/* st_ force compiler to choose data type with negative values
   as negative values indicate a token is stored */
typedef enum { st_= -1, SYMBOL_TYPES} SymbolType;
#undef SYMBOL_TYPE

/* Converts SymbolType to string */
const char* st_str(SymbolType st);

#endif
