/* Preprocessed c file parser */
/* Generated output is imm2 */

#include <stdio.h>
#include <stdarg.h>
#include "common.h"

#define MAX_TOKEN_LEN 255 /* Excluding null terminator, Tokens is string with no whitespace */
#define MAX_PARSE_TREE_NODE 2000 /* Maximum nodes in parser parse tree */
#define MAX_PARSE_NODE_CHILD 4 /* Maximum children nodes for a parse tree node */
#define MAX_TOKEN_BUFFER_CHAR 8192 /* Max characters in token buffer */
#define MAX_SCOPES 32 /* Max number of scopes */
#define MAX_SCOPE_LEN 500   /* Max symbols per scope */

/* ============================================================ */
/* Parser global configuration */

int g_debug_print_buffers = 0;
int g_debug_print_cg_recursion = 0;
int g_debug_print_parse_recursion = 0;
int g_debug_print_parse_tree = 0;
int g_debug_print_symtab = 0;

/* ============================================================ */
/* Parser data structure + functions */

typedef int TokenId;
typedef struct {
    int scope; /* Index of scope */
    int index; /* Index of symbol in scope */

} SymbolId;

/* Used to indicate invalid symbol */
SymbolId symid_invalid = {.index = -1};

/* Return 1 if the SymbolId is valid, 0 otherwise */
static int symid_valid(SymbolId sym_id) {
    return sym_id.index != -1;
}

typedef struct Parser Parser;
static char* parser_get_token(Parser* p, TokenId id);

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
    TokenId tok_id;
    Type type;
    ValueCategory valcat;
    int scope_num; /* Guaranteed to be unique for each scope */

    /* Only for class sl_access */
    SymbolId ptr;
    SymbolId ptr_idx;
} Symbol;

/* Creates symbol at given memory location */
static void symbol_construct(
        Symbol* sym,
        TokenId tok_id,
        Type type,
        ValueCategory valcat,
        int scope_num) {
    sym->tok_id = tok_id;
    sym->type = type;
    sym->valcat = valcat;
    sym->scope_num = scope_num;
}

/* Converts symbol to class representing access to memory location
   ptr: Is a symbol which when indexed yields this symbol
   idx: Is a symbol which indexes into ptr to yield this
        symbol, leave as symid_invalid to default to 0
   e.g., int* p; int a = p[2];
   If this symbol is a, ptr is p, idx is 2 */
static void symbol_sl_access(Symbol* sym, SymbolId ptr, SymbolId idx) {
    ASSERT(sym != NULL, "Symbol is null");
    sym->class = sl_access;
    sym->ptr = ptr;
    sym->ptr_idx = idx;
}

/* Returns SymbolClass for symbol */
static SymbolClass symbol_class(Symbol* sym) {
    return sym->class;
}

/* Returns token for symbol */
static char* symbol_token(Parser* p, Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return parser_get_token(p, sym->tok_id);
}

/* Returns type for symbol */
static Type symbol_type(Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->type;
}

/* Returns ValueCategory for symbol */
static ValueCategory symbol_valcat(Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->valcat;
}

/* Returns number guaranteed to be unique for each scope */
static int symbol_scope_num(Symbol* sym) {
    return sym->scope_num;
}

/* Returns the symbol for the pointer, which when indexed yields this symbol
   e.g., int* p; int a = p[2];
   If this symbol is a, the returned symbol is p */
static SymbolId symbol_ptr_sym(Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->ptr;
}

/* Returns the symbol of the index, which when used to index symbol_ptr_sym
   yields this symbol
   The index is always in bytes */
static SymbolId symbol_ptr_index(Symbol* sym) {
    ASSERT(sym != NULL, "Symbol is null");
    return sym->ptr_idx;
}

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
/* Maps symbol type to string (index with symbol type) */
#define SYMBOL_TYPE(name__) #name__,
const char* symbol_type_str[] = {SYMBOL_TYPES};
#undef SYMBOL_TYPES

/* Convert from an index into token buffer to symbol type */
static SymbolType st_from_token_id(int index) {
    return -(index + 1);
}

/* Convert from a symbol type to an index into token buffer */
static TokenId st_to_token_id(SymbolType type) {
    return -type - 1;
}

/* SymbolCat: The category(purpose) a symbol serves
   e.g., label for end of loop body */
typedef enum {
    /* Label placed right before condition check to loop loop/exit */
    sc_lab_loopbodyend = 0,
    /* Label placed after loop */
    sc_lab_loopend,
    sc_count
} SymbolCat;

typedef struct ParseNode {
    struct ParseNode* child[MAX_PARSE_NODE_CHILD];
    SymbolType type;
} ParseNode;

/* Counts number of children for given node */
static int parse_node_count_child(ParseNode* node) {
    ASSERT(node != NULL, "Node is null");

    int count = 0;
    while (count < MAX_PARSE_NODE_CHILD) {
        if (node->child[count] == NULL) {
            break;
        }
        ++count;
    }
    return count;
}

/* Retrieves the child at index for node
   negative to index backwards (-1 means last child, -2 second last) */
static ParseNode* parse_node_child(ParseNode* node, int i) {
    int childs = parse_node_count_child(node);

    if (i >= 0) {
        ASSERT(i < MAX_PARSE_NODE_CHILD, "Child index out of range");
        return node->child[i];
    }

    /* Index backwads */
    ASSERT(childs + i >= 0, "Child reverse index out of range");
    return node->child[childs + i];
}

/* Retrieves the symbol type for node */
static SymbolType parse_node_type(ParseNode* node) {
    ASSERT(node != NULL, "Node is null");
    return node->type;
}

/* Retrieves token id for node */
static TokenId parse_node_token_id(ParseNode* node) {
    return st_to_token_id(node->type);
}

/* Retrieves token id for node */
static const char* parse_node_token(Parser* p, ParseNode* node) {
    return parser_get_token(p, parse_node_token_id(node));
}

typedef struct {
    /* Remember line, char in source file */
    int line_num;
    int char_num;
    /* Index of next available space in parse node buffer */
    int i_node_buf;
    ParseNode* node; /* Node which may be modified */
    int node_childs; /* Children the node has */
    long rf_offset; /* Input file offset */
    char read_buf[MAX_TOKEN_LEN + 1]; /* Token read buffer */
} ParseLocation;

/* pbufexceed: Parser buffer exceeded
   fileposfailed: Change file position indicator failed */
#define ERROR_CODES                  \
    ERROR_CODE(noerr)                \
    ERROR_CODE(tokbufexceed)         \
    ERROR_CODE(pbufexceed)           \
    ERROR_CODE(pnodechildexceed)     \
    ERROR_CODE(ptokbufexceed)        \
    ERROR_CODE(scopedepexceed)       \
    ERROR_CODE(scopelenexceed)       \
    ERROR_CODE(syntaxerr)            \
    ERROR_CODE(writefailed)          \
    ERROR_CODE(fileposfailed)

/* Should always be initialized to ec_noerr */
/* Since functions will only sets if error occurred */
#define ERROR_CODE(name__) ec_ ## name__,
typedef enum {ERROR_CODES} ErrorCode;
#undef ERROR_CODE
#define ERROR_CODE(name__) #name__,
char* errcode_str[] = {ERROR_CODES};
#undef ERROR_CODE
#undef ERROR_CODES

struct Parser {
    FILE* rf; /* Input file */
    FILE* of; /* Output file */

    /* Functions set error code only if an error occurred */
    /* By default this is set as ec_noerr */
    ErrorCode ecode;

    /* Tracks position within input file for error messages */
    int line_num;
    int char_num;
    /* Last position within input file where production was matched */
    int last_line_num;
    int last_char_num;

    /* Token read buffer, used by read_token to store token read */
    /* The read token is kept here, subsequent calls to read_token()
       will return this.
       When consume_token() is called, the next call to read_token()
       will fill this buffer with a new token
       */
    char read_buf[MAX_TOKEN_LEN + 1];

    /* Ensure root above node buffer for pointer arithmetic to work
       (e.g., calculating index of node). Root treated as index -1
       | root | node1 | node 2 | ... */
    ParseNode parse_node_root;
    ParseNode parse_node_buf[MAX_PARSE_TREE_NODE];
    int i_parse_node_buf; /* Index one past last element */

    /* In future: Could save space by seeing if the token is already in buf */
    char token_buf[MAX_TOKEN_BUFFER_CHAR];
    TokenId i_token_buf; /* Index one past last element */

    /* First scope is most global
       First symbol element is earliest in occurrence
       Constants always use the first scope (at index 0) */
    Symbol symtab[MAX_SCOPES][MAX_SCOPE_LEN];
    int i_scope; /* Index one past end of latest(deepest) scope. */
    int i_symbol[MAX_SCOPES]; /* Index one past last element for each scope */
    /* Number to uniquely identify every scope created */
    int symtab_scope_num;
    /* Number to create unique temporary/label/etc values */
    int symtab_temp_num;
    int symtab_label_num;

    /* Stack for symbol category, push and pop with
       symtab_push_cat()
       symtab_pop_cat()
       Use MAX_SCOPES as maximum for now, only need to push at most once
       per scope */
    SymbolId symtab_cat[sc_count][MAX_SCOPES];
    int i_symtab_cat[sc_count]; /* Points one last last element */
};

/* Pushes symbol onto symbol category stack */
static void symtab_push_cat(Parser* p, SymbolCat cat, SymbolId sym) {
    if (p->i_symtab_cat[cat] >= MAX_SCOPES) {
        ASSERTF(0, "Pushed too many symbol for symbol category %d", cat);
    }
    int i = p->i_symtab_cat[cat];
    ++p->i_symtab_cat[cat];
    p->symtab_cat[cat][i] = sym;
}

/* Pops symbol from top of symbol category stack */
static void symtab_pop_cat(Parser* p, SymbolCat cat) {
    ASSERTF(p->i_symtab_cat[cat] > 0,
            "No symbol in symbol category %d to pop", cat);
    --p->i_symtab_cat[cat];
}

/* Returns the last symbol for symbol category (top of stack) */
static SymbolId symtab_last_cat(Parser* p, SymbolCat cat) {
    ASSERTF(p->i_symtab_cat[cat] > 0,
            "No symbol on symbol category %d stack", cat);
    return p->symtab_cat[cat][p->i_symtab_cat[cat] - 1];
}

static void debug_print_buffers(Parser* p);
static void debug_print_parse_tree(Parser* p);
static void debug_parse_node_walk(
        Parser* p, ParseNode* node,
        char* branch, int i_branch, const int max_branch_len);

/* Sets error code in parser */
static void parser_set_error(Parser* p, ErrorCode ecode) {
    p->ecode = ecode;
    LOGF("Error set: %d %s\n", ecode, errcode_str[ecode]);
}

/* Returns error code in parser */
static ErrorCode parser_get_error(Parser* p) {
    return p->ecode;
}

/* Returns 1 if error is set, 0 otherwise */
static int parser_has_error(Parser* p) {
    return p->ecode != ec_noerr;
}

/* Returns token at given index */
static char* parser_get_token(Parser* p, TokenId id) {
    ASSERT(id < p->i_token_buf,
           "Parse token buffer index out of range");
    return p->token_buf + id;
}

/* Add token to token buffer */
static TokenId parser_add_token(Parser* p, const char* token) {
    int token_start = p->i_token_buf;
    /* + 1 as a null terminator has to be inserted */
    if (p->i_token_buf + strlength(token) + 1 > MAX_TOKEN_BUFFER_CHAR) {
        parser_set_error(p, ec_ptokbufexceed);
        return -1;
    }
    int i = 0;
    while (token[i] != '\0') {
        p->token_buf[p->i_token_buf++] = token[i++];
    }
    p->token_buf[p->i_token_buf++] = '\0';

    return token_start;
}

/* Returns information about the current location in parse tree
   parent is the node which may be modified and thus have to rollback */
static ParseLocation parser_get_parse_location(Parser* p, ParseNode* parent) {
    ASSERT(parent != NULL, "Parent node is null");

    // TODO token buffer is not reset when backtrack, this may be a problem

    /* Current file position */
    long offset = ftell(p->rf);
    if (offset == -1L) {
        parser_set_error(p, ec_fileposfailed);
    }

    ParseLocation location = {
        .line_num = p->line_num,
        .char_num = p->char_num,
        .i_node_buf = p->i_parse_node_buf,
        .node = parent,
        .node_childs = parse_node_count_child(parent),
        .rf_offset = offset
    };

    /* Copy over token read buffer */
    strcopy(p->read_buf, location.read_buf);

    return location;
}

/* Sets the current location in the parse tree */
static void parser_set_parse_location(Parser* p, ParseLocation* location) {
    p->line_num = location->line_num;
    p->char_num = location->char_num;

    p->i_parse_node_buf = location->i_node_buf;

    /* Remove excess children */
    for (int i = location->node_childs; i < MAX_PARSE_NODE_CHILD; ++i) {
        location->node->child[i] = NULL;
    }

    /* Return to old file position */
    if (fseek(p->rf, location->rf_offset, SEEK_SET) != 0) {
        parser_set_error(p, ec_fileposfailed);
        return;
    }

    /* Return to old token read buffer */
    strcopy(location->read_buf, p->read_buf);
}

/* Attaches a node of type st onto the provided parent node
   Returns the attached node */
static ParseNode* tree_attach_node(Parser* p, ParseNode* parent, SymbolType st) {
    ASSERT(parent != NULL, "Parse tree parent node is null");

    if (p->i_parse_node_buf >= MAX_PARSE_TREE_NODE) {
        parser_set_error(p, ec_pbufexceed);
        return NULL;
    }

    ParseNode* node = p->parse_node_buf + p->i_parse_node_buf;
    ++p->i_parse_node_buf;
    node->type = st;

    /* Zero out children (may have previous values) */
    for (int i = 0; i < MAX_PARSE_NODE_CHILD; ++i) {
        node->child[i] = NULL;
    }

    /* Link parent node to child */
    int i = 0;
    while (1) {
        if (i >= MAX_PARSE_NODE_CHILD) {
            parser_set_error(p, ec_pnodechildexceed);
            return NULL;
        }
        if (parent->child[i] == NULL) {
            parent->child[i] = node;
            break;
        }
        ++i;
    }

    return node;
}

/* Attaches a token node onto the provided parent node
   Returns the attached node */
static ParseNode* tree_attach_token(Parser* p, ParseNode* parent, const char* token) {
    ASSERT(parent != NULL, "Parse tree parent node is null");

    TokenId tok_id = parser_add_token(p, token);
    if (parser_has_error(p)) {
        return NULL;
    }
    return tree_attach_node(p, parent, st_from_token_id(tok_id));
}

/* Removes the children of node from the parse tree */
static void tree_detach_node_child(Parser* p, ParseNode* node) {
    ASSERT(node != NULL, "Parse node is null");

    /* Remove children */
    for (int i = 0; i < MAX_PARSE_NODE_CHILD; ++i) {
        node->child[i] = NULL;
    }
    /* Free memory of children */
    int i_node = (int)(node - p->parse_node_buf); /* Index current node */
    p->i_parse_node_buf = i_node + 1;
}

/* Prints out contents of parser buffers */
static void debug_print_buffers(Parser* p) {
    LOG("Read buffer:\n");
    LOGF("%s\n", p->read_buf);

    LOG("Token buffer:\n");
    int first_char = 1;
    for (int i = 0; i < p->i_token_buf; ++i) {
        char c = p->token_buf[i];
        /* Number indicates index of first char in each row */
        if (first_char) {
            LOGF("%d ", i);
            first_char = 0;
        }
        if (c == '\0') {
            LOG("\n");
            first_char = 1;
        }
        else {
            LOGF("%c", c);
        }
    }
}

static void debug_print_symtab(Parser* p) {
    LOG("Symbol table:\n");
    LOGF("Scopes: [%d]\n", p->i_scope);
    for (int i = 0; i < p->i_scope; ++i) {
        LOGF("  %d [%d]\n", i, p->i_symbol[i]);
        for (int j = 0; j < p->i_symbol[i]; ++j) {
            Symbol* sym = p->symtab[i] + j;
            Type type = symbol_type(sym);
            LOGF("    %d %s", j, type_specifiers_str(type.typespec));
            for (int k = 0; k < type.pointers; ++k) {
                LOG("*");
            }
            LOGF(" %s\n", symbol_token(p, sym));
        }
    }
}

