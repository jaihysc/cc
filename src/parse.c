/* Preprocessed c file parser */
/* Generated output is imm2 */

#include <stdio.h>
#include <stdarg.h>
#include "common.h"

#define MAX_TOKEN_LEN 255 /* Excluding null terminator, Tokens is string with no whitespace */
#define MAX_PARSER_BUFFER_LEN 255 /* Excluding null terminator */
#define MAX_PARSE_TREE_NODE 500 /* Maximum nodes in parser parse tree */
#define MAX_PARSE_NODE_CHILD 4 /* Maximum children nodes for a parse tree node */
#define MAX_LEXEME_BUFFER_CHAR 512 /* Max characters in token buffer */
#define MAX_SYMTAB_TOKEN 200 /* Maximum symbols in symbol table in total */

/* ============================================================ */
/* Parser global configuration */

int g_debug_print_parse_recursion = 0;
int g_debug_print_cg_recursion = 0;
int g_debug_print_parse_tree = 0;
int g_debug_print_buffers = 0;

/* ============================================================ */
/* Parser data structure + functions */

typedef int TokenId;
typedef int SymbolId;

typedef struct {
    TokenId tok_id;
    Type type;
} Symbol;

/* Sorted by Annex A */
/* 6.4 Lexical elements */
/* 6.5 Expressions */
/* 6.7 Declarators */
/* 6.8 Statements and blocks */
/* 6.9 External definitions */
#define SYMBOL_TYPES                       \
    SYMBOL_TYPE(identifier)                \
    SYMBOL_TYPE(constant)                  \
    SYMBOL_TYPE(integer_constant)          \
    SYMBOL_TYPE(decimal_constant)          \
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
    SYMBOL_TYPE(type_qualifier)            \
    SYMBOL_TYPE(function_specifier)        \
    SYMBOL_TYPE(declarator)                \
    SYMBOL_TYPE(direct_declarator)         \
    SYMBOL_TYPE(pointer)                   \
    SYMBOL_TYPE(parameter_type_list)       \
    SYMBOL_TYPE(parameter_list)            \
    SYMBOL_TYPE(parameter_declaration)     \
    SYMBOL_TYPE(initializer)               \
                                           \
    SYMBOL_TYPE(statement)                 \
    SYMBOL_TYPE(compound_statement)        \
    SYMBOL_TYPE(block_item_list)           \
    SYMBOL_TYPE(block_item)                \
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

typedef struct {
    /* Index of next available space in parse node buffer */
    int i_node_buf;
    ParseNode* node; /* Node which may be modified */
    int node_childs; /* Children the node has */
    long rf_offset; /* Input file offset */
    char read_buf[MAX_TOKEN_LEN + 1]; /* Token read buffer */
} ParseLocation;

/* pbufexceed: Parser buffer exceeded
   fileposfailed: Change file position indicator failed */
#define ERROR_CODES          \
    ERROR_CODE(noerr)        \
    ERROR_CODE(tokbufexceed) \
    ERROR_CODE(pbufexceed)   \
    ERROR_CODE(syntaxerr)    \
    ERROR_CODE(writefailed)  \
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

typedef struct {
    FILE* rf; /* Input file */
    FILE* of; /* Output file */

    /* Functions set error code only if an error occurred */
    /* By default this is set as ec_noerr */
    ErrorCode ecode;

    /* Token read buffer, used by read_token to store token read */
    /* The read token is kept here, subsequent calls to read_token()
       will return this.
       When consume_token() is called, the next call to read_token()
       will fill this buffer with a new token
       */
    char read_buf[MAX_TOKEN_LEN + 1];

    ParseNode parse_node_buf[MAX_PARSE_TREE_NODE];
    int i_parse_node_buf; /* Points to next available space */

    /* In future: Could save space by seeing if the token is already in buf */
    char token_buf[MAX_LEXEME_BUFFER_CHAR];
    TokenId i_token_buf; /* Points to next available space */

    Symbol symtab[MAX_SYMTAB_TOKEN];
    int i_symtab; /* Points to next available space */
    int symtab_temp_num; /* Number to create unique temporary values */
} Parser;

static void debug_parser_buf_dump(Parser* p);
static void debug_parse_node_walk(
        Parser* p, ParseNode* node,
        char* branch, int i_branch, const int max_branch_len);