static void debug_print_parse_tree(Parser* p) {
    LOG("Parse tree:\n");
    /* 2 characters per node */
    int max_branch = MAX_PARSE_TREE_NODE * 2;

    char branch[max_branch];
    branch[0] = '\0';
    debug_parse_node_walk(p, &p->parse_node_root, branch, 0, max_branch);
}

/* Prints out the parse tree
   branch stores the branch string, e.g., "| |   | "
   i_branch is index of null terminator in branch
   max_branch_len is maximum characters which can be put in branch */
static void debug_parse_node_walk(
        Parser* p, ParseNode* node,
        char* branch, int i_branch, const int max_branch_len) {
    if (node->type >= 0) {
        /* Is symbol type */
        LOGF("%s\n", symbol_type_str[node->type]);
    }
    else {
        /* Is token */
        char* token = p->token_buf + st_to_token_id(parse_node_type(node));
        LOGF("\"%s\"\n", token);
    }

    /* iterate through children */
    int child_count = 0;
    while (1) {
        if (child_count >= MAX_PARSE_NODE_CHILD) {
            break;
        }
        if (node->child[child_count] == NULL) {
            break;
        }
        ++child_count;
    }

    for (int i = 0; i < child_count; ++i) {
        LOG(branch);

        /* Extend branch for to include this node */
        /* Need to add pipe, space, null terminator
           pipe overwrites existing null terminator */
        if (i_branch + 2 >= max_branch_len) {
            LOG("Max depth exceeded\n");
        }
        else {
            if (i == child_count - 1) {
                LOG("└ ");
                branch[i_branch] = ' ';
            }
            else {
                LOG("├ ");
                branch[i_branch] = '|';
            }
            branch[i_branch + 1] = ' ';
            branch[i_branch + 2] = '\0';
            debug_parse_node_walk(
                    p, node->child[i], branch, i_branch + 2, max_branch_len);
        }
        /* Restore original branch */
        branch[i_branch] = '\0';
    }
}

/* Sets up a new symbol scope to be used */
static void symtab_push_scope(Parser* p) {
    if (g_debug_print_parse_recursion) {
        LOGF("Push scope. Depth %d, Number %d\n", p->i_scope, p->symtab_scope_num);
    }

    if (p->i_scope >= MAX_SCOPES) {
        parser_set_error(p, ec_scopedepexceed);
        return;
    }
    /* Scope may have been previously used, reset index for symbol */
    p->i_symbol[p->i_scope] = 0;
    ++p->i_scope;
    ++p->symtab_scope_num;
}

/* Removes current symbol scope, now uses the last scope */
static void symtab_pop_scope(Parser* p) {
    ASSERT(p->i_scope > 0, "No scope after popping first scope");

    if (g_debug_print_symtab) {
        debug_print_symtab(p);
    }

    --p->i_scope;
    if (g_debug_print_parse_recursion) {
        LOGF("Pop scope at depth %d\n", p->i_scope);
    }
    if (g_debug_print_buffers) {
        debug_print_buffers(p);
    }
}


/* Finds provided token within indicated scope
   Returns symid_invalid if not found */
static SymbolId symtab_find_scoped(Parser* p, int i_scope, TokenId tok_id) {
    const char* tok = parser_get_token(p, tok_id);
    for (int i = 0; i < p->i_symbol[i_scope]; ++i) {
        Symbol* sym = p->symtab[i_scope] + i;
        if (strequ(symbol_token(p, sym), tok)) {
            SymbolId id;
            id.scope = i_scope;
            id.index = i;
            return id;
        }
    }
    return symid_invalid;
}

/* Finds provided token in symbol table, closest scope first
   Returns symid_invalid if not found */
static SymbolId symtab_find(Parser* p, TokenId tok_id) {
    for (int i_scope = p->i_scope - 1; i_scope >= 0; --i_scope) {
        SymbolId found_id = symtab_find_scoped(p, i_scope, tok_id);
        if (symid_valid(found_id)) {
            return found_id;
        }
    }
    return symid_invalid;
}

/* Returns symbol at provided SymbolId */
static Symbol* symtab_get(Parser* p, SymbolId sym_id) {
    ASSERT(symid_valid(sym_id), "Invalid symbol id");
    ASSERT(sym_id.index < p->i_symbol[sym_id.scope], "Symbol id out of range");
    return p->symtab[sym_id.scope] + sym_id.index;
}

/* Short forms to reduce typing */

/* Returns token for SymbolId */
static const char* symtab_get_token(Parser* p, SymbolId sym_id) {
    return symbol_token(p, symtab_get(p, sym_id));
}

/* Returns type for SymbolId */
static Type symtab_get_type(Parser* p, SymbolId sym_id) {
    return symbol_type(symtab_get(p, sym_id));
}

/* Returns ValueCategory for SymbolId */
static ValueCategory symtab_get_valcat(Parser* p, SymbolId sym_id) {
    return symbol_valcat(symtab_get(p, sym_id));
}

/* Returns number guaranteed to be unique for each scope for SymbolId*/
static int symtab_get_scope_num(Parser* p, SymbolId sym_id) {
    return symbol_scope_num(symtab_get(p, sym_id));
}

/* Creates symbol with provided information in symbol table
    tok_id: Set negative to create unnamed symbol
   Returns SymbolId of added symbol, or symid_invalid if it already exists */
static SymbolId symtab_add_scoped(Parser* p,
        int i_scope, TokenId tok_id, Type type, ValueCategory valcat) {
    ASSERT(p->i_scope > 0, "Invalid scope");

    if (tok_id >= 0) {
        /* Normal symbols can not have duplicates in same scope */
        if (symid_valid(symtab_find_scoped(p, i_scope, tok_id))) {
            const char* name = parser_get_token(p, tok_id);
            ERRMSGF("Symbol already exists %s\n", name);
            return symid_invalid;
        }
    }

    SymbolId id;
    id.scope = i_scope;
    id.index = p->i_symbol[i_scope];
    if (id.index >= MAX_SCOPE_LEN) {
        parser_set_error(p, ec_scopelenexceed);
        return symid_invalid;
    }

    Symbol* sym = p->symtab[i_scope] + id.index;
    ++p->i_symbol[i_scope];

    /* Indicate global scope for parser_output_il */
    int scope_num;
    if (i_scope == 0) {
        scope_num = 0;
    }
    else {
        scope_num = p->symtab_scope_num;
    }

    symbol_construct(sym, tok_id, type, valcat, scope_num);
    return id;
}

/* Creates symbol with provided information in symbol table
    tok_id: Set negative to create unnamed symbol
    Has special handling for constants, always added to scope index 0
   Returns SymbolId of added symbol, or symid_invalid if it already exists */
static SymbolId symtab_add(
        Parser* p, TokenId tok_id, Type type, ValueCategory valcat) {
    if (tok_id >= 0) {
        const char* name = parser_get_token(p, tok_id);

        /* Special handling for constants: Duplicates can exist */
        if ('0' <= name[0] && name[0] <= '9') {
            SymbolId const_id = symtab_find_scoped(p, 0, tok_id);
            if (symid_valid(const_id)) {
                ASSERT(type_equal(symtab_get_type(p, const_id), type),
                        "Same constant name with different types");
                return const_id;
            }
            /* Use index 0 for adding constants */
            /* Note this is a little wasteful since it is rechecking
               if the symbol exists */
            return symtab_add_scoped(p, 0, tok_id, type, valcat);
        }
    }

    /* Add new symbol to current scope */
    ASSERT(p->i_scope > 0, "No scope exists");
    int curr_scope = p->i_scope - 1;
    return symtab_add_scoped(p, curr_scope, tok_id, type, valcat);
}

/* Creates a new temporary for the current scope in symbol table */
static SymbolId symtab_add_temporary(Parser* p, Type type) {
    AAPPENDI(token, "__t", p->symtab_temp_num);
    ++p->symtab_temp_num;

    TokenId tok_id = parser_add_token(p, token);
    return symtab_add(p, tok_id, type, vc_nlval);
}

/* Creates a new label for the current scope in symbol table */
static SymbolId symtab_add_label(Parser* p) {
    AAPPENDI(token, "__l", p->symtab_label_num);
    ++p->symtab_label_num;

    TokenId tok_id = parser_add_token(p, token);
    ASSERT(p->i_scope >= 2, "No function scope");
    /* Scope at index 1 is function scope */
    return symtab_add_scoped(p, 1, tok_id, type_label, vc_nlval);
}

/* Handles format specifier $d
   Prints int as string
   Returns 1 if successful, 0 if not */
static int parser_output_ild(Parser* p, int i) {
    return fprintf(p->of, "%d", i) >= 0;
}

/* Handles format specifier $i
   Prints char* as string
   Returns 1 if successful, 0 if not */
static int parser_output_ili(Parser* p, const char* str) {
    return fprintf(p->of, "%s", str) >= 0;
}

/* Handles format specifier $s
   Prints Symbol as string
   Returns 1 if successful, 0 if not */
static int parser_output_ils(Parser* p, SymbolId sym_id) {
    int scope_num = symtab_get_scope_num(p, sym_id);
    /* symtab_add sets scope num to 0 for global scope */
    if (scope_num > 0) {
        return fprintf(p->of, "_Z%d%s",
            scope_num, symtab_get_token(p, sym_id)) >= 0;
    }
    else {
        /* No prefix for global scope as constants sit in global scope */
        return fprintf(p->of, "%s", symtab_get_token(p, sym_id)) >= 0;
    }
}

/* Handles format specifier $t
   Prints Symbol Type as string
   Returns 1 if successful, 0 if not */
static int parser_output_ilt(Parser* p, SymbolId sym_id) {
    Type type = symtab_get_type(p, sym_id);
    if (fprintf(p->of, "%s", type_specifiers_str(type_typespec(&type))) < 0)
        goto error;
    for (int i = 0; i < type_pointer(&type); ++i) {
        if (fprintf(p->of, "*") < 0) goto error;
    }
    for (int i = 0; i < type_dimension(&type); ++i) {
        fprintf(p->of, "[%d]", type_dimension_size(&type, i));
    }
    return 1;

error:
    return 0;
}

/* Writes provided IL using CUSTOM format string (See below) and va_args
   into output
   Format string: The format specifier is replaced with a result
   Format specifier | Parameter | Result
    $d                int         Integer as string
    $i                char*       String (Think Instruction)
    $s                SymbolId    Name of symbol
    $t                Type        Type as a string
*/
static void parser_output_il(Parser* p, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    /* Scans format string as follows:
        Prints from fmt until format specifier
        Move fmt to after format specifier
        Continue until null terminator */
    int i = 0;
    char c;
    while ((c = fmt[i]) != '\0') {
        if (c == '$') {
            /* Print string from fmt until current position */
            if (fprintf(p->of, "%.*s", i, fmt) < 0) goto error;
            char format_c = fmt[++i];
            ASSERT(format_c != '\0', "Expected format specifier char");

            /* Reposition to keep scanning */
            fmt += i + 1; /* Char after format_c */
            i = 0;

            int success;
            switch (format_c) {
                case 'd':
                    success = parser_output_ild(p, va_arg(args, int));
                    break;
                case 'i':
                    success = parser_output_ili(p, va_arg(args, char*));
                    break;
                case 's':
                    success = parser_output_ils(p, va_arg(args, SymbolId));
                    break;
                case 't':
                    success = parser_output_ilt(p, va_arg(args, SymbolId));
                    break;
                default:
                    ASSERT(0, "Unrecognized format specifier char");
            }
            if (!success) goto error;
        }
        else {
            ++i;
        }
    }
    /* Print out remaining format string */
    if (fprintf(p->of, "%.*s", i, fmt) < 0) goto error;

    goto exit;

error:
    parser_set_error(p, ec_writefailed);

exit:
    va_end(args);
}

/* ============================================================ */
/* Token handling */

/* Returns 1 is considered whitespace */
static int iswhitespace(char c) {
    switch (c) {
        case ' ':
        case '\t':
        case '\n':
            return 1;
        default:
            return 0;
    }
}

/* If character is part of a punctuator */
static int isofpunctuator(char c) {
    switch (c) {
        case '[': case ']': case '(': case ')': case '{': case '}':
        case '.': case '-': case '+': case '&': case '*': case '~':
        case '!': case '/': case '%': case '<': case '>': case '=':
        case '^': case '|': case '?': case ':': case ';': case ',':
        case '#':
            return 1;
        default:
            return 0;
    }
}

/* C keyword handling */

/* Returns 1 if string is considered as a keyword, 0 otherwise */
static int tok_iskeyword(const char* token) {
    const char* token_keyword[] = {
        "_Bool", "_Complex", "_Imaginary",
        "auto", "break", "case", "char", "const", "continue", "default", "do",
        "double", "else", "enum", "extern", "float", "for", "goto", "if",
        "inline", "int", "long", "register", "restrict", "return", "short",
        "signed", "sizeof", "static", "struct", "switch", "typedef", "union",
        "unsigned", "void", "volatile", "while"
    };
    return strbinfind(
            token,
            strlength(token),
            token_keyword,
            ARRAY_SIZE(token_keyword)) >= 0;
}

/* Returns 1 if string is considered a unary operator */
static int tok_isunaryop(const char* str) {
    if (strlength(str) != 1) {
        return 0;
    }

    switch (str[0]) {
        case '&': case '*': case '+': case '-': case '~': case '!':
            return 1;
        default:
            return 0;
    }
}

/* TODO why declare these in global scope? Move them to be a part of their function */
const char* token_assignment_operator[] = {
    /* See strbinfind for ordering requirements */
    "%=", "&=", "*=", "+=", "-=", "/=", "<<=", "=", ">>=", "^=", "|="
};
/* Returns 1 if token is considered an assignment operator */
static int tok_isassignmentop(const char* token) {
    return strbinfind(
            token,
            strlength(token),
            token_assignment_operator,
            ARRAY_SIZE(token_assignment_operator)) >= 0;
}

const char* token_storage_class_keyword[] = {
    /* See strbinfind for ordering requirements */
    "auto",
    "extern",
    "register",
    "static",
    "typedef"
};
/* Returns 1 if token c string is a storage class keyword, 0 otherwise */
static int tok_isstoreclass(const char* str) {
    return strbinfind(
            str,
            strlength(str),
            token_storage_class_keyword,
            ARRAY_SIZE(token_storage_class_keyword)) >= 0;
}

const char* token_type_keyword[] = {
    /* See strbinfind for ordering requirements */
    "char",
    "double",
    "float",
    "int",
    "long",
    "short",
    "signed",
    "unsigned",
    "void",
};
/* Returns 1 if token c string is a type specifier keyword, 0 otherwise */
static int tok_istypespec(const char* str) {
    return strbinfind(
            str,
            strlength(str),
            token_type_keyword,
            ARRAY_SIZE(token_type_keyword)) >= 0;
}

const char* token_type_qualifier_keyword[] = {
    /* See strbinfind for ordering requirements */
    "const",
    "restrict",
    "volatile"
};
/* Returns 1 if token c string is a type qualifier keyword, 0 otherwise */
static int tok_istypequal(const char* str) {
    return strbinfind(
            str,
            strlength(str),
            token_type_qualifier_keyword,
            ARRAY_SIZE(token_type_qualifier_keyword)) >= 0;
}

/* Returns 1 if token c string is a function specifier keyword, 0 otherwise */
static int tok_isfuncspec(const char* str) {
    return strequ(str, "inline");
}

/* Returns 1 if token c string is an identifier */
static int tok_isidentifier(const char* str) {
    if (tok_iskeyword(str)) {
        return 0;
    }

    char c = str[0];
    if (c == '\0') {
        return 0;
    }
    int is_nondigit = 0;
    if ('a' <= c && c <= 'z') {
        is_nondigit = 1;
    }
    else if ('A' <= c && c <= 'Z') {
        is_nondigit = 1;
    }
    else if (c == '_') {
        is_nondigit = 1;
    }
    if (!is_nondigit) {
        return 0;
    }

    int i = 1;
    while ((c = str[i]) != '\0') {
        int is_id = 0;
        if ('a' <= c && c <= 'z') {
            is_id = 1;
        }
        else if ('A' <= c && c <= 'Z') {
            is_id = 1;
        }
        else if (c == '_') {
            is_id = 1;
        }
        else if ('0' <= c && c <= '9') {
            is_id = 1;
        }
        if (!is_id) {
            return 0;
        }
        ++i;
    }
    return 1;
}

/* Reads in a character from input
   Does not advance read position */
static char read_char(Parser* p) {
    /* Perhaps this can be a buffer in the future to reduce system calls */
    char c = (char)getc(p->rf);

    /* Handle line marker from preprocessor*/
    if (p->char_num == 1) {
        while (c == '#') {
            getc(p->rf); /* Consume space */

            /* Line number */
            int line_num = 0;
            while ((c = (char)getc(p->rf)) != ' ') {
                line_num *= 10;
                line_num += c - '0';
            }
            p->line_num = line_num;

            /* Consume linemarker */
            while ((c = (char)getc(p->rf)) != '\n');
            c = (char)getc(p->rf); /* First char of next line */
        }
    }

    fseek(p->rf, -1, SEEK_CUR);
    return c;
}

/* Reads in a character from input 1 ahead
   of character from read_char
   Does not advance read position */
static char read_char_next(Parser* p) {
    getc(p->rf);
    char c = (char)getc(p->rf);
    fseek(p->rf, -2, SEEK_CUR);
    return c;
}


/* Advances read position to next char */
static void consume_char(Parser* p) {
    /* Move the file position forwards */
    char c = (char)getc(p->rf);

    if (c == '\n') {
        ++p->line_num;
        p->char_num = 1;
    }
    else {
        ++p->char_num;
    }
}

/* Indicates the pointed to token is no longer in use */
static void consume_token(char* token) {
    token[0] = '\0';
    if (g_debug_print_parse_recursion) {
        LOG("^Consumed\n");
    }
}

/* Reads a null terminated token
   Returns pointer to the token, the token is blank (just a null terminator)
   if end of file is reached or error happened */
static char* read_token(Parser* p) {
    /* Has cached token */
    if (p->read_buf[0] != '\0') {
        if (g_debug_print_parse_recursion) {
            LOGF("%s ^Cached\n", p->read_buf);
        }
        return p->read_buf;
    }

    char c = read_char(p);
    /* Skip leading whitespace */
    while (iswhitespace(c = read_char(p))) {
        consume_char(p);
    }

    /* Handle punctuators
       sufficient space guaranteed to hold all punctuators*/
    ASSERT(MAX_TOKEN_LEN >= 4,
            "Max token length must be at least 4 to hold C operators");
    if (isofpunctuator(c)) {
        char cn = read_char_next(p);
        /* <= >= == != *= /= %= += -= &= ^= |= */
        if (cn == '=') {
            p->read_buf[0] = c;
            p->read_buf[1] = '=';
            p->read_buf[2] = '\0';
            consume_char(p);
            consume_char(p);
            goto exit;
        }

        /* Handle double char punctuators
           + ++ - -- & && | || */
        char chars[] = {'+', '-', '&', '|'};
        for (int i = 0; i < ARRAY_SIZE(chars); ++i) {
            if (c == chars[i]) {
                consume_char(p);
                p->read_buf[0] = chars[i];
                if (cn == chars[i]) {
                    consume_char(p);
                    p->read_buf[1] = chars[i];
                    p->read_buf[2] = '\0';
                }
                else {
                    p->read_buf[1] = '\0';
                }
                goto exit;
            }
        }

        /* TODO For now, treat everything else as token */
        p->read_buf[0] = c;
        p->read_buf[1] = '\0';
        consume_char(p);
        goto exit;
    }

    /* Handle others */
    int i = 0;
    while (c != EOF) {
        if (iswhitespace(c) || isofpunctuator(c)) {
            break;
        }

        if (i >= MAX_TOKEN_LEN) {
            parser_set_error(p, ec_tokbufexceed);
            break; /* Since MAX_TOKEN_LEN excludes null terminator, we can break and add null terminator */
        }
        p->read_buf[i] = c;
        consume_char(p);
        c = read_char(p);
        ++i;
    }
    p->read_buf[i] = '\0';

exit:
    if (g_debug_print_parse_recursion) {
        LOGF("%s\n", p->read_buf);
    }

    return p->read_buf;
}




/* ============================================================ */
/* C source parsing functions */

/* 1. Shows beginning and end of each function, indented with recursion depth
      The end is green if the production rule was matched, red if not
   2. Remembers parse tree state to allow backtrack if no match was made
      Indicate a match was made by calling PARSE_MATCHED()
   3. Creates the node for the given nonterminal */