/* Sets error code in parser */
static void parser_set_error(Parser* p, ErrorCode ecode) {
    p->ecode = ecode;
    LOGF("Error set: %d\n", ecode);
}

/* Returns error code in parser */
static ErrorCode parser_get_error(Parser* p) {
    return p->ecode;
}

/* Returns 1 if error is set, 0 otherwise */
static int parser_has_error(Parser* p) {
    return p->ecode != ec_noerr;
}

/* Writes provided IL using format string and va_args into output */
static void parser_output_ilf(Parser* p, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (vfprintf(p->of, fmt, args) < 0) {
        parser_set_error(p, ec_writefailed);
    }
    va_end(args);
}

/* Writes provided IL using format string and va_args into output */
static void parser_output_il(Parser* p, const char* txt) {
    parser_output_ilf(p, "%s", txt);
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
    if (p->i_token_buf + strlength(token) + 1 > MAX_LEXEME_BUFFER_CHAR) {
        parser_set_error(p, ec_pbufexceed);
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

    /* Current file position */
    long offset = ftell(p->rf);
    if (offset == -1L) {
        parser_set_error(p, ec_fileposfailed);
    }

    ParseLocation location = {
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
static ParseNode* parser_attach_node(Parser* p, ParseNode* parent, SymbolType st) {
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
            parser_set_error(p, ec_pbufexceed);
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
static ParseNode* parser_attach_token(Parser* p, ParseNode* parent, const char* token) {
    ASSERT(parent != NULL, "Parse tree parent node is null");

    TokenId tok_id = parser_add_token(p, token);
    if (parser_has_error(p)) {
        return NULL;
    }
    return parser_attach_node(p, parent, st_from_token_id(tok_id));
}

/* Prints out contents of parser buffers */
static void debug_parser_buf_dump(Parser* p) {
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

    LOG("Symbol table:\n");
    for (int i = 0; i < p->i_symtab; ++i) {
        Symbol tok = p->symtab[i];
        LOGF("%d %s ", i, type_specifiers_str(tok.type.typespec));
        for (int j = 0; j < tok.type.pointers; ++j) {
            LOG("*");
        }
        LOGF("%s\n", parser_get_token(p, tok.tok_id));
    }
}

static void debug_draw_parse_tree(Parser* p) {
    LOG("Parse tree:\n");
    /* Draw tree with first node since it is the top node */
    if (p->i_parse_node_buf > 0) {
        /* 2 characters per node */
        int max_branch = MAX_PARSE_TREE_NODE * 2;

        char branch[max_branch];
        branch[0] = '\0';
        debug_parse_node_walk(p, p->parse_node_buf, branch, 0, max_branch);
    }
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

/* Moves to the last (higher) scope */
/* static void symtab_ascend(Parser* p) {
} */

/* Moves to a new scope within the current scope */
/* static void symtab_descend(Parser* p) {
} */

/* Finds provided token in symbol table, closest scope first
   -1 if not found */
static SymbolId symtab_find(Parser* p, TokenId tok_id) {
    char* lex_1 = parser_get_token(p, tok_id);
    for (int i = 0; i < p->i_symtab; ++i) {
        char* lex_2 = parser_get_token(p, p->symtab[i].tok_id);
        if (strequ(lex_1, lex_2)) {
            return i;
        }
    }
    return -1;
}

/* Returns token for symbol */
static const char* symtab_get_token(Parser* p, SymbolId sym_id) {
    ASSERT(sym_id >= 0, "Token id out of range");
    ASSERT(sym_id < p->i_symtab, "Token id out of range");
    return p->token_buf + p->symtab[sym_id].tok_id;
}

/* Returns data type for symbol */
static Type symtab_get_type(Parser* p, SymbolId sym_id) {
    ASSERT(sym_id >= 0, "Token id out of range");
    ASSERT(sym_id < p->i_symtab, "Token id out of range");
    return p->symtab[sym_id].type;
}

/* In the future the parser has to generate the symbol table to
   provide context for parsing */

/* Adds provided token index and type into symbol table */
static SymbolId symtab_add(Parser* p, TokenId tok_id, Type type) {
    /* Check if already exists in current scope */
    SymbolId id = symtab_find(p, tok_id);
    if (id >= 0) {
        /* In future come up with method to handle already existing symbols */
        ASSERT(0, "Symbol already exists");
        return id;
    }

    /* Add new symbol */
    if (id >= MAX_SYMTAB_TOKEN) {
        parser_set_error(p, ec_pbufexceed);
        return -1;
    }
    id = p->i_symtab++;

    p->symtab[id].tok_id = tok_id;
    p->symtab[id].type = type;

    return id;
}

/* Creates a new temporary for the current scope in symbol table */
static SymbolId symtab_add_temporary(Parser* p, Type type) {
    int numlen = ichar(p->symtab_temp_num);
    /* chars for __local_, null terminator and number */
    char token[9 + numlen];
    strcopy("__local_", token);
    itostr(p->symtab_temp_num, token + 8);
    token[8 + numlen] = '\0';
    ++p->symtab_temp_num;

    TokenId tok_id = parser_add_token(p, token);
    return symtab_add(p, tok_id, type);
}

/* Retrieves the token associated with symbol id from symbol table*/
static const char* symbol_token(Parser* p, SymbolId id) {
    return p->token_buf + p->symtab[id].tok_id;
}

/* Retrieves the type associated with symbol id from symbol table*/
static Type symbol_type(Parser* p, SymbolId id) {
    return p->symtab[id].type;
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

/* If character is considered as its own token */
static int istokenchar(char c) {
    switch (c) {
        /* TODO incomplete list of token chars */
        case '(': case ')': case '{': case '&': case '*': case ',': case ';':
            return 1;
        default:
            return 0;
    }
}

/* C keyword handling */

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

/* Indicates the pointed to token is no longer in use */
static void consume_token(char* token) {
    token[0] = '\0';
    if (g_debug_print_parse_recursion) {
        LOG("^Consumed\n");
    }
}

/* Reads a null terminated token */
/* Returns pointer to the token, NULL if EOF or errored */
static char* read_token(Parser* p) {
    if (p->read_buf[0] != '\0') {
        if (g_debug_print_parse_recursion) {
            LOGF("%s ^Cached\n", p->read_buf);
        }
        return p->read_buf;
    }

    int i = 0; /* Index into buf */
    int seen_token = 0;
    char c;
    while ((c = getc(p->rf)) != EOF) {
        if (i >= MAX_TOKEN_LEN) {
            parser_set_error(p, ec_tokbufexceed);
            break; /* Since MAX_TOKEN_LEN excludes null terminator, we can break and add null terminator */
        }

        if (iswhitespace(c)) {
            if (seen_token) {
                break;
            }
            /* Skip leading whitespace */
            continue;
        }
        if (istokenchar(c)) {
            /* Token char is a separate token, save it to be read again */
            if (seen_token) {
                fseek(p->rf, -1, SEEK_CUR);
                break;
            }
            else {
                ASSERT(i == 0, "Expected token char to occupy index 0");
                p->read_buf[i] = c;
                ++i;
                break;
            }
        }
        seen_token = 1;
        p->read_buf[i] = c;
        ++i;
    }

    p->read_buf[i] = '\0';
    if (g_debug_print_parse_recursion) {
        LOGF("%s\n", p->read_buf);
    }

    if (c == EOF) {
        return NULL;
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
        parser_attach_node(p, parent, st_ ## symbol_type__);            \
    int matched__ = 0
/* Call at end on function */
#define PARSE_FUNC_END()                                               \
    if (g_debug_print_parse_recursion) {                               \
    debug_parse_func_recursion_depth--;                                \
    for (int i__ = 0; i__ < debug_parse_func_recursion_depth; ++i__) { \
        LOG("  ");                                                     \
    }                                                                  \
    if (matched__) {LOG("\033[32m");} else {LOG("\033[0;1;31m");}      \
    LOGF("<%d\033[0m\n", debug_parse_func_recursion_depth);            \
    }                                                                  \
    if (!matched__) {parser_set_parse_location(p, &start_location__);} \
    return matched__
/* Call if a match was made in production rule */
#define PARSE_MATCHED() matched__ = 1
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
/* 6.5 Expressions */
static int parse_primary_expression(Parser* p, ParseNode* parent);
static int parse_postfix_expression(Parser* p, ParseNode* parent);
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
static int parse_expression(Parser* p, ParseNode* parent);
/* 6.7 Declarators */
static int parse_declaration(Parser* p, ParseNode* parent);
static int parse_declaration_specifiers(Parser* p, ParseNode* parent);
static int parse_init_declarator_list(Parser* p, ParseNode* parent);
static int parse_init_declarator(Parser* p, ParseNode* parent);
static int parse_storage_class_specifier(Parser* p, ParseNode* parent);
static int parse_type_specifier(Parser* p, ParseNode* parent);
static int parse_type_qualifier(Parser* p, ParseNode* parent);
static int parse_function_specifier(Parser* p, ParseNode* parent);
static int parse_declarator(Parser* p, ParseNode* parent);
static int parse_direct_declarator(Parser* p, ParseNode* parent);
static int parse_direct_declarator_2(Parser* p, ParseNode* parent);
static int parse_pointer(Parser* p, ParseNode* parent);
static int parse_parameter_type_list(Parser* p, ParseNode* parent);
static int parse_parameter_list(Parser* p, ParseNode* parent);
static int parse_parameter_declaration(Parser* p, ParseNode* parent);
static int parse_initializer(Parser* p, ParseNode* parent);
/* 6.8 Statements and blocks */
static int parse_statement(Parser* p, ParseNode* parent);
static int parse_compound_statement(Parser* p, ParseNode* parent);
static int parse_block_item_list(Parser* p, ParseNode* parent);
static int parse_block_item(Parser* p, ParseNode* parent);
static int parse_jump_statement(Parser* p, ParseNode* parent);
/* 6.9 External definitions */
static int parse_function_definition(Parser* p, ParseNode* parent);
/* Helpers */
static int parse_expect(Parser* p, const char* match_token);

/* identifier */
static int parse_identifier(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(identifier);

    char* token;
    if ((token = read_token(p)) == NULL || parser_has_error(p)) goto exit;

    if (tok_isidentifier(token)) {
        parser_attach_token(p, PARSE_CURRENT_NODE, token);
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

    /* Incomplete */

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_decimal_constant(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(decimal_constant);

    char* token;
    if ((token = read_token(p)) == NULL || parser_has_error(p)) goto exit;

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

    parser_attach_token(p, PARSE_CURRENT_NODE, token);
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

    if (parse_primary_expression(p, PARSE_CURRENT_NODE)) goto matched;

    /* Incomplete */

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_unary_expression(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(unary_expression);

    if (parse_postfix_expression(p, PARSE_CURRENT_NODE)) goto matched;

    /* Incomplete */

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_cast_expression(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(cast_expression);

    if (parse_unary_expression(p, PARSE_CURRENT_NODE)) goto matched;

    /* Incomplete */

    goto exit;

matched:
    PARSE_MATCHED();

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
        parser_attach_token(p, PARSE_CURRENT_NODE, "*");
        if (!parse_multiplicative_expression(p, PARSE_CURRENT_NODE)) goto exit;
    }
    else if (parse_expect(p, "/")) {
        parser_attach_token(p, PARSE_CURRENT_NODE, "/");
        if (!parse_multiplicative_expression(p, PARSE_CURRENT_NODE)) goto exit;
    }
    else if (parse_expect(p, "%")) {
        parser_attach_token(p, PARSE_CURRENT_NODE, "%");
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
        parser_attach_token(p, PARSE_CURRENT_NODE, "+");
        if (!parse_additive_expression(p, PARSE_CURRENT_NODE)) goto exit;
    }
    if (parse_expect(p, "-")) {
        parser_attach_token(p, PARSE_CURRENT_NODE, "-");
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

    if (parse_shift_expression(p, PARSE_CURRENT_NODE)) goto matched;

    /* Incomplete */

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_equality_expression(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(equality_expression);

    if (parse_relational_expression(p, PARSE_CURRENT_NODE)) goto matched;

    /* Incomplete */

    goto exit;

matched:
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

    if (parse_inclusive_or_expression(p, PARSE_CURRENT_NODE)) goto matched;

    /* Incomplete */

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_logical_or_expression(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(logical_or_expression);

    if (parse_logical_and_expression(p, PARSE_CURRENT_NODE)) goto matched;

    /* Incomplete */

    goto exit;

matched:
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
    PARSE_FUNC_START(assignment_expression);

    if (parse_conditional_expression(p, PARSE_CURRENT_NODE)) goto matched;

    /* Incomplete */

    goto exit;

matched:
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

    char* token;
    if ((token = read_token(p)) == NULL || parser_has_error(p)) goto exit;
    if (tok_isstoreclass(token)) {
        parser_attach_token(p, PARSE_CURRENT_NODE, token);
        consume_token(token);
        PARSE_MATCHED();
    }

exit:
    PARSE_FUNC_END();
}

static int parse_type_specifier(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(type_specifier);

    char* token;
    if ((token = read_token(p)) == NULL || parser_has_error(p)) goto exit;
    if (tok_istypespec(token)) {
        parser_attach_token(p, PARSE_CURRENT_NODE, token);
        consume_token(token);
        PARSE_MATCHED();
    }

exit:
    PARSE_FUNC_END();
}

static int parse_type_qualifier(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(type_qualifier);

    char* token;
    if ((token = read_token(p)) == NULL || parser_has_error(p)) goto exit;
    if (tok_istypequal(token)) {
        parser_attach_token(p, PARSE_CURRENT_NODE, token);
        consume_token(token);
        PARSE_MATCHED();
    }

exit:
    PARSE_FUNC_END();
}

static int parse_function_specifier(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(function_specifier);

    char* token;
    if ((token = read_token(p)) == NULL || parser_has_error(p)) goto exit;
    if (tok_isfuncspec(token)) {
        parser_attach_token(p, PARSE_CURRENT_NODE, token);
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

    if (parse_expect(p, "(")) {
        if (parse_parameter_type_list(p, PARSE_CURRENT_NODE)) {
            if (parse_expect(p, ")")) goto matched;
        }
    }

    goto exit;

matched:
    PARSE_MATCHED();
    parse_direct_declarator_2(p, PARSE_CURRENT_NODE);

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

    if (parse_jump_statement(p, PARSE_CURRENT_NODE)) goto matched;

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_compound_statement(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(compound_statement);

    if (parse_expect(p, "{")) {
        parse_block_item_list(p, PARSE_CURRENT_NODE);
        if (parse_expect(p, "}")) {
            PARSE_MATCHED();
        }
    }

    PARSE_FUNC_END();
}

static int parse_block_item_list(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(block_item_list);

    /* Left recursion in C standard is converted to right recursion */
    /* block-item-list -> block-item block-item-list | block-item */
    if (!parse_block_item(p, PARSE_CURRENT_NODE)) goto exit;
    PARSE_MATCHED();

    parse_block_item_list(p, PARSE_CURRENT_NODE);

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

static int parse_jump_statement(Parser* p, ParseNode* parent) {
    PARSE_FUNC_START(jump_statement);

    if (parse_expect(p, "return")) {
        parser_attach_token(p, PARSE_CURRENT_NODE, "return");
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

    if (!parse_compound_statement(p, PARSE_CURRENT_NODE) ||
            parser_has_error(p)) goto exit;

    PARSE_MATCHED();
exit:
    PARSE_FUNC_END();
}

/* Return 1 if next token read matches provided token, 0 otherwise */
/* The token is consumed if it matches the provided token */
static int parse_expect(Parser* p, const char* match_token) {
    char* token;
    if ((token = read_token(p)) == NULL || parser_has_error(p)) {
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

/* 6.4 Lexical elements */
static SymbolId cg_identifier(Parser* p, ParseNode* node);
static SymbolId cg_constant(Parser* p, ParseNode* node);
static SymbolId cg_integer_constant(Parser* p, ParseNode* node);
static SymbolId cg_decimal_constant(Parser* p, ParseNode* node);
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
static void cg_block_item_list(Parser* p, ParseNode* node);
static void cg_block_item(Parser* p, ParseNode* node);
static void cg_jump_statement(Parser* p, ParseNode* node);
/* 6.9 External definitions */
static void cg_function_definition(Parser* p, ParseNode* node);
/* Helpers */
static TypeSpecifiers cg_extract_type_specifiers(Parser* p, ParseNode* node);
static int cg_extract_pointer(ParseNode* node);
static TokenId cg_extract_identifier(ParseNode* node);
static SymbolId cg_extract_symbol(Parser* p,
        ParseNode* declaration_specifiers_node, ParseNode* declarator_node);
static void cg_output_token(Parser* p, SymbolId id);
static void cg_output_type(Parser* p, SymbolId id);

static SymbolId cg_identifier(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(identifier);

    ParseNode* token_node = parse_node_child(node, 0);
    TokenId tok_id = parse_node_token_id(token_node);
    SymbolId sym_id = symtab_find(p, tok_id);
    ASSERT(sym_id >= 0, "Failed to find identifier");

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

    SymbolId sym_id = cg_decimal_constant(p, parse_node_child(node, 0));

    /* Incomplete */

    DEBUG_CG_FUNC_END();
    return sym_id;
}

static SymbolId cg_decimal_constant(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(decimal_constant);

    ParseNode* token_node = parse_node_child(node, 0);
    TokenId tok_id = parse_node_token_id(token_node);

    /* TODO Symbol table needs special handling for constants */
    Type type;
    type.typespec = ts_i32; /* TODO temporary hardcode */
    type.pointers = 0;
    SymbolId sym_id = symtab_add(p, tok_id, type);

    DEBUG_CG_FUNC_END();
    return sym_id;
}

static SymbolId cg_primary_expression(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(primary_expression);

    SymbolId sym_id = 0;
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

static SymbolId cg_postfix_expression(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(postfix_expression);

    SymbolId sym_id = cg_primary_expression(p, parse_node_child(node, 0));

    /* Incomplete */

    DEBUG_CG_FUNC_END();
    return sym_id;
}

static SymbolId cg_unary_expression(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(unary_expression);

    SymbolId sym_id = cg_postfix_expression(p, parse_node_child(node, 0));

    /* Incomplete */

    DEBUG_CG_FUNC_END();
    return sym_id;
}

static SymbolId cg_cast_expression(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(cast_expression);

    SymbolId sym_id = cg_unary_expression(p, parse_node_child(node, 0));

    /* Incomplete */

    DEBUG_CG_FUNC_END();
    return sym_id;
}

/* Traverses down multiplicative_expression and generates code in preorder */
static SymbolId cg_multiplicative_expression(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(multiplicative_expression);

    SymbolId operand_1 = cg_cast_expression(p, parse_node_child(node, 0));
    ParseNode* operator = parse_node_child(node, 1);
    node = parse_node_child(node, 2);
    while (node != NULL) {
        SymbolId operand_2 = cg_cast_expression(p, parse_node_child(node, 0));
        SymbolId operand_temp =
            symtab_add_temporary(p, symbol_type(p, operand_1));

        /* def for temporary */
        parser_output_il(p, "def ");
        cg_output_type(p, operand_temp);
        parser_output_il(p, " ");
        cg_output_token(p, operand_temp);
        parser_output_il(p, "\n");

        /* Operator can only be of 3 possible types */
        char* operator_token =
            parser_get_token(p, parse_node_token_id(operator));
        char* instruction = "mul";
        if (strequ(operator_token, "/")) {
            instruction = "div";
        }
        else if (strequ(operator_token, "%")) {
            instruction = "mod";
        }
        parser_output_ilf(p, "%s %s,%s,%s\n",
                instruction,
                symtab_get_token(p, operand_temp),
                symtab_get_token(p, operand_1),
                symtab_get_token(p, operand_2));

        operand_1 = operand_temp;
        operator = parse_node_child(node, 1);
        node = parse_node_child(node, 2);
    }
    ASSERT(operator == NULL,
            "Trailing operator without cast-expression");

    DEBUG_CG_FUNC_END();
    return operand_1;
}

/* Traverses down additive_expression and generates code in preorder */
static SymbolId cg_additive_expression(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(additive_expression);

    SymbolId operand_1 =
        cg_multiplicative_expression(p, parse_node_child(node, 0));
    ParseNode* operator = parse_node_child(node, 1);
    node = parse_node_child(node, 2);
    while (node != NULL) {
        SymbolId operand_2 =
            cg_multiplicative_expression(p, parse_node_child(node, 0));
        SymbolId operand_temp =
            symtab_add_temporary(p, symbol_type(p, operand_1));

        /* def for temporary */
        parser_output_il(p, "def ");
        cg_output_type(p, operand_temp);
        parser_output_il(p, " ");
        cg_output_token(p, operand_temp);
        parser_output_il(p, "\n");

        /* Operator can only be of 2 possible types */
        char* operator_token =
            parser_get_token(p, parse_node_token_id(operator));
        char* instruction = "add";
        if (strequ(operator_token, "-")) {
            instruction = "sub";
        }
        parser_output_ilf(p, "%s %s,%s,%s\n",
                instruction,
                symtab_get_token(p, operand_temp),
                symtab_get_token(p, operand_1),
                symtab_get_token(p, operand_2));

        operand_1 = operand_temp;
        operator = parse_node_child(node, 1);
        node = parse_node_child(node, 2);
    }
    ASSERT(operator == NULL,
            "Trailing operator without multiplicative-expression");

    DEBUG_CG_FUNC_END();
    return operand_1;
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

    SymbolId sym_id = cg_shift_expression(p, parse_node_child(node, 0));

    /* Incomplete */

    DEBUG_CG_FUNC_END();
    return sym_id;
}

static SymbolId cg_equality_expression(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(equality_expression);

    SymbolId sym_id = cg_relational_expression(p, parse_node_child(node, 0));

    /* Incomplete */

    DEBUG_CG_FUNC_END();
    return sym_id;
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

    SymbolId sym_id = cg_inclusive_or_expression(p, parse_node_child(node, 0));

    /* Incomplete */

    DEBUG_CG_FUNC_END();
    return sym_id;
}

static SymbolId cg_logical_or_expression(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(logical_or_expression);

    SymbolId sym_id = cg_logical_and_expression(p, parse_node_child(node, 0));

    /* Incomplete */

    DEBUG_CG_FUNC_END();
    return sym_id;
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

    SymbolId sym_id = cg_conditional_expression(p, parse_node_child(node, 0));

    /* Incomplete */

    DEBUG_CG_FUNC_END();
    return sym_id;
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

    /* L value */
    SymbolId lval_id = cg_extract_symbol(p, declspec, declarator);
    parser_output_il(p, "def ");
    cg_output_type(p, lval_id);
    parser_output_il(p, " ");
    cg_output_token(p, lval_id);
    parser_output_il(p, "\n");

    /* R value assigned to l value */
    ParseNode* initializer = parse_node_child(initdecl, 1);
    SymbolId rval_id = cg_initializer(p, initializer);
    parser_output_il(p, "mov ");
    cg_output_token(p, lval_id);
    parser_output_il(p, ",");
    cg_output_token(p, rval_id);
    parser_output_il(p, "\n");

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

    cg_output_type(p, id);
    parser_output_il(p, " ");
    cg_output_token(p, id);

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

    cg_jump_statement(p, parse_node_child(node, 0));

    /* Incomplete */

    DEBUG_CG_FUNC_END();
}

/* Traverses block item list and generates code */
static void cg_block_item_list(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(block_item_list);

    while (node != NULL) {
        cg_block_item(p, parse_node_child(node, 0));
        node = parse_node_child(node, 1);
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

static void cg_jump_statement(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(jump_statement);

    TokenId jmp_id = parse_node_token_id(parse_node_child(node, 0));
    const char* jmp_token = parser_get_token(p, jmp_id);

    if (strequ(jmp_token, "return")) {
        SymbolId exp_id = cg_expression(p, parse_node_child(node, 1));
        parser_output_il(p, "ret ");
        cg_output_token(p, exp_id);
        parser_output_il(p, "\n");
    }

    DEBUG_CG_FUNC_END();
}

/* Generates il for function definition */
static void cg_function_definition(Parser* p, ParseNode* node) {
    DEBUG_CG_FUNC_START(function_definition);
    ASSERT(parse_node_count_child(node) == 3, "Expected 3 children of node");

    ParseNode* declspec = parse_node_child(node, 0);
    ParseNode* declarator = parse_node_child(node, 1);

    /* name is function name, type is return type */
    SymbolId func_id = cg_extract_symbol(p, declspec, declarator);
    parser_output_il(p, "func ");
    cg_output_token(p, func_id);
    parser_output_il(p, ",");
    cg_output_type(p, func_id);
    parser_output_il(p, ",");

    ParseNode* dirdeclarator = parse_node_child(declarator, -1);
    ParseNode* dirdeclarator2 = parse_node_child(dirdeclarator, 1);
    ParseNode* paramtypelist = parse_node_child(dirdeclarator2, 0);
    ParseNode* paramlist = parse_node_child(paramtypelist, 0);
    cg_parameter_list(p, paramlist);
    parser_output_il(p, "\n");

    /* Function body */
    ParseNode* compound_statement = parse_node_child(node, 2);
    ParseNode* blockitemlist = parse_node_child(compound_statement, 0);
    cg_block_item_list(p, blockitemlist);

    DEBUG_CG_FUNC_END();
}

/* Extracts type specifiers
   Expects declaration-specifiers node */
static TypeSpecifiers cg_extract_type_specifiers(Parser* p, ParseNode* node) {
    ASSERT(parse_node_type(node) == st_declaration_specifiers, "Incorrect node type");
    ASSERT(parse_node_count_child(node) >= 1,
            "Expected at least 1 children of node");

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
        ts_i32, ts_i32, ts_i32, ts_i32,
        ts_u32, ts_u32,
        ts_i64, ts_i64, ts_i64, ts_i64,
        ts_u64, ts_u64,
        ts_f32,
        ts_f64,
        ts_f64,
    };

    /* 22 (max size of arrangements) + 1 for null terminator */
    int i_buf = 0;
    char buf[23];
    while (node != NULL) {
        ParseNode* subnode = parse_node_child(node, 0);
        if (parse_node_type(subnode) == st_type_specifier) {
            char* token = parser_get_token(p,
                    parse_node_token_id(parse_node_child(subnode, 0)));

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
            buf[i_buf] = '\0';

            /* Search for arrangement matching buffer */
            for (int j = 0; j < count; ++j) {
                if (strequ(arrangement[j], buf)) {
                    return typespec[j];
                }
            }
        }
        node = parse_node_child(node, 1);
    }

    return ts_none;
}

/* Counts number of pointers
   Expects declarator node */
static int cg_extract_pointer(ParseNode* node) {
    ASSERT(parse_node_type(node) == st_declarator, "Incorrect node type");
    ASSERT(parse_node_count_child(node) >= 1,
            "Expected at least 1 children of node");

    /* No pointer node */
    if (parse_node_count_child(node) == 1) {
        return 0;
    }

    ParseNode* pointer = parse_node_child(node, 0);
    int pointers = 1;
    while (pointer != NULL) {
        ++pointers;
        pointer = parse_node_child(pointer, 1);
    }
    return pointers;
}

/* Retrieves identifier string
   Expects declarator node */
static TokenId cg_extract_identifier(ParseNode* node) {
    ASSERT(node->type == st_declarator, "Incorrect node type");

    ParseNode* dirdecl = parse_node_child(node, -1);
    ParseNode* identifier = parse_node_child(dirdecl, 0);
    ParseNode* token_node = parse_node_child(identifier, 0);
    return parse_node_token_id(token_node);
}

/* Adds symbol (type + token) to symbol table
   Expects declaration-specifiers node and declarator node*/
static SymbolId cg_extract_symbol(Parser* p,
        ParseNode* declaration_specifiers_node, ParseNode* declarator_node) {
    TokenId tok_id = cg_extract_identifier(declarator_node);
    Type type;
    type.typespec = cg_extract_type_specifiers(p, declaration_specifiers_node);
    type.pointers = cg_extract_pointer(declarator_node);
    return symtab_add(p, tok_id, type);
}

/* Outputs token for provided symbol */
static void cg_output_token(Parser* p, SymbolId id) {
    parser_output_il(p, symtab_get_token(p, id));
}

/* Outputs type for provided symbol */
static void cg_output_type(Parser* p, SymbolId id) {
    Type type = symtab_get_type(p, id);
    parser_output_il(p, type_specifiers_str(type.typespec));
    for (int i = 0; i < type.pointers; ++i) {
        parser_output_il(p, "*");
    }
}

/* ============================================================ */
/* Initialization and configuration */

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
        else if (strequ(argv[i], "-Zd1")) {
            g_debug_print_parse_recursion = 1;
        }
        else if (strequ(argv[i], "-Zd2")) {
            g_debug_print_cg_recursion = 1;
        }
        else if (strequ(argv[i], "-Zd3")) {
            g_debug_print_parse_tree = 1;
        }
        else if (strequ(argv[i], "-Zd4")) {
            g_debug_print_buffers = 1;
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
    Parser p = {.rf = NULL, .of = NULL};
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

    /* Create the top level node in parse node buf */
    if (MAX_PARSE_TREE_NODE > 0) {
        p.i_parse_node_buf = 1;
        int parse_success = parse_function_definition(&p, p.parse_node_buf);

        if (parse_success) {
            if (p.i_parse_node_buf >= 2) {
                cg_function_definition(&p, p.parse_node_buf + 1);
            }
        }
        else {
            parser_set_error(&p, ec_syntaxerr);
            ERRMSG("Failed to build parse tree\n");
        }

        /* Debug options */
        if (g_debug_print_buffers) {
            debug_parser_buf_dump(&p);
        }
        if (g_debug_print_parse_tree) {
            debug_draw_parse_tree(&p);
        }
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