int debug_parse_func_recursion_depth = 0;
/* Call at beginning on function */
#define PARSE_FUNC_START(symbol_type__)                                 \
    if (g_debug_print_parse_recursion) {                                \
    for (int i__ = 0; i__ < debug_parse_func_recursion_depth; ++i__) {  \
        LOG("  ");                                                      \
    }                                                                   \
    LOGF(">%d " #symbol_type__ "\n", debug_parse_func_recursion_depth); \
    debug_parse_func_recursion_depth++;                                 \
    }                                                                   \
    ASSERT(parent != NULL, "Parent node is null");                      \
    ParseLocation start_location__ =                                    \
        parser_get_parse_location(p, parent);                           \
    ParseNode* node__ =                                                 \
        tree_attach_node(p, parent, st_ ## symbol_type__);              \
    int matched__ = 0
/* Call at end on function */
#define PARSE_FUNC_END()                                                   \
    if (g_debug_print_parse_recursion) {                                   \
    debug_parse_func_recursion_depth--;                                    \
    for (int i__ = 0; i__ < debug_parse_func_recursion_depth; ++i__) {     \
        LOG("  ");                                                         \
    }                                                                      \
    if (matched__) {LOG("\033[32m");} else {LOG("\033[0;1;31m");}          \
    LOGF("<%d\033[0m\n", debug_parse_func_recursion_depth);                \
    }                                                                      \
    if (!matched__) {parser_set_parse_location(p, &start_location__);}     \
    else {p->last_line_num = p->line_num; p->last_char_num = p->char_num;} \
    return matched__
/* Call if a match was made in production rule */
#define PARSE_MATCHED() matched__ = 1
/* Deletes child nodes of current location */
#define PARSE_TRIM_TREE()                                           \
    if (g_debug_print_parse_recursion) {LOG("Parse tree clear\n");} \
    if (g_debug_print_parse_tree) {debug_print_parse_tree(p);}      \
    tree_detach_node_child(p, node__)

/* Name for the pointer to the current node */
#define PARSE_CURRENT_NODE node__

/* Sorted by Annex A.2 in C99 standard */
/* Return code indicates whether symbol was successfully parsed */
/* ParseNode* is the parent node to attach additional nodes to */

/* 6.4 Lexical elements */
static int parse_identifier(Parser* p, ParseNode* parent);
static int parse_constant(Parser* p, ParseNode* parent);
static int parse_integer_constant(Parser* p, ParseNode* parent);
static int parse_decimal_constant(Parser* p, ParseNode* parent);
static int parse_octal_constant(Parser* p, ParseNode* parent);
static int parse_hexadecimal_constant(Parser*, ParseNode*);
/* 6.5 Expressions */
static int parse_primary_expression(Parser* p, ParseNode* parent);
static int parse_postfix_expression(Parser* p, ParseNode* parent);
static int parse_postfix_expression_2(Parser* p, ParseNode* parent);
static int parse_unary_expression(Parser* p, ParseNode* parent);
static int parse_cast_expression(Parser* p, ParseNode* parent);
static int parse_multiplicative_expression(Parser* p, ParseNode* parent);
static int parse_additive_expression(Parser* p, ParseNode* parent);
static int parse_shift_expression(Parser* p, ParseNode* parent);
static int parse_relational_expression(Parser* p, ParseNode* parent);
static int parse_equality_expression(Parser* p, ParseNode* parent);
static int parse_and_expression(Parser* p, ParseNode* parent);
static int parse_exclusive_or_expression(Parser* p, ParseNode* parent);
static int parse_inclusive_or_expression(Parser* p, ParseNode* parent);
static int parse_logical_and_expression(Parser* p, ParseNode* parent);
static int parse_logical_or_expression(Parser* p, ParseNode* parent);
static int parse_conditional_expression(Parser* p, ParseNode* parent);
static int parse_assignment_expression(Parser* p, ParseNode* parent);
static int parse_assignment_expression_2(Parser* p, ParseNode* parent);
static int parse_assignment_expression_3(Parser* p, ParseNode* parent);
static int parse_expression(Parser* p, ParseNode* parent);
/* 6.7 Declarators */
static int parse_declaration(Parser* p, ParseNode* parent);
static int parse_declaration_specifiers(Parser* p, ParseNode* parent);
static int parse_init_declarator_list(Parser* p, ParseNode* parent);
static int parse_init_declarator(Parser* p, ParseNode* parent);
static int parse_storage_class_specifier(Parser* p, ParseNode* parent);
static int parse_type_specifier(Parser* p, ParseNode* parent);
static int parse_specifier_qualifier_list(Parser*, ParseNode*);
static int parse_type_qualifier(Parser* p, ParseNode* parent);
static int parse_function_specifier(Parser* p, ParseNode* parent);
static int parse_declarator(Parser* p, ParseNode* parent);
static int parse_direct_declarator(Parser* p, ParseNode* parent);
static int parse_direct_declarator_2(Parser* p, ParseNode* parent);
static int parse_pointer(Parser* p, ParseNode* parent);
static int parse_parameter_type_list(Parser* p, ParseNode* parent);
static int parse_parameter_list(Parser* p, ParseNode* parent);
static int parse_parameter_declaration(Parser* p, ParseNode* parent);
static int parse_type_name(Parser*, ParseNode*);
static int parse_abstract_declarator(Parser*, ParseNode*);
static int parse_initializer(Parser* p, ParseNode* parent);
/* 6.8 Statements and blocks */
static int parse_statement(Parser* p, ParseNode* parent);
static int parse_compound_statement(Parser* p, ParseNode* parent);
static int parse_block_item(Parser* p, ParseNode* parent);
static int parse_expression_statement(Parser* p, ParseNode* parent);
static int parse_selection_statement(Parser* p, ParseNode* parent);
static int parse_iteration_statement(Parser* p, ParseNode* parent);
static int parse_jump_statement(Parser* p, ParseNode* parent);
/* 6.9 External definitions */
static int parse_function_definition(Parser* p, ParseNode* parent);
/* Helpers */
static int parse_expect(Parser* p, const char* match_token);

/* 6.4 Lexical elements */
static SymbolId cg_identifier(Parser* p, ParseNode* node);
static SymbolId cg_constant(Parser* p, ParseNode* node);
static SymbolId cg_integer_constant(Parser* p, ParseNode* node);
/* 6.5 Expressions */
static SymbolId cg_primary_expression(Parser* p, ParseNode* node);
static SymbolId cg_postfix_expression(Parser* p, ParseNode* node);
static SymbolId cg_unary_expression(Parser* p, ParseNode* node);
static SymbolId cg_cast_expression(Parser* p, ParseNode* node);
static SymbolId cg_multiplicative_expression(Parser* p, ParseNode* node);
static SymbolId cg_additive_expression(Parser* p, ParseNode* node);
static SymbolId cg_shift_expression(Parser* p, ParseNode* node);
static SymbolId cg_relational_expression(Parser* p, ParseNode* node);
static SymbolId cg_equality_expression(Parser* p, ParseNode* node);
static SymbolId cg_and_expression(Parser* p, ParseNode* node);
static SymbolId cg_exclusive_or_expression(Parser* p, ParseNode* node);
static SymbolId cg_inclusive_or_expression(Parser* p, ParseNode* node);
static SymbolId cg_logical_and_expression(Parser* p, ParseNode* node);
static SymbolId cg_logical_or_expression(Parser* p, ParseNode* node);
static SymbolId cg_conditional_expression(Parser* p, ParseNode* node);
static SymbolId cg_assignment_expression(Parser* p, ParseNode* node);
static SymbolId cg_expression(Parser* p, ParseNode* node);
/* 6.7 Declarators */
static void cg_declaration(Parser* p, ParseNode* node);
static void cg_parameter_list(Parser* p, ParseNode* node);
static void cg_parameter_declaration(Parser* p, ParseNode* node);
static SymbolId cg_initializer(Parser* p, ParseNode* node);
/* 6.8 Statements and blocks */
static void cg_statement(Parser* p, ParseNode* node);
static void cg_block_item(Parser* p, ParseNode* node);
static void cg_expression_statement(Parser* p, ParseNode* node);
static void cg_jump_statement(Parser* p, ParseNode* node);
/* 6.9 External definitions */
/* Helpers */
static TypeSpecifiers cg_extract_type_specifiers(Parser* p, ParseNode* node);
static int cg_extract_pointer(ParseNode* node);
static Type cg_type_name_extract(Parser*, ParseNode*);
static SymbolId cg_extract_symbol(Parser* p,
        ParseNode* declaration_specifiers_node, ParseNode* declarator_node);
/* Code generation helpers */
static void cg_function_signature(Parser* p, ParseNode* node);
static SymbolId cg_make_temporary(Parser* p, Type type);
static SymbolId cg_make_label(Parser* p);
static SymbolId cg_expression_op2(
        Parser*, ParseNode*, SymbolId (*)(Parser*, ParseNode*));
static SymbolId cg_byte_offset(Parser*, SymbolId, int);
static SymbolId cg_expression_op2t(
        Parser*,
        ParseNode*,
        SymbolId (*)(Parser*, ParseNode*),
        const Type* result_type);
static void cg_com_type_rtol(Parser*, SymbolId, SymbolId*);
static Type cg_com_type_lr(Parser*, SymbolId*, SymbolId*);
/* Code generation for each IL instruction, converts operands to non Lvalues
   suffixes are used to specify the different variants */
static SymbolId cg_nlval(Parser*, SymbolId);
static void cgil_cl(Parser*, SymbolId, SymbolId, SymbolId);
static void cgil_cle(Parser*, SymbolId, SymbolId, SymbolId);
static void cgil_jmp(Parser*, SymbolId);
static void cgil_jnz(Parser*, SymbolId, SymbolId);
static void cgil_jz(Parser*, SymbolId, SymbolId);
static void cgil_lab(Parser*, SymbolId);
static void cgil_movss(Parser*, SymbolId, SymbolId);
static void cgil_movsi(Parser*, SymbolId, int);
static void cgil_mtc(Parser*, SymbolId, SymbolId);
static void cgil_mul(Parser*, SymbolId, SymbolId, int);
static void cgil_not(Parser*, SymbolId, SymbolId);
static void cgil_ret(Parser*, SymbolId);

/* identifier */
static int parse_identifier(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(identifier);

    char* token = read_token(p);
    if (parser_has_error(p)) goto exit;

    if (tok_isidentifier(token)) {
        tree_attach_token(p, PARSE_CURRENT_NODE, token);
        consume_token(token);
        PARSE_MATCHED();
        goto exit;
    }

exit:
    PARSE_FUNC_END();
}

static int parse_constant(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(constant);

    if (parse_integer_constant(p, PARSE_CURRENT_NODE)) goto matched;

    /* Incomplete */

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_integer_constant(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(integer_constant);

    if (parse_decimal_constant(p, PARSE_CURRENT_NODE)) goto matched;
    if (parse_octal_constant(p, PARSE_CURRENT_NODE)) goto matched;
    if (parse_hexadecimal_constant(p, PARSE_CURRENT_NODE)) goto matched;

    /* Incomplete */

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_decimal_constant(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(decimal_constant);

    char* token = read_token(p);
    if (parser_has_error(p)) goto exit;

    /* First character is nonzero-digit */
    char c = token[0];
    if (c <= '0' || c > '9') {
        goto exit;
    }

    /* Remaining characters is digit */
    int i = 0;
    while (c != '\0') {
        if (c < '0' || c > '9') {
            goto exit;
        }

        ++i;
        c = token[i];
    }

    tree_attach_token(p, PARSE_CURRENT_NODE, token);
    consume_token(token);
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_octal_constant(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(octal_constant);

    /* Left recursion in C standard is converted to right recursion */
    /* octal-constant
       -> 0 octal-constant-2(opt) */
    /* octal-constant-2
       -> octal-digit octal-constant-2(opt) */

    char* token = read_token(p);
    if (parser_has_error(p)) goto exit;

    char c = token[0];
    if (c != '0') {
        goto exit;
    }

    /* Remaining characters is digit */
    int i = 0;
    while (c != '\0') {
        if (c < '0' || c > '7') {
            goto exit;
        }
        ++i;
        c = token[i];
    }

    tree_attach_token(p, PARSE_CURRENT_NODE, token);
    consume_token(token);
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_hexadecimal_constant(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(hexadecimal_constant);

    char* token = read_token(p);
    if (parser_has_error(p)) goto exit;

    if (token[0] != '0') goto exit;
    if (token[1] != 'x' && token[1] != 'X') goto exit;

    /* Need at least 1 digit */
    char c = token[2];
    if ((c < '0' || c > '9') &&
        (c < 'a' || c > 'f') &&
        (c < 'A' || c > 'F')) {
        goto exit;
    }

    /* Remaining characters is hex digit */
    int i = 3;
    c = token[i];
    while (c != '\0') {
        if ((c < '0' || c > '9') &&
            (c < 'a' || c > 'f') &&
            (c < 'A' || c > 'F')) {
            goto exit;
        }
        ++i;
        c = token[i];
    }

    tree_attach_token(p, PARSE_CURRENT_NODE, token);
    consume_token(token);
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_primary_expression(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(primary_expression);

    if (parse_identifier(p, PARSE_CURRENT_NODE)) goto matched;
    if (parse_constant(p, PARSE_CURRENT_NODE)) goto matched;

    if (parse_expect(p, "(")) {
        if (!parse_expression(p, PARSE_CURRENT_NODE)) goto exit;
        if (parse_expect(p, ")")) goto matched;
    }

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_postfix_expression(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(postfix_expression);

    /* Left recursion in C standard is converted to right recursion */
    /* postfix-expression
       -> primary-expression postfix-expression-2(opt)
        | ( type-name ) { initializer-list } postfix-expression-2(opt)
        | ( type-name ) { initializer-list , } postfix-expression-2(opt) */
    /* postfix-expression-2
       -> [ expression ] postfix-expression-2(opt)
        | ( argument-expression-list(opt) ) postfix-expression-2(opt)
        | . identifier postfix-expression-2(opt)
        | -> identifier postfix-expression-2(opt)
        | ++ postfix-expression-2(opt)
        | -- postfix-expression-2(opt) */

    if (parse_primary_expression(p, PARSE_CURRENT_NODE)) {
        PARSE_MATCHED();
        parse_postfix_expression_2(p, PARSE_CURRENT_NODE);
    }

    /* Incomplete */

    PARSE_FUNC_END();
}

static int parse_postfix_expression_2(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(postfix_expression);

    if (parse_expect(p, "[")) {
        if (!parse_expression(p, PARSE_CURRENT_NODE)) goto exit;
        if (!parse_expect(p, "]")) goto exit;
        PARSE_MATCHED();

        parse_postfix_expression_2(p, PARSE_CURRENT_NODE);
    }
    /* Postfix increment, decrement */
    else if (parse_expect(p, "++")) {
        tree_attach_token(p, PARSE_CURRENT_NODE, "++");
        PARSE_MATCHED();

        parse_postfix_expression_2(p, PARSE_CURRENT_NODE);
    }
    else if (parse_expect(p, "--")) {
        tree_attach_token(p, PARSE_CURRENT_NODE, "--");
        PARSE_MATCHED();

        parse_postfix_expression_2(p, PARSE_CURRENT_NODE);
    }

    /* Incomplete */

exit:
    PARSE_FUNC_END();
}

static int parse_unary_expression(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(unary_expression);

    char* token = read_token(p);
    if (parser_has_error(p)) goto exit;

    /* Prefix increment, decrement */
    if (strequ(token, "++")) {
        tree_attach_token(p, PARSE_CURRENT_NODE, token);
        consume_token(token);
        if (!parse_unary_expression(p, PARSE_CURRENT_NODE)) goto exit;
    }
    else if (strequ(token, "--")) {
        tree_attach_token(p, PARSE_CURRENT_NODE, token);
        consume_token(token);
        if (!parse_unary_expression(p, PARSE_CURRENT_NODE)) goto exit;
    }
    else if (tok_isunaryop(token)) {
        tree_attach_token(p, PARSE_CURRENT_NODE, token);
        consume_token(token);
        if (!parse_cast_expression(p, PARSE_CURRENT_NODE)) goto exit;
    }
    else if (!parse_postfix_expression(p, PARSE_CURRENT_NODE)) {
        goto exit;
    }

    /* Incomplete */

    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_cast_expression(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(cast_expression);

    /* This comes first as the below consumes ( */
    if (parse_unary_expression(p, PARSE_CURRENT_NODE)) {
        PARSE_MATCHED();
    }
    else if (parse_expect(p, "(")) {
        if (!parse_type_name(p, PARSE_CURRENT_NODE)) goto exit;
        if (!parse_expect(p, ")")) goto exit;
        if (!parse_cast_expression(p, PARSE_CURRENT_NODE)) goto exit;

        PARSE_MATCHED();
    }

exit:
    PARSE_FUNC_END();
}

static int parse_multiplicative_expression(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(multiplicative_expression);

    /* Left recursion in C standard is converted to right recursion */
    /* NOTE: This does affect order of operations, operations involving
       multiplicative_expression nodes if preorder traversal
       e.g., a * b / c in the order of encountering them */
    /* multiplicative-expression
       -> cast-expression * multiplicative-expression
        | cast-expression / multiplicative-expression
        | cast-expression % multiplicative-expression
        | cast-expression */

    if (!parse_cast_expression(p, PARSE_CURRENT_NODE)) goto exit;

    if (parse_expect(p, "*")) {
        tree_attach_token(p, PARSE_CURRENT_NODE, "mul");
        if (!parse_multiplicative_expression(p, PARSE_CURRENT_NODE)) goto exit;
    }
    else if (parse_expect(p, "/")) {
        tree_attach_token(p, PARSE_CURRENT_NODE, "div");
        if (!parse_multiplicative_expression(p, PARSE_CURRENT_NODE)) goto exit;
    }
    else if (parse_expect(p, "%")) {
        tree_attach_token(p, PARSE_CURRENT_NODE, "mod");
        if (!parse_multiplicative_expression(p, PARSE_CURRENT_NODE)) goto exit;
    }

    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_additive_expression(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(additive_expression);

    /* Left recursion in C standard is converted to right recursion */
    /* NOTE: This does affect order of operations, operations involving
       multiplicative_expression nodes is preorder traversal
       e.g., a + b - c in the order of encountering them */
    /* additive-expression -> multiplicative-expression + additive-expression
                            | multiplicative-expression - additive-expression
                            | multiplicative-expression */

    if (!parse_multiplicative_expression(p, PARSE_CURRENT_NODE)) goto exit;

    if (parse_expect(p, "+")) {
        tree_attach_token(p, PARSE_CURRENT_NODE, "+");
        if (!parse_additive_expression(p, PARSE_CURRENT_NODE)) goto exit;
    }
    if (parse_expect(p, "-")) {
        tree_attach_token(p, PARSE_CURRENT_NODE, "-");
        if (!parse_additive_expression(p, PARSE_CURRENT_NODE)) goto exit;
    }

    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_shift_expression(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(shift_expression);

    if (parse_additive_expression(p, PARSE_CURRENT_NODE)) goto matched;

    /* Incomplete */

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_relational_expression(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(relational_expression);

    /* Left recursion in C standard is converted to right recursion */
    /* relational-expression
        -> shift-expression
         | shift-expression < relational-expression
         | shift-expression > relational-expression
         | shift-expression <= relational-expression
         | shift-expression >= relational-expression*/

    if (!parse_shift_expression(p, PARSE_CURRENT_NODE)) goto exit;

    if (parse_expect(p, "<")) {
        tree_attach_token(p, PARSE_CURRENT_NODE, "<");
        if (!parse_relational_expression(p, PARSE_CURRENT_NODE)) goto exit;
    }
    else if (parse_expect(p, ">")) {
        tree_attach_token(p, PARSE_CURRENT_NODE, ">");
        if (!parse_relational_expression(p, PARSE_CURRENT_NODE)) goto exit;
    }
    else if (parse_expect(p, "<=")) {
        tree_attach_token(p, PARSE_CURRENT_NODE, "<=");
        if (!parse_relational_expression(p, PARSE_CURRENT_NODE)) goto exit;
    }
    else if (parse_expect(p, ">=")) {
        tree_attach_token(p, PARSE_CURRENT_NODE, ">=");
        if (!parse_relational_expression(p, PARSE_CURRENT_NODE)) goto exit;
    }
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_equality_expression(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(equality_expression);

    /* Left recursion in C standard is converted to right recursion */
    /* equality-expression
        -> relational-expression
         | relational-expression == equality-expression
         | relational-expression != equality-expression */

    if (!parse_relational_expression(p, PARSE_CURRENT_NODE)) goto exit;

    if (parse_expect(p, "==")) {
        tree_attach_token(p, PARSE_CURRENT_NODE, "ce");
        if (!parse_equality_expression(p, PARSE_CURRENT_NODE)) goto exit;
    }
    else if (parse_expect(p, "!=")) {
        tree_attach_token(p, PARSE_CURRENT_NODE, "cne");
        if (!parse_equality_expression(p, PARSE_CURRENT_NODE)) goto exit;
    }
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_and_expression(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(and_expression);

    if (parse_equality_expression(p, PARSE_CURRENT_NODE)) goto matched;

    /* Incomplete */

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_exclusive_or_expression(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(exclusive_or_expression);

    if (parse_and_expression(p, PARSE_CURRENT_NODE)) goto matched;

    /* Incomplete */

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_inclusive_or_expression(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(inclusive_or_expression);

    if (parse_exclusive_or_expression(p, PARSE_CURRENT_NODE)) goto matched;

    /* Incomplete */

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_logical_and_expression(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(logical_and_expression);

    /* Left recursion in C standard is converted to right recursion */
    /* logical-and-expression
        -> inclusive-or-expression
         | inclusive-or-expression && logical-and-expression */

    if (!parse_inclusive_or_expression(p, PARSE_CURRENT_NODE)) goto exit;

    if (parse_expect(p, "&&")) {
        if (!parse_logical_and_expression(p, PARSE_CURRENT_NODE)) goto exit;
    }
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_logical_or_expression(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(logical_or_expression);

    /* Left recursion in C standard is converted to right recursion */
    /* logical-or-expression
        -> logical-and-expression
         | logical-and-expression || logical-or-expression */

    if (!parse_logical_and_expression(p, PARSE_CURRENT_NODE)) goto exit;

    if (parse_expect(p, "||")) {
        if (!parse_logical_or_expression(p, PARSE_CURRENT_NODE)) goto exit;
    }
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_conditional_expression(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(conditional_expression);

    if (parse_logical_or_expression(p, PARSE_CURRENT_NODE)) goto matched;

    /* Incomplete */

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_assignment_expression(Parser* p, ParseNode* parent) {
    /* For certain tokens, unary-expression can match but fail the
       remaining production, it must backtrack to undo the unary-expression
       to attempt another rule */
    /* Cannot check conditional-expression first, assignment-expression may
       match, but parent productions fail to match */
    /* Because of how the backtrack system is implemented, the function
       must return for backtrack to occur, thus I split the two possible
       ways to form a production into functions */
    if (parse_assignment_expression_3(p, parent)) return 1;
    if (parse_assignment_expression_2(p, parent)) return 1;

    return 0;
}

static int parse_assignment_expression_2(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(assignment_expression);

    if (parse_conditional_expression(p, PARSE_CURRENT_NODE)) {
        PARSE_MATCHED();
    }

    PARSE_FUNC_END();
}

static int parse_assignment_expression_3(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(assignment_expression);

    if (!parse_unary_expression(p, PARSE_CURRENT_NODE)) goto exit;

    char* token = read_token(p);
    if (!tok_isassignmentop(token)) goto exit;
    tree_attach_token(p, PARSE_CURRENT_NODE, token);
    consume_token(token);

    if (!parse_assignment_expression(p, PARSE_CURRENT_NODE)) goto exit;
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_expression(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(expression);

    if (parse_assignment_expression(p, PARSE_CURRENT_NODE)) goto matched;

    /* Incomplete */

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_declaration(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(declaration);

    if (!parse_declaration_specifiers(p, PARSE_CURRENT_NODE)) goto exit;
    parse_init_declarator_list(p, PARSE_CURRENT_NODE);
    if (!parse_expect(p, ";")) goto exit;

    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_declaration_specifiers(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(declaration_specifiers);

    if (parse_storage_class_specifier(p, PARSE_CURRENT_NODE)) goto matched;
    if (parse_type_specifier(p, PARSE_CURRENT_NODE)) goto matched;
    if (parse_type_qualifier(p, PARSE_CURRENT_NODE)) goto matched;
    if (parse_function_specifier(p, PARSE_CURRENT_NODE)) goto matched;

    goto exit;

matched:
    PARSE_MATCHED();
    parse_declaration_specifiers(p, PARSE_CURRENT_NODE);

exit:
    PARSE_FUNC_END();
}

static int parse_init_declarator_list(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(init_declarator_list);

    if (!parse_init_declarator(p, PARSE_CURRENT_NODE)) goto exit;

    if (parse_expect(p, ",")) {
        if (!parse_init_declarator(p, PARSE_CURRENT_NODE)) goto exit;
    }

    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_init_declarator(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(init_declarator);

    if (!parse_declarator(p, PARSE_CURRENT_NODE)) goto exit;

    if (parse_expect(p, "=")) {
        if (!parse_initializer(p, PARSE_CURRENT_NODE)) goto exit;
    }

    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_storage_class_specifier(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(storage_class_specifier);

    char* token = read_token(p);
    if (parser_has_error(p)) goto exit;
    if (tok_isstoreclass(token)) {
        tree_attach_token(p, PARSE_CURRENT_NODE, token);
        consume_token(token);
        PARSE_MATCHED();
    }

exit:
    PARSE_FUNC_END();
}

static int parse_type_specifier(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(type_specifier);

    char* token = read_token(p);
    if (parser_has_error(p)) goto exit;
    if (tok_istypespec(token)) {
        tree_attach_token(p, PARSE_CURRENT_NODE, token);
        consume_token(token);
        PARSE_MATCHED();
    }

exit:
    PARSE_FUNC_END();
}

static int parse_specifier_qualifier_list(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(specifier_qualifier_list);

    if (parse_type_specifier(p, PARSE_CURRENT_NODE)) {
        PARSE_MATCHED();
        parse_specifier_qualifier_list(p, PARSE_CURRENT_NODE);

        /* Incomplete */
    }
    else if (parse_type_qualifier(p, PARSE_CURRENT_NODE)) {
        PARSE_MATCHED();
        parse_specifier_qualifier_list(p, PARSE_CURRENT_NODE);

        /* Incomplete */
    }

    PARSE_FUNC_END();
}

static int parse_type_qualifier(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(type_qualifier);

    char* token = read_token(p);
    if (parser_has_error(p)) goto exit;
    if (tok_istypequal(token)) {
        tree_attach_token(p, PARSE_CURRENT_NODE, token);
        consume_token(token);
        PARSE_MATCHED();
    }

exit:
    PARSE_FUNC_END();
}

static int parse_function_specifier(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(function_specifier);

    char* token = read_token(p);
    if (parser_has_error(p)) goto exit;
    if (tok_isfuncspec(token)) {
        tree_attach_token(p, PARSE_CURRENT_NODE, token);
        consume_token(token);
        PARSE_MATCHED();
    }

exit:
    PARSE_FUNC_END();
}

static int parse_declarator(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(declarator);

    parse_pointer(p, PARSE_CURRENT_NODE);
    if (parser_has_error(p)) goto exit;

    if (!parse_direct_declarator(p, PARSE_CURRENT_NODE)) goto exit;

    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_direct_declarator(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(direct_declarator);

    /* Left recursion in C standard is converted to right recursion */
    /* direct-declarator -> identifier direct-declarator-2(optional)
                          | ( declarator ) direct-declarator-2(optional) */
    /* direct-declarator-2 ->
       | [ type-qualifier-list(optional) assignment-expression(optional) ]
         direct-declarator-2(optional)
         ...
       | ( parameter-type-list ) direct-declarator-2(optional)
         direct-declarator-2(optional) */

    if (!parse_identifier(p, PARSE_CURRENT_NODE)) goto exit;
    PARSE_MATCHED();

    parse_direct_declarator_2(p, PARSE_CURRENT_NODE);

exit:
    PARSE_FUNC_END();
}

static int parse_direct_declarator_2(Parser* p, ParseNode* parent) {
    /* Use the same symbol type as it was originally one nonterminal
       split into two */
    PARSE_FUNC_START(direct_declarator);

    if (parse_expect(p, "[")) {
        tree_attach_token(p, PARSE_CURRENT_NODE, "[");

        if (!parse_assignment_expression(p, PARSE_CURRENT_NODE)) goto exit;
        /* Incomplete */
        if (!parse_expect(p, "]")) goto exit;
        PARSE_MATCHED();
        parse_direct_declarator_2(p, PARSE_CURRENT_NODE);
    }
    if (parse_expect(p, "(")) {
        tree_attach_token(p, PARSE_CURRENT_NODE, "(");

        if (!parse_parameter_type_list(p, PARSE_CURRENT_NODE)) goto exit;
        if (!parse_expect(p, ")")) goto exit;
        PARSE_MATCHED();
        parse_direct_declarator_2(p, PARSE_CURRENT_NODE);
    }
    goto exit;
exit:
    PARSE_FUNC_END();
}

static int parse_pointer(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(pointer);

    if (!parse_expect(p, "*")) goto exit;
    PARSE_MATCHED();

    parse_pointer(p, PARSE_CURRENT_NODE);

exit:
    PARSE_FUNC_END();
}

static int parse_parameter_type_list(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(parameter_type_list);

    if (!parse_parameter_list(p, PARSE_CURRENT_NODE)) goto exit;
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_parameter_list(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(parameter_list);

    /* Left recursion in C standard is converted to right recursion */
    /* parameter-list -> parameter-declaration
                       | parameter-declaration , parameter-list */

    if (!parse_parameter_declaration(p, PARSE_CURRENT_NODE)) goto exit;

    if (parse_expect(p, ",")) {
        if (!parse_parameter_list(p, PARSE_CURRENT_NODE)) {
            goto exit;
        }
    }
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_parameter_declaration(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(parameter_declaration);

    if (!parse_declaration_specifiers(p, PARSE_CURRENT_NODE)) goto exit;
    if (!parse_declarator(p, PARSE_CURRENT_NODE)) goto exit;

    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_type_name(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(type_name);

    if (!parse_specifier_qualifier_list(p, PARSE_CURRENT_NODE)) goto exit;
    parse_abstract_declarator(p, PARSE_CURRENT_NODE);

    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_abstract_declarator(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(abstract_declarator);

    if (!parse_pointer(p, PARSE_CURRENT_NODE)) goto exit;

    /* Incomplete */

    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_initializer(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(initializer);

    if (parse_assignment_expression(p, PARSE_CURRENT_NODE)) goto matched;

    /* Incomplete */

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_statement(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(statement);

    if (parse_compound_statement(p, PARSE_CURRENT_NODE)) goto matched;
    if (parse_expression_statement(p, PARSE_CURRENT_NODE)) goto matched;
    if (parse_selection_statement(p, PARSE_CURRENT_NODE)) goto matched;
    if (parse_iteration_statement(p, PARSE_CURRENT_NODE)) goto matched;
    if (parse_jump_statement(p, PARSE_CURRENT_NODE)) goto matched;

    /* Incomplete */

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_compound_statement(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(compound_statement);

    if (parse_expect(p, "{")) {
        symtab_push_scope(p);
        /* block-item-list nodes are not needed for il generation */
        while (parse_block_item(p, PARSE_CURRENT_NODE)) {
            cg_block_item(p, parse_node_child(PARSE_CURRENT_NODE, 0));
            PARSE_TRIM_TREE();
        }

        if (!parse_expect(p, "}")) {
            ERRMSG("Expected '}' to end compound statement\n");
            parser_set_error(p, ec_syntaxerr);
            goto exit;
        }
        PARSE_MATCHED();
        symtab_pop_scope(p);
    }

exit:
    PARSE_FUNC_END();
}

static int parse_block_item(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(block_item);

    if (parse_declaration(p, PARSE_CURRENT_NODE)) goto matched;
    if (parse_statement(p, PARSE_CURRENT_NODE)) goto matched;

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_expression_statement(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(expression_statement);

    parse_expression(p, PARSE_CURRENT_NODE);
    if (!parse_expect(p, ";")) goto exit;

    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_selection_statement(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(selection_statement);

    /* Generate as follows: (if only, no else):
       evaluate expression
       jz false
         code when true
       false: */
    /* Generate as follows: (if else):
       evaluate expression
       jz false
         code when true
         jmp end
       false:
         code when false
       end: */

    /* Validate that the production matches if "if" seen
       as it is not possible to backtrack after il generation and
       parse tree is cleared */

    if (!parse_expect(p, "if")) goto exit;
    if (!parse_expect(p, "(")) {
        ERRMSG("Expected '('\n");
        goto syntaxerr;
    }
    if (!parse_expression(p, PARSE_CURRENT_NODE)) {
        ERRMSG("Expected expession\n");
        goto syntaxerr;
    }
    if (!parse_expect(p, ")")) {
        ERRMSG("Expected ')'\n");
        goto syntaxerr;
    }

    SymbolId lab_false_id = cg_make_label(p);

    SymbolId exp_id = cg_expression(p, parse_node_child(PARSE_CURRENT_NODE, 0));
    PARSE_TRIM_TREE();
    cgil_jz(p, lab_false_id, exp_id);

    symtab_push_scope(p);
    if (!parse_statement(p, PARSE_CURRENT_NODE)) {
        ERRMSG("Expected statement\n");
        goto syntaxerr;
    }

    cg_statement(p, parse_node_child(PARSE_CURRENT_NODE, 0));
    PARSE_TRIM_TREE();
    symtab_pop_scope(p);

    if (parse_expect(p, "else")) {
        SymbolId lab_end_id = cg_make_label(p);
        cgil_jmp(p, lab_end_id);
        cgil_lab(p, lab_false_id);

        symtab_push_scope(p);
        if (!parse_statement(p, PARSE_CURRENT_NODE)) {
            ERRMSG("Expected statement\n");
            goto syntaxerr;
        }

        cg_statement(p, parse_node_child(PARSE_CURRENT_NODE, 0));
        PARSE_TRIM_TREE();
        symtab_pop_scope(p);

        cgil_lab(p, lab_end_id);
    }
    else {
        cgil_lab(p, lab_false_id);
    }

    /* Incomplete */

    PARSE_MATCHED();
    goto exit;

syntaxerr:
    parser_set_error(p, ec_syntaxerr);

exit:
    PARSE_FUNC_END();
}

static int parse_iteration_statement(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(iteration_statement);

    if (parse_expect(p, "while")) {
        /* Generate as follows:
           eval expr1
           jz end
           loop:
           statement
           loop_body_end:
           eval expr1
           jnz loop
           end: */
        if (!parse_expect(p, "(")) {
            ERRMSG("Expected '('\n");
            goto syntaxerr;
        }
        if (!parse_expression(p, PARSE_CURRENT_NODE)) {
            ERRMSG("Expected expression\n");
            goto syntaxerr;
        }
        if (!parse_expect(p, ")")) {
            ERRMSG("Expected ')'\n");
            goto syntaxerr;
        }

        SymbolId lab_loop_id = cg_make_label(p);
        SymbolId lab_body_end_id = cg_make_label(p);
        SymbolId lab_end_id = cg_make_label(p);
        symtab_push_cat(p, sc_lab_loopbodyend, lab_body_end_id);
        symtab_push_cat(p, sc_lab_loopend, lab_end_id);

        SymbolId exp_id =
            cg_expression(p, parse_node_child(PARSE_CURRENT_NODE, 0));
        cgil_jz(p, lab_end_id, exp_id);
        cgil_lab(p, lab_loop_id);

        symtab_push_scope(p);
        if (!parse_statement(p, PARSE_CURRENT_NODE)) {
            ERRMSG("Expected statement\n");
            goto syntaxerr;
        }
        cg_statement(p, parse_node_child(PARSE_CURRENT_NODE, 1));
        symtab_pop_scope(p);
        symtab_pop_cat(p, sc_lab_loopbodyend);
        symtab_pop_cat(p, sc_lab_loopend);

        cgil_lab(p, lab_body_end_id);

        exp_id = cg_expression(p, parse_node_child(PARSE_CURRENT_NODE, 0));
        cgil_jnz(p, lab_loop_id, exp_id);
        cgil_lab(p, lab_end_id);
        PARSE_TRIM_TREE();

        PARSE_MATCHED();
    }
    else if (parse_expect(p, "do")) {
        /* Generate as follows:
           loop:
           statement
           loop_body_end:
           eval expr1
           jnz loop
           end: */
        SymbolId lab_loop_id = cg_make_label(p);
        SymbolId lab_body_end_id = cg_make_label(p);
        SymbolId lab_end_id = cg_make_label(p);
        symtab_push_cat(p, sc_lab_loopbodyend, lab_body_end_id);
        symtab_push_cat(p, sc_lab_loopend, lab_end_id);

        cgil_lab(p, lab_loop_id);

        symtab_push_scope(p);
        if (!parse_statement(p, PARSE_CURRENT_NODE)) {
            ERRMSG("Expected statement\n");
            goto syntaxerr;
        }
        cg_statement(p, parse_node_child(PARSE_CURRENT_NODE, 0));
        PARSE_TRIM_TREE();

        symtab_pop_scope(p);
        symtab_pop_cat(p, sc_lab_loopbodyend);
        symtab_pop_cat(p, sc_lab_loopend);

        cgil_lab(p, lab_body_end_id);

        if (!parse_expect(p, "while")) {
            ERRMSG("Expected 'while'\n");
            goto syntaxerr;
        }
        if (!parse_expect(p, "(")) {
            ERRMSG("Expected '('\n");
            goto syntaxerr;
        }
        if (!parse_expression(p, PARSE_CURRENT_NODE)) {
            ERRMSG("Expected expression\n");
            goto syntaxerr;
        }
        if (!parse_expect(p, ")")) {
            ERRMSG("Expected ')'\n");
            goto syntaxerr;
        }
        if (!parse_expect(p, ";")) {
            ERRMSG("Expected ';'\n");
            goto syntaxerr;
        }
        SymbolId exp_id =
            cg_expression(p, parse_node_child(PARSE_CURRENT_NODE, 0));
        cgil_jnz(p, lab_loop_id, exp_id);
        PARSE_TRIM_TREE();

        cgil_lab(p, lab_end_id);

        PARSE_MATCHED();
    }
    else if (parse_expect(p, "for")) {
        /* Generate as follows:
           expr1 / declaration
           eval expr2
           jz end
           loop:
           statement
           loop_body_end:
           expr3
           eval expr2
           jnz loop
           end: */

        symtab_push_scope(p); /* Scope for declaration */
        if (!parse_expect(p, "(")) {
            ERRMSG("Expected '('\n");
            goto syntaxerr;
        }

        /* expression-1 or declaration */
        if (parse_expression(p, PARSE_CURRENT_NODE)) {
            cg_expression(p, parse_node_child(PARSE_CURRENT_NODE, 0));
            if (!parse_expect(p, ";")) {
                ERRMSG("Expected ';'\n");
                goto syntaxerr;
            }
            PARSE_TRIM_TREE();
        }
        else if (parse_declaration(p, PARSE_CURRENT_NODE)) {
            cg_declaration(p, parse_node_child(PARSE_CURRENT_NODE, 0));
            PARSE_TRIM_TREE();
        }
        else {
            ERRMSG("Expected expression or declaration\n");
            goto syntaxerr;
        }

        /* expression-2 (Controlling expression)
           Stored as child 0 */
        if (!parse_expression(p, PARSE_CURRENT_NODE)) {
            ERRMSG("Expected expession\n");
            goto syntaxerr;
        }
        if (!parse_expect(p, ";")) {
            ERRMSG("Expected ';'\n");
            goto syntaxerr;
        }

        /* expression-3
           Stored as child 1 */
        if (!parse_expression(p, PARSE_CURRENT_NODE)) {
            ERRMSG("Expected expession\n");
            goto syntaxerr;
        }
        if (!parse_expect(p, ")")) {
            ERRMSG("Expected ')'\n");
            goto syntaxerr;
        }

        SymbolId lab_loop_id = cg_make_label(p);
        SymbolId lab_body_end_id = cg_make_label(p);
        SymbolId lab_end_id = cg_make_label(p);
        symtab_push_cat(p, sc_lab_loopbodyend, lab_body_end_id);
        symtab_push_cat(p, sc_lab_loopend, lab_end_id);

        SymbolId exp2_id =
            cg_expression(p, parse_node_child(PARSE_CURRENT_NODE, 0));
        cgil_jz(p, lab_end_id, exp2_id);
        cgil_lab(p, lab_loop_id);

        symtab_push_scope(p);
        if (!parse_statement(p, PARSE_CURRENT_NODE)) {
            ERRMSG("Expected statement\n");
            goto syntaxerr;
        }
        cg_statement(p, parse_node_child(PARSE_CURRENT_NODE, 2));
        symtab_pop_scope(p);
        symtab_pop_cat(p, sc_lab_loopbodyend);
        symtab_pop_cat(p, sc_lab_loopend);

        cgil_lab(p, lab_body_end_id);

        /* expression-3 (At loop end)
           expression-2 (Controlling expression) */
        cg_expression(p, parse_node_child(PARSE_CURRENT_NODE, 1));
        exp2_id = cg_expression(p, parse_node_child(PARSE_CURRENT_NODE, 0));
        PARSE_TRIM_TREE();
        cgil_jnz(p, lab_loop_id, exp2_id);
        cgil_lab(p, lab_end_id);

        symtab_pop_scope(p); /* Scope for declaration */

        PARSE_MATCHED();
    }

    /* Incomplete */

    goto exit;

syntaxerr:
    parser_set_error(p, ec_syntaxerr);

exit:
    PARSE_FUNC_END();
}

static int parse_jump_statement(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(jump_statement);

    if (parse_expect(p, "continue")) {
        tree_attach_token(p, PARSE_CURRENT_NODE, "continue");
        parse_expression(p, PARSE_CURRENT_NODE);
        if (parse_expect(p, ";")) goto matched;
    }
    if (parse_expect(p, "break")) {
        tree_attach_token(p, PARSE_CURRENT_NODE, "break");
        parse_expression(p, PARSE_CURRENT_NODE);
        if (parse_expect(p, ";")) goto matched;
    }
    else if (parse_expect(p, "return")) {
        tree_attach_token(p, PARSE_CURRENT_NODE, "return");
        parse_expression(p, PARSE_CURRENT_NODE);
        if (parse_expect(p, ";")) goto matched;
    }

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_function_definition(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(function_definition);

    if (!parse_declaration_specifiers(p, PARSE_CURRENT_NODE) ||
            parser_has_error(p)) goto exit;

    if (!parse_declarator(p, PARSE_CURRENT_NODE) ||
            parser_has_error(p)) goto exit;

    cg_function_signature(p, PARSE_CURRENT_NODE);
    PARSE_TRIM_TREE();

    if (!parse_compound_statement(p, PARSE_CURRENT_NODE) ||
            parser_has_error(p)) goto exit;

    PARSE_MATCHED();
exit:
    PARSE_FUNC_END();
}

/* Return 1 if next token read matches provided token, 0 otherwise */
/* The token is consumed if it matches the provided token */
static int parse_expect(Parser* p, const char* match_token) {
    char* token = read_token(p);
    if (parser_has_error(p)) {
        return 0;
    }

    if (strequ(token, match_token)) {
        consume_token(token);
        return 1;
    }
    return 0;
}

/* ============================================================ */
/* Intermediate code generation */
/* codegen functions assume the parse tree is valid */

/* 1. Prints out start of each recursive function
   2. Verifies correct node type */
/* Call at beginning on function */
int debug_cg_func_recursion_depth = 0;
#define DEBUG_CG_FUNC_START(symbol_type__)                           \
    if (g_debug_print_cg_recursion) {                                \
    for (int i__ = 0; i__ < debug_cg_func_recursion_depth; ++i__) {  \
        LOG("  ");                                                   \
    }                                                                \
    LOGF(">%d " #symbol_type__ "\n", debug_cg_func_recursion_depth); \
    debug_cg_func_recursion_depth++;                                 \
    }                                                                \
    ASSERT(node != NULL, "Node is null");                            \
    ASSERT(parse_node_type(node) == st_ ## symbol_type__,            \
            "Incorrect node type")
#define DEBUG_CG_FUNC_END()                                         \
    if (g_debug_print_cg_recursion) {                               \
    debug_cg_func_recursion_depth--;                                \
    for (int i__ = 0; i__ < debug_cg_func_recursion_depth; ++i__) { \
        LOG("  ");                                                  \
    }                                                               \
    LOGF("<%d\n", debug_cg_func_recursion_depth);                   \
    } do {} while (0)

static SymbolId cg_identifier(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(identifier);

    ParseNode* token_node = parse_node_child(node, 0);
    TokenId tok_id = parse_node_token_id(token_node);
    SymbolId sym_id = symtab_find(p, tok_id);
    ASSERT(symid_valid(sym_id), "Failed to find identifier");

    DEBUG_CG_FUNC_END();
    return sym_id;
}

static SymbolId cg_constant(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(constant);

    SymbolId sym_id = cg_integer_constant(p, parse_node_child(node, 0));

    /* Incomplete */

    DEBUG_CG_FUNC_END();
    return sym_id;
}

static SymbolId cg_integer_constant(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(integer_constant);

    /* The various type of integer constants are all handled
       the same, by passing it to the assembler */
    ParseNode* constant_node = parse_node_child(node, 0);
    ParseNode* token_node = parse_node_child(constant_node, 0);
    TokenId tok_id = parse_node_token_id(token_node);

    /* TODO temporary hardcode for ts_i32 */
    Type type;
    type_construct(&type, ts_i32, 0);
    SymbolId sym_id = symtab_add(p, tok_id, type, vc_nlval);

    DEBUG_CG_FUNC_END();
    return sym_id;
}

static SymbolId cg_primary_expression(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(primary_expression);

    SymbolId sym_id;
    ParseNode* child = parse_node_child(node, 0);
    if (parse_node_type(child) == st_identifier) {
        sym_id = cg_identifier(p, child);
    }
    else if (parse_node_type(child) == st_constant) {
        sym_id = cg_constant(p, child);
    }
    else if (parse_node_type(child) == st_expression) {
        sym_id = cg_expression(p, child);
    }
    else {
        ASSERT(0, "Unimplemented");
    }

    /* Incomplete */

    DEBUG_CG_FUNC_END();
    return sym_id;
}

/* Generates the IL necessary to index a given pointer at the given index
   Returns Lvalue holding the location yielded from indexing */
static SymbolId cg_array_subscript(Parser* p, SymbolId ptr, SymbolId index) {
    Type type = symtab_get_type(p, ptr);
    type_dec_indirection(&type);
    SymbolId byte_index = cg_byte_offset(p, index, type_bytes(type));

    /* Obtain old_ptr by converting a non Lvalue
       p[0][1]
       ~~~~ This to a non Lvalue so can take first index of pointer.
            The index is converted into bytes as required by IL */
    SymbolId result = symtab_add(p, -1, type, vc_lval);
    symbol_sl_access(symtab_get(p, result), cg_nlval(p, ptr), byte_index);
    return result;
}

static SymbolId cg_postfix_expression(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(postfix_expression);

    SymbolId sym_id = cg_primary_expression(p, parse_node_child(node, 0));
                                 /* ^^^ The symbol which is incremented */
    SymbolId result_id = sym_id; /* Value read (given to others) */

    /* Integer promotions not needed here as the increment is applied after
       the value is read, thus the read value can never overflow */

    /* Traverse the postfix nodes, no need for recursion
       as they are all the same type */
    /* Does not need to check for NULL with last_child as it always has at
       least 1 node to return (primary expression, token node, etc)*/
    ParseNode* last_child = parse_node_child(node, -1);
    while (parse_node_type(last_child) == st_postfix_expression) {
        /* Node held by each child */
        ParseNode* n = parse_node_child(last_child, 0);
        ASSERT(node != NULL, "Expected node");

        if (parse_node_type(n) == st_expression) {
            SymbolId index = cg_expression(p, n);
            result_id = cg_array_subscript(p, result_id, index);
            /* ^ New                          ^ Old
               Dereference old result_id to get new result_id */
        }
        else {
            const char* token = parser_get_token(p, parse_node_token_id(n));
            /* Postfix increment, decrement */
            Type type = symtab_get_type(p, result_id);
            if (strequ(token, "++")) {
                SymbolId temp_id = cg_make_temporary(p, type);
                cgil_movss(p, temp_id, result_id);
                result_id = temp_id;

                if (type_is_arithmetic(&type)) {
                    parser_output_il(p, "add $s,$s,1\n", sym_id, sym_id);
                }
                else if (type_is_pointer(&type)) {
                    parser_output_il(
                            p,
                            "add $s,$s,$d\n",
                            sym_id,
                            sym_id,
                            type_bytes(type_point_to(&type)));
                }
                else {
                    ERRMSG("Invalid operands for postfix ++ operator\n");
                }
            }
            else if (strequ(token, "--")) {
                SymbolId temp_id = cg_make_temporary(p, type);
                cgil_movss(p, temp_id, result_id);
                result_id = temp_id;

                if (type_is_arithmetic(&type)) {
                    parser_output_il(p, "sub $s,$s,1\n", sym_id, sym_id);
                }
                else if (type_is_pointer(&type)) {
                    parser_output_il(
                            p,
                            "sub $s,$s,$d\n",
                            sym_id,
                            sym_id,
                            type_bytes(type_point_to(&type)));
                }
                else {
                    ERRMSG("Invalid operands for prefix -- operator\n");
                }
            }
        }

        /* Incomplete */

        node = last_child;
        last_child = parse_node_child(node, -1);
    }

    DEBUG_CG_FUNC_END();
    return result_id;
}

/* Helper function for cg_unary_expression, performs integer promotions if
   necessary
   id is symbol which will be promoted, if promoted: value of id is changed to
   the promoted symbol
   Returns the promoted type if promotion occurred, otherwise the current type */
static Type cg_unary_expression_promote(Parser* p, SymbolId* id) {
    Type type = symtab_get_type(p, *id);
    if (type_integral(type)) {
        Type promoted = type_promotion(type);
        /* Promoted type != old type means IL must be
           generated to perform the promotion,
           if it is the same nothing needs to be done */
        if (!type_equal(type, promoted)) {
            SymbolId promoted_id = cg_make_temporary(p, promoted);
            cgil_mtc(p, promoted_id, *id);
            *id = promoted_id;
            return promoted;
        }
    }
    return type;
}

static SymbolId cg_unary_expression(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(unary_expression);

    SymbolId result_id;
    if (parse_node_count_child(node) == 1) {
        result_id = cg_postfix_expression(p, parse_node_child(node, 0));
    }
    else {
        TokenId tok_id = parse_node_token_id(parse_node_child(node, 0));
        const char* token = parser_get_token(p, tok_id);

        /* Prefix increment, decrement */
        if (strequ(token, "++")) {
            result_id = cg_unary_expression(p, parse_node_child(node, 1));
            cg_unary_expression_promote(p, &result_id);

            Type type = symtab_get_type(p, result_id);
            if (type_is_arithmetic(&type)) {
                parser_output_il(p, "add $s,$s,1\n", result_id, result_id);
            }
            else if (type_is_pointer(&type)) {
                parser_output_il(
                        p,
                        "add $s,$s,$d\n",
                        result_id,
                        result_id,
                        type_bytes(type_point_to(&type)));
            }
            else {
                ERRMSG("Invalid operands for prefix ++ operator\n");
            }
        }
        else if (strequ(token, "--")) {
            result_id = cg_unary_expression(p, parse_node_child(node, 1));
            cg_unary_expression_promote(p, &result_id);

            Type type = symtab_get_type(p, result_id);
            if (type_is_arithmetic(&type)) {
                parser_output_il(p, "sub $s,$s,1\n", result_id, result_id);
            }
            else if (type_is_pointer(&type)) {
                parser_output_il(
                        p,
                        "sub $s,$s,$d\n",
                        result_id,
                        result_id,
                        type_bytes(type_point_to(&type)));
            }
            else {
                ERRMSG("Invalid operands for postfix -- operator\n");
            }
        }
        else if (strequ(token, "sizeof")) {
            /* Incomplete */
        }
        else {
            /* unary-operator */
            SymbolId operand_1 =
                cg_cast_expression(p, parse_node_child(node, 1));
            Type result_type = cg_unary_expression_promote(p, &operand_1);

            char op = token[0]; /* Unary operator only single token */
            switch (op) {
                case '&':
                    type_inc_pointer(&result_type);
                    Symbol* sym = symtab_get(p, operand_1);

                    /* C99 6.5.3.2.3 */

                    if (symbol_class(sym) == sl_access) {
                        /* The operation of &*p, (p is ptr) does nothing */
                        if (!symid_valid(symbol_ptr_index(sym))) {
                            result_id = symbol_ptr_sym(sym);
                        }
                        /* &p[n] is p + n */
                        else {
                            result_id = cg_make_temporary(p, result_type);
                            parser_output_il(
                                    p,
                                    "add $s,$s,$s\n",
                                    result_id,
                                    symbol_ptr_sym(sym),
                                    symbol_ptr_index(sym));
                        }
                    }
                    else {
                        result_id = cg_make_temporary(p, result_type);
                        parser_output_il(
                                p, "mad $s,$s\n", result_id, operand_1);
                    }
                    break;
                case '*':
                    type_dec_indirection(&result_type);
                    result_id = symtab_add(p, -1, result_type, vc_lval);
                    /* Obtain the pointer by converting a non Lvalue
                       **p
                       ^~~ Underlined to a non Lvalue so can be dereferenced by
                       ^   indicated */
                    symbol_sl_access(
                            symtab_get(p, result_id),
                            cg_nlval(p, operand_1),
                            symid_invalid);
                    break;
                case '-':
                    result_id = cg_make_temporary(p, result_type);
                    /* Negative by multiplying by -1 */
                    cgil_mul(p, result_id, operand_1, -1);
                    break;
                case '!':
                    result_id = cg_make_temporary(p, result_type);
                    cgil_not(p, result_id, operand_1);
                    break;
                default:
                    ASSERT(0, "Unimplemented");
            }
        }
    }

    DEBUG_CG_FUNC_END();
    return result_id;
}

static SymbolId cg_cast_expression(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(cast_expression);

    SymbolId sym_id;
    if (parse_node_count_child(node) == 1) {
        sym_id = cg_unary_expression(p, parse_node_child(node, 0));
    }
    else {
        ASSERT(parse_node_count_child(node) == 2, "Incorrect child count");
        Type type = cg_type_name_extract(p, parse_node_child(node, 0));
        sym_id = cg_cast_expression(p, parse_node_child(node, 1));

        if (!type_equal(type, symtab_get_type(p, sym_id))) {
            SymbolId temp_id = cg_make_temporary(p, type);
            cgil_mtc(p, temp_id, sym_id);
            sym_id = temp_id;
        }
    }

    DEBUG_CG_FUNC_END();
    return sym_id;
}

/* Traverses down multiplicative_expression and generates code in preorder */
static SymbolId cg_multiplicative_expression(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(multiplicative_expression);

    SymbolId id = cg_expression_op2(p, node, cg_cast_expression);

    DEBUG_CG_FUNC_END();
    return id;
}

/* Traverses down additive_expression and generates code in preorder */
static SymbolId cg_additive_expression(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(additive_expression);

    SymbolId op1 = cg_multiplicative_expression(p, parse_node_child(node, 0));
    ParseNode* operator = parse_node_child(node, 1);
    node = parse_node_child(node, 2);
    while (node != NULL) {
        SymbolId op2 =
            cg_multiplicative_expression(p, parse_node_child(node, 0));
        SymbolId op_temp;

        Type op1_t = symtab_get_type(p, op1);
        Type op2_t = symtab_get_type(p, op2);

        /* Addition */
        char operator_char =
            parser_get_token(p, parse_node_token_id(operator))[0];
        if (operator_char == '+') {
            if (type_is_arithmetic(&op1_t) && type_is_arithmetic(&op2_t)) {
                Type com_type = cg_com_type_lr(p, &op1, &op2);
                op_temp = cg_make_temporary(p, com_type);
                parser_output_il(p, "add $s,$s,$s\n",
                        op_temp, cg_nlval(p, op1), cg_nlval(p, op2));
            }
            /* One pointer, one arithmetic */
            else if ((type_is_pointer(&op1_t) || type_array(&op1_t)) &&
                    type_is_arithmetic(&op2_t)) {
                SymbolId ptr_id = cg_nlval(p, op1);
                op_temp = cg_make_temporary(p, symtab_get_type(p, ptr_id));

                SymbolId byte_offset = cg_byte_offset(
                        p,
                        cg_nlval(p, op2),
                        type_bytes(type_element(&op1_t)));
                parser_output_il(p, "add $s,$s,$s\n",
                        op_temp, ptr_id, byte_offset);

            }
            else if (type_is_arithmetic(&op1_t) &&
                    (type_is_pointer(&op2_t) || type_is_arithmetic(&op1_t))) {
                SymbolId ptr_id = cg_nlval(p, op2);
                op_temp = cg_make_temporary(p, symtab_get_type(p, ptr_id));
                SymbolId byte_offset = cg_byte_offset(
                        p,
                        cg_nlval(p, op1),
                        type_bytes(type_element(&op2_t)));
                parser_output_il(p, "add $s,$s,$s\n",
                        op_temp, ptr_id, byte_offset);
            }
            else {
                ERRMSG("Invalid operands for binary + operator\n");
                return symid_invalid;
            }
        }
        /* Subtraction */
        else if (operator_char == '-') {
            if (type_is_arithmetic(&op1_t) && type_is_arithmetic(&op2_t)) {
                Type com_type = cg_com_type_lr(p, &op1, &op2);
                op_temp = cg_make_temporary(p, com_type);
                parser_output_il(p, "sub $s,$s,$s\n",
                        op_temp, cg_nlval(p, op1), cg_nlval(p, op2));
            }
            /* pointer - pointer */
            else if ((type_is_pointer(&op1_t) || type_array(&op1_t)) &&
                    (type_is_pointer(&op2_t) || type_array(&op2_t))) {
                SymbolId ptr1_id = cg_nlval(p, op1);
                SymbolId ptr2_id = cg_nlval(p, op2);

                op_temp = cg_make_temporary(p, type_ptrdiff);
                parser_output_il(p, "sub $s,$s,$s\n",
                        op_temp, ptr1_id, ptr2_id);
                parser_output_il(p, "div $s,$s,$d\n",
                        op_temp, op_temp, type_bytes(type_element(&op1_t)));
            }
            /* pointer - arithmetic */
            else if ((type_is_pointer(&op1_t) || type_array(&op1_t)) &&
                    type_is_arithmetic(&op2_t)) {
                SymbolId ptr_id = cg_nlval(p, op1);
                op_temp = cg_make_temporary(p, symtab_get_type(p, ptr_id));

                SymbolId byte_offset = cg_byte_offset(
                        p,
                        cg_nlval(p, op2),
                        type_bytes(type_element(&op1_t)));
                parser_output_il(p, "sub $s,$s,$s\n",
                        op_temp, ptr_id, byte_offset);
            }
            else {
                ERRMSG("Invalid operands for binary - operator\n");
                return symid_invalid;
            }
        }
        else {
            ASSERT(0, "Unrecognized operator char");
        }

        op1 = op_temp;
        operator = parse_node_child(node, 1);
        node = parse_node_child(node, 2);
    }
    ASSERT(operator == NULL,
            "Trailing operator without multiplicative-expression");

    DEBUG_CG_FUNC_END();
    return op1;
}

static SymbolId cg_shift_expression(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(shift_expression);

    SymbolId sym_id = cg_additive_expression(p, parse_node_child(node, 0));

    /* Incomplete */

    DEBUG_CG_FUNC_END();
    return sym_id;
}

static SymbolId cg_relational_expression(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(relational_expression);

    SymbolId operand_1 = cg_shift_expression(p, parse_node_child(node, 0));

    ParseNode* operator = parse_node_child(node, 1);
    node = parse_node_child(node, 2);
    while (node != NULL) {
        SymbolId operand_2 =
            cg_shift_expression(p, parse_node_child(node, 0));

        cg_com_type_lr(p, &operand_1, &operand_2);
        SymbolId operand_temp = cg_make_temporary(p, type_int);

        const char* operator_token = parse_node_token(p, operator);
        if (strequ(operator_token, "<")) {
            cgil_cl(p, operand_temp, operand_1, operand_2);
        }
        else if (strequ(operator_token, "<=")) {
            cgil_cle(p, operand_temp, operand_1, operand_2);
        }
        else if (strequ(operator_token, ">")) {
            cgil_cl(p, operand_temp, operand_2, operand_1);
        }
        else {
            ASSERT(strequ(operator_token, ">="), "Invalid token");
            cgil_cle(p, operand_temp, operand_2, operand_1);
        }

        operand_1 = operand_temp;
        operator = parse_node_child(node, 1);
        node = parse_node_child(node, 2);
    }
    ASSERT(operator == NULL,
            "Trailing operator without shift-expression");

    DEBUG_CG_FUNC_END();
    return operand_1;
}

static SymbolId cg_equality_expression(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(equality_expression);

    SymbolId id =
        cg_expression_op2t(p, node, cg_relational_expression, &type_int);

    DEBUG_CG_FUNC_END();
    return id;
}

static SymbolId cg_and_expression(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(and_expression);

    SymbolId sym_id = cg_equality_expression(p, parse_node_child(node, 0));

    /* Incomplete */

    DEBUG_CG_FUNC_END();
    return sym_id;
}

static SymbolId cg_exclusive_or_expression(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(exclusive_or_expression);

    SymbolId sym_id = cg_and_expression(p, parse_node_child(node, 0));

    /* Incomplete */

    DEBUG_CG_FUNC_END();
    return sym_id;
}

static SymbolId cg_inclusive_or_expression(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(inclusive_or_expression);

    SymbolId sym_id = cg_exclusive_or_expression(p, parse_node_child(node, 0));

    /* Incomplete */

    DEBUG_CG_FUNC_END();
    return sym_id;
}

static SymbolId cg_logical_and_expression(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(logical_and_expression);

    SymbolId result_id;
    if (parse_node_count_child(node) == 1) {
        /* Not a logical and (&&), do nothing */
        result_id = cg_inclusive_or_expression(p, parse_node_child(node, 0));
    }
    else {
        /* Compute short-circuit logical and */
        SymbolId label_false_id = cg_make_label(p);
        SymbolId label_end_id = cg_make_label(p);
        result_id = cg_make_temporary(p, type_int);
        do {
            SymbolId sym_id =
                cg_inclusive_or_expression(p, parse_node_child(node, 0));
            cgil_jz(p, label_false_id, sym_id);

            node = parse_node_child(node, 1);
        }
        while (node != NULL);

        /* True */
        cgil_movsi(p, result_id, 1);
        cgil_jmp(p, label_end_id);
        /* False */
        cgil_lab(p, label_false_id);
        cgil_movsi(p, result_id, 0);

        cgil_lab(p, label_end_id);
    }

    DEBUG_CG_FUNC_END();
    return result_id;
}

static SymbolId cg_logical_or_expression(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(logical_or_expression);

    SymbolId result_id;
    if (parse_node_count_child(node) == 1) {
        /* Not a logical or (||), do nothing */
        result_id = cg_logical_and_expression(p, parse_node_child(node, 0));
    }
    else {
        /* Compute short-circuit logical or */
        SymbolId label_true_id = cg_make_label(p);
        SymbolId label_end_id = cg_make_label(p);
        result_id = cg_make_temporary(p, type_int);
        do {
            SymbolId sym_id =
                cg_logical_and_expression(p, parse_node_child(node, 0));
            cgil_jnz(p, label_true_id, sym_id);

            node = parse_node_child(node, 1);
        }
        while (node != NULL);

        /* False */
        cgil_movsi(p, result_id, 0);
        cgil_jmp(p, label_end_id);
        /* True */
        cgil_lab(p, label_true_id);
        cgil_movsi(p, result_id, 1);

        cgil_lab(p, label_end_id);
    }


    DEBUG_CG_FUNC_END();
    return result_id;
}

static SymbolId cg_conditional_expression(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(conditional_expression);

    SymbolId sym_id = cg_logical_or_expression(p, parse_node_child(node, 0));

    /* Incomplete */

    DEBUG_CG_FUNC_END();
    return sym_id;
}

static SymbolId cg_assignment_expression(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(assignment_expression);

    SymbolId opl_id;
    if (parse_node_count_child(node) == 1) {
        opl_id = cg_conditional_expression(p, parse_node_child(node, 0));
    }
    else {
        ASSERT(parse_node_count_child(node) == 3, "Expected 3 children");
        SymbolId opr_id =
            cg_assignment_expression(p, parse_node_child(node, 2));

        opl_id = cg_unary_expression(p, parse_node_child(node, 0));

        cg_com_type_rtol(p, opl_id, &opr_id);
        const char* token = parse_node_token(p, parse_node_child(node, 1));
        if (strequ(token, "=")) {
            cgil_movss(p, opl_id, opr_id);
        }
        else {
            const char* ins;
            if (strequ(token, "*=")) {
                ins = "mul";
            }
            else if (strequ(token, "/=")) {
                ins = "div";
            }
            else if (strequ(token, "%=")) {
                ins = "mod";
            }
            else if (strequ(token, "+=")) {
                ins = "add";
            }
            else if (strequ(token, "-=")) {
                ins = "sub";
            }
            else {
                /* Incomplete */
                ASSERT(0, "Unimplemented");
            }
            parser_output_il(p, "$i $s,$s,$s\n",
                    ins, opl_id, cg_nlval(p, opl_id), cg_nlval(p, opr_id));
        }
    }

    DEBUG_CG_FUNC_END();
    return opl_id;
}

static SymbolId cg_expression(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(expression);

    SymbolId sym_id = cg_assignment_expression(p, parse_node_child(node, 0));

    /* Incomplete */

    DEBUG_CG_FUNC_END();
    return sym_id;
}

/* Generates il declaration */
static void cg_declaration(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(declaration);

    ParseNode* declspec = parse_node_child(node, 0);
    /* NOTE: this only handles init-declarator-list of size 1 */
    ParseNode* initdecllist = parse_node_child(node, 1);
    ParseNode* initdecl = parse_node_child(initdecllist, 0);
    ParseNode* declarator = parse_node_child(initdecl, 0);

    /* Declarator */
    SymbolId lval_id = cg_extract_symbol(p, declspec, declarator);
    parser_output_il(p, "def $t $s\n", lval_id, lval_id);

    /* Initializer if it exists */
    if (parse_node_count_child(initdecl) == 2) {
        ParseNode* initializer = parse_node_child(initdecl, 1);
        SymbolId rval_id = cg_initializer(p, initializer);
        cg_com_type_rtol(p, lval_id, &rval_id);
        cgil_movss(p, lval_id, rval_id);
    }

    DEBUG_CG_FUNC_END();
}

/* Generates il type and identifier for parameters of parameter list */
static void cg_parameter_list(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(parameter_list);
    ASSERT(parse_node_count_child(node) >= 1,
            "Expected at least 1 children of node");

    /* No comma ahead of first parameter */
    cg_parameter_declaration(p, parse_node_child(node, 0));

    node = parse_node_child(node, 1);
    while (node != NULL) {
        ASSERT(parse_node_count_child(node) >= 1,
                "Expected at least 1 children of node");
        parser_output_il(p, ",");
        cg_parameter_declaration(p, parse_node_child(node, 0));
        node = parse_node_child(node, 1);
    }

    DEBUG_CG_FUNC_END();
}

/* Generates il type and identifier for parameter declaration */
static void cg_parameter_declaration(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(parameter_declaration);
    ASSERT(parse_node_count_child(node) == 2, "Expected 2 children of node");

    ParseNode* declspec = parse_node_child(node, 0);
    ParseNode* declarator = parse_node_child(node, 1);
    SymbolId id = cg_extract_symbol(p, declspec, declarator);

    parser_output_il(p, "$t $s", id, id);

    DEBUG_CG_FUNC_END();
}

/* Generates il for initializer
   Returned SymbolId corresponds to the result of the initializer expression */
static SymbolId cg_initializer(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(initializer);

    SymbolId sym_id = cg_assignment_expression(p, parse_node_child(node, 0));

    /* Incomplete */

    DEBUG_CG_FUNC_END();
    return sym_id;
}

static void cg_statement(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(statement);

    ParseNode* child = parse_node_child(node, 0);
    switch (parse_node_type(child)) {
        case st_expression_statement:
            cg_expression_statement(p, child);
            break;
        case st_selection_statement:
            /* If generates il itself */
            break;
        case st_jump_statement:
            cg_jump_statement(p, child);
            break;
        default:
            /* Incomplete */
            break;
    }

    DEBUG_CG_FUNC_END();
}

static void cg_block_item(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(block_item);

    ParseNode* child = parse_node_child(node, 0);
    if (parse_node_type(child) == st_declaration) {
        cg_declaration(p, child);
    }
    else {
        cg_statement(p, child);
    }

    DEBUG_CG_FUNC_END();
}

static void cg_expression_statement(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(expression_statement);

    cg_expression(p, parse_node_child(node, 0));

    DEBUG_CG_FUNC_END();
}

static void cg_jump_statement(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(jump_statement);

    TokenId jmp_id = parse_node_token_id(parse_node_child(node, 0));
    const char* jmp_token = parser_get_token(p, jmp_id);

    if (strequ(jmp_token, "continue")) {
        SymbolId body_end_id = symtab_last_cat(p, sc_lab_loopbodyend);
        cgil_jmp(p, body_end_id);
    }
    else if (strequ(jmp_token, "break")) {
        SymbolId end_id = symtab_last_cat(p, sc_lab_loopend);
        cgil_jmp(p, end_id);
    }
    else if (strequ(jmp_token, "return")) {
        SymbolId exp_id = cg_expression(p, parse_node_child(node, 1));
        cgil_ret(p, exp_id);
    }

    DEBUG_CG_FUNC_END();
}

/* Extracts type specifiers from parse tree of following format:
   a-node
   - t-node
   - a-node
     - t-node
     - a-node
       - ...
   Where a-node is some node which is traversed, t-node contains information
   to extract, e.g., type specifiers. If t-node is not a recognized type, it is
   ignored. */
static TypeSpecifiers cg_extract_type_specifiers(Parser* p, ParseNode* node) {
    /* Index of string in arrangement corresponds to type specifier at index */
    char* arrangement[] = {
        "void",
        "char", "signed char",
        "unsigned char",
        "short", "signed short", "short int", "signed short int",
        "unsigned short", "unsigned short int",
        "int", "signed", "signed int",
        "unsigned", "unsigned int",
        "long", "signed long", "long int", "signed long int",
        "unsigned long", "unsigned long int",
        "long long", "signed long long", "long long int", "signed long long int",
        "unsigned long long", "unsigned long long int",
        "float",
        "double",
        "long double",
    };
    int count = ARRAY_SIZE(arrangement);
    TypeSpecifiers typespec[] = {
        ts_void,
        ts_i8, ts_i8,
        ts_u8,
        ts_i16, ts_i16, ts_i16, ts_i16,
        ts_u16, ts_u16,
        ts_i32, ts_i32, ts_i32,
        ts_u32, ts_u32,
        ts_i32_, ts_i32_, ts_i32_, ts_i32_,
        ts_u32_, ts_u32_,
        ts_i64, ts_i64, ts_i64, ts_i64,
        ts_u64, ts_u64,
        ts_f32,
        ts_f64,
        ts_f64_,
    };

    /* 22 (max size of arrangements) + 1 for null terminator */
    int i_buf = 0;
    char buf[23];
    /* Walk through the declaration specifier children to
       form the type specifier string */
    while (node != NULL) {
        ASSERT(parse_node_count_child(node) >= 1,
                "Expected at least 1 children of a-node");

        ParseNode* subnode = parse_node_child(node, 0);
        if (parse_node_type(subnode) == st_type_specifier) {
            const char* token =
                parse_node_token(p, parse_node_child(subnode, 0));

            /* Space to separate tokens, no space for before first token */
            if (i_buf != 0) {
                buf[i_buf++] = ' ';
            }

            /* Copy token into buffer */
            int i = 0;
            char c;
            while ((c = token[i]) != '\0') {
                if (i_buf >= 22) {
                    /* Too long, not a match */
                    return ts_none;
                }
                buf[i_buf] = c;
                ++i;
                ++i_buf;
            }
        }
        node = parse_node_child(node, 1);
    }

    buf[i_buf] = '\0';
    /* Match type specifier string to a corresponding type */
    for (int j = 0; j < count; ++j) {
        if (strequ(arrangement[j], buf)) {
            return typespec[j];
        }
    }

    return ts_none;
}

/* Extracts pointer count from parse tree of following format:
   a-node
   - pointer
     - pointer
   - b-node
   where a-node is some node whose children is examined, if its first
   child is a pointer, it will count the total number of pointers attached
   by descending down the first child */
static int cg_extract_pointer(ParseNode* node) {
    ASSERT(parse_node_count_child(node) >= 1,
            "Expected at least 1 children of node");

    ParseNode* child = parse_node_child(node, 0);

    /* No pointer node */
    if (parse_node_type(child) != st_pointer) {
        return 0;
    }

    int count = 0;
    while (child != NULL) {
        ++count;
        child = parse_node_child(child, 0);
    }
    return count;
}

/* Extracts type from type-name node */
static Type cg_type_name_extract(Parser* p, ParseNode* node) {
    ASSERT(parse_node_type(node) == st_type_name, "Incorrect node type");
    ASSERT(parse_node_count_child(node) >= 1,
            "Expected at least 1 children of node");
    ParseNode* spec_qual_list = parse_node_child(node, 0);
    TypeSpecifiers ts = cg_extract_type_specifiers(p, spec_qual_list);
    ParseNode* abstract_decl = parse_node_child(node, 1);
    int pointers = 0;
    if (abstract_decl != NULL) {
        pointers = cg_extract_pointer(abstract_decl);
    }

    Type type;
    type_construct(&type, ts, pointers);
    return type;
}

/* Adds symbol (type + token) to symbol table
   Expects declaration-specifiers node and declarator node*/
static SymbolId cg_extract_symbol(Parser* p,
        ParseNode* declspec, ParseNode* declarator) {
    TypeSpecifiers ts = cg_extract_type_specifiers(p, declspec);
    int pointers = cg_extract_pointer(declarator);

    ParseNode* dirdeclarator = parse_node_child(declarator, -1);

    /* Identifier */
    ParseNode* identifier = parse_node_child(dirdeclarator, 0);
    ParseNode* identifier_tok = parse_node_child(identifier, 0);
    TokenId tok_id = parse_node_token_id(identifier_tok);

    Type type;
    type_construct(&type, ts, pointers);

    /* Arrays, e.g., [10][90][11] */
    dirdeclarator = parse_node_child(dirdeclarator, 1);
    while (dirdeclarator != NULL &&
            parse_node_type(dirdeclarator) == st_direct_declarator) {
        ParseNode* tok_node = parse_node_child(dirdeclarator, 0);
        const char* tok = parse_node_token(p, tok_node);
        /* May be parameter-type-list, which we ignore */
        if (strequ("(", tok)) {
            goto loop;
        }
        if (!strequ("[", tok)) {
            ERRMSG("Expected ( or [\n");
            return symid_invalid;
        }

        /* FIXME assuming assignment-expression is at index 1, this is only
           the case for some productions of direct-declarator */
        /* Evaluates to number of elements for this dimension */
        SymbolId dim_count_id =
            cg_assignment_expression(p, parse_node_child(dirdeclarator, 1));
        Symbol* sym = symtab_get(p, dim_count_id);
        int count = strtoi(symbol_token(p, sym));

        type_add_dimension(&type, count);
loop:
        dirdeclarator = parse_node_child(dirdeclarator, -1);
    }

    return symtab_add(p, tok_id, type, vc_lval);
}

/* Generates il for function-definition excluding compound-statement */
static void cg_function_signature(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(function_definition);

    ParseNode* declspec = parse_node_child(node, 0);
    ParseNode* declarator = parse_node_child(node, 1);

    /* name is function name, type is return type */
    SymbolId func_id = cg_extract_symbol(p, declspec, declarator);
    parser_output_il(p, "func $s,$t,", func_id, func_id);

    ParseNode* dirdeclarator = parse_node_child(declarator, -1);
    ParseNode* dirdeclarator2 = parse_node_child(dirdeclarator, 1);

    const char* tok = parse_node_token(p, parse_node_child(dirdeclarator2, 0));
    if (!strequ( "(", tok)) {
        ERRMSG("Expected (' to begin parameter-type-list");
        return;
    }
    ParseNode* paramtypelist = parse_node_child(dirdeclarator2, 1);
    ParseNode* paramlist = parse_node_child(paramtypelist, 0);
    cg_parameter_list(p, paramlist);
    parser_output_il(p, "\n");

    DEBUG_CG_FUNC_END();
}

/* Creates temporary in symbol table and
   generates the necessary il to create a temporary */
static SymbolId cg_make_temporary(Parser* p, Type type) {
    SymbolId sym_id = symtab_add_temporary(p, type);

    parser_output_il(p, "def $t $s\n", sym_id, sym_id);
    return sym_id;
}

static SymbolId cg_make_label(Parser* p) {
    SymbolId label_id = symtab_add_label(p);

    parser_output_il(p, "def $t $s\n", label_id, label_id);
    return label_id;
}

/* Calls cg_expression_op2t without overriding the type of the operation's
   result */
static SymbolId cg_expression_op2(
        Parser* p,
        ParseNode* node,
        SymbolId (*cg_b_expr)(Parser*, ParseNode*)) {
    return cg_expression_op2t(p, node, cg_b_expr, NULL);
}

/* Generates IL for some production a-expression which is a 2 operand operator
   of the following format:
   a-expression
    - b-expression
    - Operator token
    - a-expression
      - b-expression
      - Operator token
      - a-expression
        - ...

   The SymbolId for the result of each b-expression is obtained by calling
   cg_b_expr
   The IL instruction emitted is the operator's token
   (2nd child of each a-expression)

   result_type: The type of the result of the operator, leave null to use the
                common type
   Returns Symbolid holding the result of the expression */
static SymbolId cg_expression_op2t(
        Parser* p,
        ParseNode* node,
        SymbolId (*cg_b_expr)(Parser*, ParseNode*),
        const Type* result_type) {
    SymbolId op1 = cg_b_expr(p, parse_node_child(node, 0));
    ParseNode* operator = parse_node_child(node, 1);
    node = parse_node_child(node, 2);
    while (node != NULL) {
        SymbolId op2 = cg_b_expr(p, parse_node_child(node, 0));
        Type com_type = cg_com_type_lr(p, &op1, &op2);

        Type temporary_type;
        if (result_type == NULL) {
            temporary_type = com_type;
        }
        else {
            temporary_type = *result_type;
        }
        SymbolId op_temp = cg_make_temporary(p, temporary_type);
        const char* il_ins =
            parser_get_token(p, parse_node_token_id(operator));
        parser_output_il(p, "$i $s,$s,$s\n",
                il_ins, op_temp, cg_nlval(p, op1), cg_nlval(p, op2));

        op1 = op_temp;
        operator = parse_node_child(node, 1);
        node = parse_node_child(node, 2);
    }
    ASSERT(operator == NULL, "Trailing operator without b-expression");

    return op1;
}

/* Generates the code to convert an offset to a byte offset, for example:
   int* a;
   a + 5; <-- The offset here 5, needs to be converted to a byte offset
              to offset the pointer by elements. 5 * sizeof(int)
   offset: The value which will be multiplied by the size of the element
           to obtain the byte offset
   ele_bytet: Number of bytes for element */
static SymbolId cg_byte_offset(Parser* p, SymbolId offset, int elem_byte) {
    SymbolId byte_offset = cg_make_temporary(p, type_ptroffset);
    cg_com_type_rtol(p, byte_offset, &offset);
    parser_output_il(
            p, "mul $s,$s,$d\n", byte_offset, offset, elem_byte);
    return byte_offset;
}

/* If left and right are different types, right operand is converted
   to type of left operand and the value of the provided right operand
   is changed to a temporary which has the same type as the left operand */
static void cg_com_type_rtol(Parser* p, SymbolId opl_id, SymbolId* opr_id) {
    Type opl_type = symtab_get_type(p, opl_id);
    Type opr_type = symtab_get_type(p, *opr_id);
    if (!type_equal(opl_type, opr_type)) {
        SymbolId temp_id = cg_make_temporary(p, opl_type);
        cgil_mtc(p, temp_id, *opr_id);
        *opr_id = temp_id;
    }
}

/* A common type is calculated with op1 and op2, if necessary both
   are converted to the common type and the provided op1 op2 changed
   to the id of the temporaries holding op1 and op2 as common type
   The common type is returned */
static Type cg_com_type_lr(Parser* p, SymbolId* op1_id, SymbolId* op2_id) {
    Type com_type = type_common(
            type_promotion(symtab_get_type(p, *op1_id)),
            type_promotion(symtab_get_type(p, *op2_id)));
    if (!type_equal(symtab_get_type(p, *op1_id), com_type)) {
        SymbolId promoted_id = cg_make_temporary(p, com_type);
        cgil_mtc(p, promoted_id, *op1_id);
        *op1_id = promoted_id;
    }
    if (!type_equal(symtab_get_type(p, *op2_id), com_type)) {
        SymbolId promoted_id = cg_make_temporary(p, com_type);
        cgil_mtc(p, promoted_id, *op2_id);
        *op2_id = promoted_id;
    }
    return com_type;
}

/* Generates the instructions to convert the given symbol to a non Lvalue if
   it is not already one
   Returns the SymbolId of the converted non Lvalue, or the provided SymbolId
   if already a non Lvalue */
static SymbolId cg_nlval(Parser* p, SymbolId symid) {
    if (symtab_get_valcat(p, symid) == vc_nlval) {
        return symid;
    }

    /* FIXME the returned symbol is still of value category lval
       It is fine for now since it is converted to nlval right before
       it is used to emit IL, thus it is never checked */

    Symbol* sym = symtab_get(p, symid);
    Type type = symbol_type(sym);
    switch (symbol_class(sym)) {
        case sl_normal:
            /* Array decay into pointer */
            if (type_array(&type)) {
                SymbolId temp_id =
                    cg_make_temporary(p, type_array_as_pointer(&type));
                parser_output_il(p, "mad $s,$s\n", temp_id, symid);
                return temp_id;
            }
            return symid;
        case sl_access:
            {
                /* 6.3.2.1.2 Convert to value stored in object */
                SymbolId temp_id = cg_make_temporary(p, symbol_type(sym));
                SymbolId index = symbol_ptr_index(sym);
                if (symid_valid(index)) {
                    parser_output_il(p, "mfi $s,$s,$s\n",
                            temp_id, symbol_ptr_sym(sym), index);
                }
                else {
                    parser_output_il(p, "mfi $s,$s,0\n",
                            temp_id, symbol_ptr_sym(sym));
                }
                return temp_id;
            }
        default:
            ASSERT(0, "Unimplemented");
            return symid;
    }
}

static void cgil_cl(Parser* p, SymbolId dest, SymbolId op1, SymbolId op2) {
    parser_output_il(p, "cl $s,$s,$s\n",
            dest, cg_nlval(p, op1), cg_nlval(p, op2));
}

static void cgil_cle(Parser* p, SymbolId dest, SymbolId op1, SymbolId op2) {
    parser_output_il(p, "cle $s,$s,$s\n",
            dest, cg_nlval(p, op1), cg_nlval(p, op2));
}

static void cgil_jmp(Parser* p, SymbolId lab) {
    parser_output_il(p, "jmp $s\n", lab);
}

static void cgil_jnz(Parser* p, SymbolId lab, SymbolId op1) {
    parser_output_il(p, "jnz $s,$s\n", lab, cg_nlval(p, op1));
}

static void cgil_jz(Parser* p, SymbolId lab, SymbolId op1) {
    parser_output_il(p, "jz $s,$s\n", lab, cg_nlval(p, op1));
}

static void cgil_lab(Parser* p, SymbolId lab) {
    parser_output_il(p, "lab $s\n", lab);
}

static void cgil_movss(Parser* p, SymbolId dest, SymbolId src) {
    Symbol* sym_dest = symtab_get(p, dest);
    switch (symbol_class(sym_dest)) {
        case sl_normal:
            parser_output_il(p, "mov $s,$s\n", dest, cg_nlval(p, src));
            break;
        case sl_access:
            {
                SymbolId index = symbol_ptr_index(sym_dest);
                if (symid_valid(index)) {
                    parser_output_il(p, "mti $s,$s,$s\n",
                            symbol_ptr_sym(sym_dest), index, cg_nlval(p, src));
                }
                else {
                    parser_output_il(p, "mti $s,0,$s\n",
                            symbol_ptr_sym(sym_dest), cg_nlval(p, src));
                }
            }
            break;
        default:
            ASSERT(0, "Unimplemented");
    }
}

static void cgil_movsi(Parser* p, SymbolId dest, int src) {
    Symbol* sym_dest = symtab_get(p, dest);
    switch (symbol_class(sym_dest)) {
        case sl_normal:
            parser_output_il(p, "mov $s,$d\n", dest, src);
            break;
        case sl_access:
            {
                SymbolId index = symbol_ptr_index(sym_dest);
                if (symid_valid(index)) {
                    parser_output_il(p, "mti $s,$s,$d\n",
                            symbol_ptr_sym(sym_dest), index, src);
                }
                else {
                    parser_output_il(p, "mti $s,0,$d\n",
                            symbol_ptr_sym(sym_dest), src);
                }
            }
            break;
        default:
            ASSERT(0, "Unimplemented");
    }
}

static void cgil_mtc(Parser* p, SymbolId dest, SymbolId src) {
    parser_output_il(p, "mtc $s,$s\n", dest, cg_nlval(p, src));
}

static void cgil_mul(Parser* p, SymbolId dest, SymbolId op1, int op2) {
    parser_output_il(p, "mul $s,$s,$d\n", dest, cg_nlval(p, op1), op2);
}

static void cgil_not(Parser* p, SymbolId dest, SymbolId op1) {
    parser_output_il(p, "not $s,$s\n", dest, cg_nlval(p, op1));
}

static void cgil_ret(Parser* p, SymbolId op1) {
    parser_output_il(p, "ret $s\n", cg_nlval(p, op1));
}

/* ============================================================ */
/* Initialization and configuration */

/* The index of the option's string in the string array
   is the index of the pointer for the variable corresponding to the option

   SWITCH_OPTION(option string, variable to set)
   Order by option string, see strbinfind for ordering requirements */
#define SWITCH_OPTIONS                                                    \
    SWITCH_OPTION(-dprint-buffers, g_debug_print_buffers)                 \
    SWITCH_OPTION(-dprint-cg-recursion, g_debug_print_cg_recursion)       \
    SWITCH_OPTION(-dprint-parse-recursion, g_debug_print_parse_recursion) \
    SWITCH_OPTION(-dprint-parse-tree, g_debug_print_parse_tree)           \
    SWITCH_OPTION(-dprint-symtab, g_debug_print_symtab)

#define SWITCH_OPTION(str__, var__) #str__,
const char* option_switch_str[] = {SWITCH_OPTIONS};
#undef SWITCH_OPTION
#define SWITCH_OPTION(str__, var__) &var__,
int* option_switch_value[] = {SWITCH_OPTIONS};
#undef SWITCH_OPTION

/* Parses cli args and processes them */
/* NOTE: will not clean up file handles at exit */
/* Returns non zero if error */
static int handle_cli_arg(Parser* p, int argc, char** argv) {
    int rt_code = 0;
    /* Skip first argv since it is path */
    for (int i = 1; i < argc; ++i) {
        /* Handle switch options */
        int i_switch = strbinfind(
                argv[i],
                strlength(argv[i]),
                option_switch_str,
                ARRAY_SIZE(option_switch_str));
        if (i_switch >= 0) {
            *option_switch_value[i_switch] = 1;
            continue;
        }

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
        else {
            if (p->rf != NULL) {
                /* Incorrect flag */
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
    int rt_code = 0;
    Parser p = {.rf = NULL, .of = NULL, .line_num = 1, .char_num = 1};
    p.ecode = ec_noerr;

    rt_code = handle_cli_arg(&p, argc, argv);
    if (rt_code != 0) {
        goto cleanup;
    }
    if (p.rf == NULL) {
        ERRMSG("No input file\n");
        goto cleanup;
    }
    if (p.of == NULL) {
        /* Default to opening imm2 */
        p.of = fopen("imm2", "w");
        if (p.of == NULL) {
            ERRMSG("Failed to open output file\n");
            rt_code = 1;
            goto cleanup;
        }
    }

    symtab_push_scope(&p);
    if (!parse_function_definition(&p, &p.parse_node_root)) {
        parser_set_error(&p, ec_syntaxerr);
        ERRMSGF("Failed to build parse tree. Line %d, around char %d\n",
                p.last_line_num, p.last_char_num);
    }
    symtab_pop_scope(&p);
    ASSERT(p.i_scope == 0, "Scopes not empty on parse end");
    for (int i = 0; i < sc_count; ++i) {
        ASSERTF(p.i_symtab_cat[i] == 0,
                "Symbol category stack %d not empty on parse end", i);
    }

    /* Debug options */
    if (g_debug_print_parse_tree) {
        LOG("Remaining ");
        debug_print_parse_tree(&p);
    }

    if (parser_has_error(&p)) {
        ErrorCode ecode = parser_get_error(&p);
        ERRMSGF("Error during parsing: %d %s\n", ecode, errcode_str[ecode]);
        rt_code = ecode;
    }

cleanup:
    if (p.rf != NULL) {
        fclose(p.rf);
    }
    if (p.of != NULL) {
        fclose(p.of);
    }
    return rt_code;
}

