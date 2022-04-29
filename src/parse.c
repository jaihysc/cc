/* Preprocessed c file parser */
/* Generated output is imm2 */

#include <stdio.h>
#include <stdarg.h>
#include "common.h"

#define MAX_TOKEN_LEN 255 /* Excluding null terminator, Tokens is string with no whitespace */
#define MAX_PARSER_BUFFER_LEN 255 /* Excluding null terminator */
#define MAX_PARSE_TREE_NODE 200 /* Maximum nodes in parser parse tree */
#define MAX_EXPR_OUTPUT_BUFFER_LEN 100 /* Max elements in expression parser output buffer */
#define MAX_EXPR_TOKEN_BUFFER_CHAR 512 /* Max characters in expression parser token buffer */
#define MAX_EXPR_NODE_BUFFER_LEN 40 /* Max nodes in expression parser tree node buffer */
#define MAX_EXPR_OPERATOR_BUFFER_LEN 20 /* Max operators in expression parser operator buffer */

/* ============================================================ */
/* Tree data structure + functions */

typedef enum {
    op_add = 0,
    op_subtract,
    op_multiply,
    op_divide,
    op_modulus
} operator;
/* Higher means more important, index by operator's numeric value */
int operator_precedence[] = {1,1,2,2,2};
/* Character representation of operator */
char operator_char[] = {'+', '-', '*', '/', '%'};

typedef enum {
    /* Sorted by Annex A */
    /* 6.4 Lexical elements */
    st_identifier,
    st_constant,
    st_integer_constant,
    st_decimal_constant,
    /* 6.5 Expressions */
    st_primary_expression,
    st_postfix_expression,
    st_argument_expression_list,
    st_unary_expression,
    st_cast_expression,
    st_multiplicative_expression,
    st_additive_expression,
    st_shift_expression,
    st_relational_expression,
    st_equality_expression,
    st_and_expression,
    st_exclusive_or_expression,
    st_inclusive_or_expression,
    st_logical_and_expression,
    st_logical_or_expression,
    st_conditional_expression,
    st_assignment_expression,
    st_expression,
    /* 6.7 Declarators */
    st_declaration,
    st_declaration_specifiers,
    st_init_declarator_list,
    st_init_declarator,
    st_storage_class_specifier,
    st_type_specifier,
    st_type_qualifier,
    st_function_specifier,
    st_declarator,
    st_direct_declarator,
    st_direct_declarator_2, /* Convert left recursion in C standard to right */
    st_pointer,
    st_parameter_type_list,
    st_parameter_list,
    st_parameter_declaration,
    st_initializer,
    /* 6.8 Statements and blocks */
    st_statement,
    st_compound_statement,
    st_block_item_list,
    st_block_item,
    st_jump_statement,
    /* 6.9 External definitions */
    st_function_definition
} symbol_type;

typedef struct {
    int a; /* temporary placeholder */
} parse_location;

typedef struct parse_node {
    struct parse_node* child[4];
    symbol_type type;
} parse_node;

typedef struct {
    void* left;
    void* right;
    int left_type;
    int right_type;
    operator op;
} expr_node;

/* Attaches node on left/right side of parent node */
static void expr_node_l_node(expr_node* parent_node, expr_node* node) {
    parent_node->left = node;
    parent_node->left_type = 0;
}
static void expr_node_r_node(expr_node* parent_node, expr_node* node) {
    parent_node->right = node;
    parent_node->right_type = 0;
}

/* Attaches token on left/right side of parent node */
static void expr_node_l_token(expr_node* parent_node, char* token) {
    parent_node->left = token;
    parent_node->left_type = 1;
}
static void expr_node_r_token(expr_node* parent_node, char* token) {
    parent_node->right = token;
    parent_node->right_type = 1;
}

static int expr_node_l_isnode(expr_node* node) {
    return node->left_type == 0;
}
static int expr_node_r_isnode(expr_node* node) {
    return node->right_type == 0;
}

static int expr_node_l_istoken(expr_node* node) {
    return node->left_type == 1;
}
static int expr_node_r_istoken(expr_node* node) {
    return node->right_type == 1;
}

/* ============================================================ */
/* Parser data structure + functions */

/* Should always be initialized to ec_noerr */
/* Since functions will only sets if error occurred */
typedef enum {
    ec_noerr = 0,
    ec_tokbufexceed,
    ec_pbufexceed, /* Parser buffer exceeded */
    ec_syntaxerr,
    ec_writefailed,
} errcode;

/* Indicates what parser currently reading,
   helps parser select the right buffer to output to */
typedef enum {
    ps_fdecl = 0, /* Function declaration */
    ps_paramtypelist,
    ps_expr
} pstate;

/* The text buffers of the parser */
/* Holds text until enough information is known to generate an il instruction */
typedef enum {
    /* Reuse buffers if they will never be both used at the same time*/
    pb_declspec_type = 0,
    pb_declarator_id = 1,
    pb_paramtypelist = 2,
    pb_op1 = 1, /* Expression first operand and result */
    pb_op2 = 2,
    pb_count /* Count of entries in enum */
} pbuffer;

typedef struct {
    FILE* rf; /* Input file */
    FILE* of; /* Output file */

    pstate state;
    /* Functions set error code only if an error occurred */
    /* By default this is set as ec_noerr */
    errcode ecode;

    /* Token read buffer, used by read_token to store token read */
    /* The read token is kept here, subsequent calls to read_token()
       will return this.
       When consume_token() is called, the next call to read_token()
       will fill this buffer with a new token
       */
    char tok_buf[MAX_TOKEN_LEN + 1];

    char buf[pb_count][MAX_PARSER_BUFFER_LEN + 1];
    int i_buf[pb_count]; /* Points to next available space */

    parse_node parse_node_buf[MAX_PARSE_TREE_NODE];
    int i_parse_node_buf; /* Points to next available space */

    int expr_output_buf[MAX_EXPR_OUTPUT_BUFFER_LEN];
    int i_expr_output_buf; /* Points to next available space */
    char expr_token_buf[MAX_EXPR_TOKEN_BUFFER_CHAR];
    int i_expr_token_buf; /* Points to next available space */
    expr_node expr_node_buf[MAX_EXPR_NODE_BUFFER_LEN];
    int i_expr_node_buf; /* Points to next available space */
    int expr_operator_buf[MAX_EXPR_OPERATOR_BUFFER_LEN];
    int i_expr_operator_buf; /* Points to next available space */
} parser;

static void parser_expr_gen_node(parser* p, const operator op);
static void debug_parser_buf_dump(parser* p);
static void debug_expr_node_walk(expr_node* node, int depth, int right_node);

/* Sets error code in parser */
static void parser_set_error(parser* p, errcode ecode) {
    p->ecode = ecode;
}

/* Returns error code in parser */
static errcode parser_get_error(parser* p) {
    return p->ecode;
}

/* Returns 1 if error is set, 0 otherwise */
static int parser_has_error(parser* p) {
    return p->ecode != ec_noerr;
}

/* Sets state in parser */
static void parser_set_state(parser* p, pstate state) {
    LOGF("Parser state change: %d -> %d\n", p->state, state);
    p->state = state;
}

/* Returns parser state */
static pstate parser_state(parser* p) {
    return p->state;
}

/* Reads contents of parser buffer specified by target */
/* Read is null terminated string */
static char* parser_buf_rd(parser* p, pbuffer target) {
    return p->buf[target];
}

/* Adds null terminated string into buffer specified by target */
/* The null terminator is not included */
/* Can be called again even if error occurred previously */
static void parser_buf_push(parser* p, pbuffer target, const char* str) {
    int i = 0;
    char c;
    while ((c = str[i]) != '\0') {
        if (p->i_buf[target] >= MAX_PARSER_BUFFER_LEN) {
            parser_set_error(p, ec_pbufexceed);
            break;
        }
        p->buf[target][p->i_buf[target]] = c;
        ++p->i_buf[target];
        ++i;
    }
    /* Always 1 extra slot to insert null terminator */
    p->buf[target][p->i_buf[target]] = '\0';
}

/* Clears all buffers of parser, leaving only null terminator */
static void parser_buf_clear(parser* p) {
    for (int i = 0; i < pb_count; ++i) {
        p->buf[i][0] = '\0';
        p->i_buf[i] = 0;
    }
}

/* Writes provided IL using format string and va_args into output */
static void parser_output_il(parser* p, const char* fmt, ...) {
    debug_parser_buf_dump(p);
    va_list args;
    va_start(args, fmt);
    if (vfprintf(p->of, fmt, args) < 0) {
        parser_set_error(p, ec_writefailed);
    }
    va_end(args);
}

/* Returns information about the current location in parse tree */
static parse_location get_parse_location(parser* p) {
    parse_location location;
    return location;
}

/* Sets the current location in the parse tree */
static void set_parse_location(parser* p, parse_location* location) {
}

/* Attaches a node of type st onto the provided parent node
   Returns the attached node */
static parse_node* parser_attach_node(parser* p, parse_node* parent_node, symbol_type st) {
    return NULL;
}

/* Attaches a token node onto the provided parent node
   Returns the attached node */
static parse_node* parser_attach_token(parser* p, parse_node* parent_node, const char* token) {
    return NULL;
}

/* Adds provided token to expression token buffer, with a
   reference in the expression output buffer */
static void parser_expr_add_token(parser* p, const char* token) {
    /* Add token to token buffer */
    /* + 1 as a null terminator has to be inserted */
    int token_start = p->i_expr_token_buf;
    if (p->i_expr_token_buf + strlength(token) + 1 > MAX_EXPR_TOKEN_BUFFER_CHAR) {
        parser_set_error(p, ec_pbufexceed);
        return;
    }
    int i = 0;
    while (token[i] != '\0') {
        p->expr_token_buf[p->i_expr_token_buf++] = token[i++];
    }
    p->expr_token_buf[p->i_expr_token_buf++] = '\0';

    /* Add reference in output buffer */
    if (p->i_expr_output_buf >= MAX_EXPR_OUTPUT_BUFFER_LEN) {
        parser_set_error(p, ec_pbufexceed);
        return;
    }
    p->expr_output_buf[p->i_expr_output_buf++] = token_start;
}

/* Handles provided operator, generating tree nodes or
   pushing the operator into a stack when appropriate */
static void parser_expr_add_operator(parser* p, const operator op) {
    /* Shunting yard algorithm */

    while (p->i_expr_operator_buf > 0) {
        /* Top of operator stack */
        operator last_op = p->expr_operator_buf[p->i_expr_operator_buf - 1];
        if (operator_precedence[last_op] >= operator_precedence[op]) {
            parser_expr_gen_node(p, last_op);
            --p->i_expr_operator_buf;
        }
        else {
            break;
        }
    }

    if (p->i_expr_operator_buf >= MAX_EXPR_OPERATOR_BUFFER_LEN) {
        parser_set_error(p, ec_pbufexceed);
        return;
    }
    p->expr_operator_buf[p->i_expr_operator_buf++] = op;
}

/* Generates tree nodes using all operators remaining in operator buffer
   Call when done parsing expression */
static void parser_expr_flush(parser* p) {
    while (p->i_expr_operator_buf > 0) {
        operator op = p->expr_operator_buf[p->i_expr_operator_buf - 1];
        parser_expr_gen_node(p, op);
        --p->i_expr_operator_buf;
    }
}

/* Clears buffers for expression parsing */
static void parser_expr_clear(parser* p) {
    p->i_expr_output_buf = 0;
    p->i_expr_token_buf = 0;
    p->i_expr_node_buf = 0;
    p->i_expr_operator_buf = 0;
}

/* Generates a tree node using the given operator and operands
   in expression output buffer */
static void parser_expr_gen_node(parser* p, const operator op) {
    if (p->i_expr_node_buf >= MAX_EXPR_NODE_BUFFER_LEN) {
        parser_set_error(p, ec_pbufexceed);
        return;
    }
    expr_node* node = p->expr_node_buf + p->i_expr_node_buf;
    ASSERT(p->i_expr_output_buf >= 2,
           "Insufficient arguments generating expression tree node");
    /* left and right nodes corresponds to the last 2 elements
       in the output buffer */
    int left_index = p->expr_output_buf[p->i_expr_output_buf - 2];
    if (left_index >= 0) {
        expr_node_l_token(node, p->expr_token_buf + left_index);
    }
    else {
        expr_node_l_node(node, p->expr_node_buf + (-left_index - 1));
    }
    int right_index = p->expr_output_buf[p->i_expr_output_buf - 1];
    if (right_index >= 0) {
        expr_node_r_token(node, p->expr_token_buf + right_index);
    }
    else {
        expr_node_r_node(node, p->expr_node_buf + (-right_index - 1));
    }

    node->op = op;

    /* Leave a reference to the node in output buffer */
    p->expr_output_buf[p->i_expr_output_buf - 2] = -(p->i_expr_node_buf + 1);
    /* Used 2 elements in output buffer, added 1 back */
    --p->i_expr_output_buf;
    /* Increment this last as the original value is needed */
    ++p->i_expr_node_buf;
}

/* Prints out contents of parser buffers */
static void debug_parser_buf_dump(parser* p) {
    LOG("Parser buffer: Count, contents\n");
    for (int i = 0; i < pb_count; ++i) {
        LOGF("%d | %s\n", i, p->buf[i]);
    }

    LOG("Token buffer:\n");
    LOGF("%s\n", p->tok_buf);

    LOG("Expression output buffer:\n");
    for (int i = 0; i < p->i_expr_output_buf; ++i) {
        LOGF("%d ", p->expr_output_buf[i]);
    }
    LOG("\n");

    LOG("Expression token buffer:\n");
    for (int i = 0; i < p->i_expr_token_buf; ++i) {
        char c = p->expr_token_buf[i];
        if (c == '\0') {
            LOG("\n")
        }
        else {
            LOGF("%c", c);
        }
    }

    LOG("Expresssion node buffer:\n");
    /* Draw tree with last node since it is the top node */
    if (p->i_expr_node_buf > 0) {
        debug_expr_node_walk(p->expr_node_buf + p->i_expr_node_buf - 1, 0, 0);
    }

    LOG("Expression operator buffer:\n");
    for (int i = 0; i < p->i_expr_operator_buf; ++i) {
        LOGF("%c ", operator_char[p->expr_operator_buf[i]]);
    }
    LOG("\n");
}

/* Depth is recursion depth, used to draw branches. Start at 0
   Right node if node being drawn is the right side of its parent, omits one
   set of branches to make formatting correct */
static void debug_expr_node_walk(expr_node* node, int depth, int right_node) {
    /* Character representation for the operator */
    LOGF("%c\n", operator_char[node->op]);

    if (!right_node) {
        for (int i = 0; i < depth; ++i) {
            LOG("| ");
        }
    }
    else {
        for (int i = 0; i < depth - 1; ++i) {
            LOG("| ");
        }
        LOG("  ");
    }
    LOG("├ ");
    if (expr_node_l_isnode(node)) {
        debug_expr_node_walk(node->left, depth + 1, 0);
    }
    else if (expr_node_l_istoken(node)) {
        LOGF("%s\n", (char*)node->left);
    }

    if (!right_node) {
        for (int i = 0; i < depth; ++i) {
            LOG("| ");
        }
    }
    else {
        for (int i = 0; i < depth - 1; ++i) {
            LOG("| ");
        }
        LOG("  ");
    }
    LOG("└ ");
    if (expr_node_r_isnode(node)) {
        debug_expr_node_walk(node->right, depth + 1, 1);
    }
    else if (expr_node_r_istoken(node)) {
        LOGF("%s\n", (char*)node->right);
    }
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
    LOG("^Consumed\n");
}

/* Reads a null terminated token */
/* Returns pointer to the token, NULL if EOF or errored */
static char* read_token(parser* p) {
    if (p->tok_buf[0] != '\0') {
        LOGF("%s ^Cached\n", p->tok_buf);
        return p->tok_buf;
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
                /* A token char itself is a token */
                ASSERT(i == 0, "Expected token char to occupy index 0");
                p->tok_buf[i] = c;
                ++i;
                break;
            }
        }
        seen_token = 1;
        p->tok_buf[i] = c;
        ++i;
    }

    p->tok_buf[i] = '\0';
    LOGF("%s\n", p->tok_buf);

    if (c == EOF) {
        return NULL;
    }
    return p->tok_buf;
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
    for (int i__ = 0; i__ < debug_parse_func_recursion_depth; ++i__) {  \
        LOG("  ");                                                      \
    }                                                                   \
    LOGF(">%d " #symbol_type__ "\n", debug_parse_func_recursion_depth); \
    debug_parse_func_recursion_depth++;                                 \
    parse_location start_location__ = get_parse_location(p);            \
    parse_node* node__ =                                                \
        parser_attach_node(p, parent_node, st_ ## symbol_type__);       \
    int matched__ = 0
/* Call at end on function */
#define PARSE_FUNC_END()                                               \
    debug_parse_func_recursion_depth--;                                \
    for (int i__ = 0; i__ < debug_parse_func_recursion_depth; ++i__) { \
        LOG("  ");                                                     \
    }                                                                  \
    if (matched__) {LOG("\033[32m");} else {LOG("\033[0;1;31m");}      \
    LOGF("<%d\033[0m\n", debug_parse_func_recursion_depth);            \
    if (!matched__) {set_parse_location(p, &start_location__);}        \
    return matched__
/* Call if a match was made in production rule */
#define PARSE_MATCHED() matched__ = 1
/* Name for the pointer to the current node */
#define PARSE_CURRENT_NODE node__

/* Sorted by Annex A.2 in C99 standard */
/* Return code indicates whether symbol was successfully parsed */
/* parse_node* is the parent node to attach additional nodes to */

/* 6.4 Lexical elements */
static int parse_identifier(parser* p, parse_node* parent_node);
static int parse_constant(parser* p, parse_node* parent_node);
static int parse_integer_constant(parser* p, parse_node* parent_node);
static int parse_decimal_constant(parser* p, parse_node* parent_node);
/* 6.5 Expressions */
static int parse_primary_expression(parser* p, parse_node* parent_node);
static int parse_postfix_expression(parser* p, parse_node* parent_node);
static int parse_unary_expression(parser* p, parse_node* parent_node);
static int parse_cast_expression(parser* p, parse_node* parent_node);
static int parse_multiplicative_expression(parser* p, parse_node* parent_node);
static int parse_additive_expression(parser* p, parse_node* parent_node);
static int parse_shift_expression(parser* p, parse_node* parent_node);
static int parse_relational_expression(parser* p, parse_node* parent_node);
static int parse_equality_expression(parser* p, parse_node* parent_node);
static int parse_and_expression(parser* p, parse_node* parent_node);
static int parse_exclusive_or_expression(parser* p, parse_node* parent_node);
static int parse_inclusive_or_expression(parser* p, parse_node* parent_node);
static int parse_logical_and_expression(parser* p, parse_node* parent_node);
static int parse_logical_or_expression(parser* p, parse_node* parent_node);
static int parse_conditional_expression(parser* p, parse_node* parent_node);
static int parse_assignment_expression(parser* p, parse_node* parent_node);
static int parse_expression(parser* p, parse_node* parent_node);
/* 6.7 Declarators */
static int parse_declaration(parser* p, parse_node* parent_node);
static int parse_declaration_specifiers(parser* p, parse_node* parent_node);
static int parse_init_declarator_list(parser* p, parse_node* parent_node);
static int parse_init_declarator(parser* p, parse_node* parent_node);
static int parse_storage_class_specifier(parser* p, parse_node* parent_node);
static int parse_type_specifier(parser* p, parse_node* parent_node);
static int parse_type_qualifier(parser* p, parse_node* parent_node);
static int parse_function_specifier(parser* p, parse_node* parent_node);
static int parse_declarator(parser* p, parse_node* parent_node);
static int parse_direct_declarator(parser* p, parse_node* parent_node);
static int parse_direct_declarator_2(parser* p, parse_node* parent_node);
static int parse_pointer(parser* p, parse_node* parent_node);
static int parse_parameter_type_list(parser* p, parse_node* parent_node);
static int parse_parameter_list(parser* p, parse_node* parent_node);
static int parse_parameter_declaration(parser* p, parse_node* parent_node);
static int parse_initializer(parser* p, parse_node* parent_node);
/* 6.8 Statements and blocks */
static int parse_statement(parser* p, parse_node* parent_node);
static int parse_compound_statement(parser* p, parse_node* parent_node);
static int parse_block_item_list(parser* p, parse_node* parent_node);
static int parse_block_item(parser* p, parse_node* parent_node);
static int parse_jump_statement(parser* p, parse_node* parent_node);
/* 6.9 External definitions */
static int parse_function_definition(parser* p, parse_node* parent_node);
/* Helpers */
static int parse_expect(parser* p, const char* match_token);

/* identifier */
static int parse_identifier(parser* p, parse_node* parent_node) {
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

static int parse_constant(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(constant);
    /*
//    int return_code = parse_integerconst(p);
    */

    PARSE_FUNC_END();
}

static int parse_integer_constant(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(integer_constant);

//    int return_code = parse_decimalconst(p);

    PARSE_FUNC_END();
}

static int parse_decimal_constant(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(decimal_constant);

//    char* token;
//    if ((token = read_token(p)) == NULL || parser_get_error(p) != ec_noerr) {
//        goto exit;
//    }
//
//    /* First character is nonzero-digit */
//    char c = token[0];
//    if (c <= '0' || c > '9') {
//        goto exit;
//    }
//
//    /* Remaining characters is digit */
//    int i = 0;
//    while (c != '\0') {
//        if (c < '0' || c > '9') {
//            goto exit;
//        }
//
//        ++i;
//        c = token[i];
//    }
//
//    if (parser_state(p) == ps_expr) {
//        parser_expr_add_token(p, token);
//    }
//    consume_token(token);
//    return_code = 1;

// exit:
    PARSE_FUNC_END();
}

static int parse_primary_expression(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(primary_expression);

    if (parse_identifier(p, PARSE_CURRENT_NODE)) goto matched;

//    int has_const = parse_const(p);
//    if (has_const || parser_has_error(p)) goto exit;
//
//    /* ( expression ) */
//    int has_bracket = parse_expecttoken(p, "(");
//    if (parser_has_error(p)) goto exit;
//    if (has_bracket) {
//        parse_expr(p);
//        if (parser_has_error(p)) goto exit;
//
//        /* Matching close bracket for expression */
//        int has_close_bracket = parse_expecttoken(p, ")");
//        if (parser_has_error(p)) goto exit;
//        if (!has_close_bracket) {
//            ERRMSG("Expected ) after expression\n");
//            parser_set_error(p, ec_syntaxerr);
//            goto exit;
//        }
//    }

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_postfix_expression(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(postfix_expression);

    if (parse_primary_expression(p, PARSE_CURRENT_NODE)) goto matched;

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_unary_expression(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(unary_expression);

    if (parse_postfix_expression(p, PARSE_CURRENT_NODE)) goto matched;

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_cast_expression(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(cast_expression);

    if (parse_unary_expression(p, PARSE_CURRENT_NODE)) goto matched;

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_multiplicative_expression(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(multiplicative_expression);

     if (parse_cast_expression(p, PARSE_CURRENT_NODE)) goto matched;

//     while (1) {
//
//         int has_multiply = parse_expecttoken(p, "*");
//         if (parser_has_error(p)) goto exit;
//         if (has_multiply) {
//             parser_expr_add_operator(p, op_multiply);
//             continue;
//         }
//
//         int has_divide = parse_expecttoken(p, "/");
//         if (parser_has_error(p)) goto exit;
//         if (has_divide) {
//             parser_expr_add_operator(p, op_divide);
//             continue;
//         }
//
//         int has_mod = parse_expecttoken(p, "%");
//         if (parser_has_error(p)) goto exit;
//         if (has_mod) {
//             parser_expr_add_operator(p, op_modulus);
//             continue;
//         }
//
//         break;
//     }
//
// exit:

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_additive_expression(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(additive_expression);

    if (parse_multiplicative_expression(p, PARSE_CURRENT_NODE)) goto matched;

//    while (1) {
//
//        int has_add = parse_expecttoken(p, "+");
//        if (parser_has_error(p)) goto exit;
//        if (has_add) {
//            parser_expr_add_operator(p, op_add);
//            continue;
//        }
//
//        int has_sub = parse_expecttoken(p, "-");
//        if (parser_has_error(p)) goto exit;
//        if (has_sub) {
//            parser_expr_add_operator(p, op_subtract);
//            continue;
//        }
//
//        break;
//    }
//
//exit:

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_shift_expression(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(shift_expression);

    if (parse_additive_expression(p, PARSE_CURRENT_NODE)) goto matched;

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_relational_expression(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(relational_expression);

    if (parse_shift_expression(p, PARSE_CURRENT_NODE)) goto matched;

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_equality_expression(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(equality_expression);

    if (parse_relational_expression(p, PARSE_CURRENT_NODE)) goto matched;

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_and_expression(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(and_expression);

    if (parse_equality_expression(p, PARSE_CURRENT_NODE)) goto matched;

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_exclusive_or_expression(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(exclusive_or_expression);

    if (parse_and_expression(p, PARSE_CURRENT_NODE)) goto matched;

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_inclusive_or_expression(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(inclusive_or_expression);

    if (parse_exclusive_or_expression(p, PARSE_CURRENT_NODE)) goto matched;

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_logical_and_expression(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(logical_and_expression);

    if (parse_inclusive_or_expression(p, PARSE_CURRENT_NODE)) goto matched;

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_logical_or_expression(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(logical_or_expression);

    if (parse_logical_and_expression(p, PARSE_CURRENT_NODE)) goto matched;

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_conditional_expression(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(conditional_expression);

    if (parse_logical_or_expression(p, PARSE_CURRENT_NODE)) goto matched;

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_assignment_expression(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(assignment_expression);

    if (parse_conditional_expression(p, PARSE_CURRENT_NODE)) goto matched;

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_expression(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(expression);

    if (parse_assignment_expression(p, PARSE_CURRENT_NODE)) goto matched;

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_declaration(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(declaration);

//    parse_declspec(p);
//    parse_initdeclaratorlist(p);
//
//    int has_semicolon = parse_expecttoken(p, ";");
//    if (parser_has_error(p)) goto exit;
//    if (!has_semicolon) {
//        ERRMSG("Expected semicolon after declaration\n");
//        parser_set_error(p, ec_syntaxerr);
//    }
//
//exit:
    PARSE_FUNC_END();
}

static int parse_declaration_specifiers(parser* p, parse_node* parent_node) {
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

static int parse_init_declarator_list(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(init_declarator_list);

//    while (1) {
//        parse_initdeclarator(p);
//
//        /* init-declarators separated by commas */
//        int has_token = parse_expecttoken(p, ",");
//        if (parser_has_error(p)) goto exit;
//        if (!has_token) {
//            break;
//        }
//    }
//
//exit:
    PARSE_FUNC_END();
}

static int parse_init_declarator(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(init_declarator);

//    /* declarator OR declarator = initializer */
//    parse_declarator(p);
//
//    int has_token = parse_expecttoken(p, "=");
//    if (parser_has_error(p)) goto exit;
//    if (has_token) {
//        parse_initializer(p);
//    }
//
//exit:
    PARSE_FUNC_END();
}

static int parse_storage_class_specifier(parser* p, parse_node* parent_node) {
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

static int parse_type_specifier(parser* p, parse_node* parent_node) {
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

static int parse_type_qualifier(parser* p, parse_node* parent_node) {
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

static int parse_function_specifier(parser* p, parse_node* parent_node) {
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

static int parse_declarator(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(declarator);

    parse_pointer(p, PARSE_CURRENT_NODE);
    if (parser_has_error(p)) goto exit;

    if (!parse_direct_declarator(p, PARSE_CURRENT_NODE)) goto exit;

    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_direct_declarator(parser* p, parse_node* parent_node) {
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

static int parse_direct_declarator_2(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(direct_declarator_2);

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

static int parse_pointer(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(pointer);

    if (!parse_expect(p, "*")) goto exit;
    parser_attach_token(p, PARSE_CURRENT_NODE, "*");
    PARSE_MATCHED();

    parse_pointer(p, PARSE_CURRENT_NODE);

exit:
    PARSE_FUNC_END();
}

static int parse_parameter_type_list(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(parameter_type_list);

    if (!parse_parameter_list(p, PARSE_CURRENT_NODE)) goto exit;
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_parameter_list(parser* p, parse_node* parent_node) {
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

static int parse_parameter_declaration(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(parameter_declaration);

    if (!parse_declaration_specifiers(p, PARSE_CURRENT_NODE)) goto exit;
    if (!parse_declarator(p, PARSE_CURRENT_NODE)) goto exit;

    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_initializer(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(initializer);

//    parser_set_state(p, ps_expr);
//    parse_assignmentexpr(p);
//    /* TODO This is temporary to test parsing */
//    parser_expr_flush(p);
//    debug_parser_buf_dump(p);
//    parser_expr_clear(p);
//    /* TODO remember and return to old state (stack maybe?) */
//    parser_set_state(p, ps_fdecl);
//    /* TODO be more careful about state changes to avoid a mess of
//       state changes in the future */

    PARSE_FUNC_END();
}

static int parse_statement(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(statement);

    if (parse_jump_statement(p, PARSE_CURRENT_NODE)) goto matched;

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_compound_statement(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(compound_statement);

    if (parse_expect(p, "{")) {
        parse_block_item_list(p, PARSE_CURRENT_NODE);
        if (parse_expect(p, "}")) {
            PARSE_MATCHED();
        }
    }

    PARSE_FUNC_END();
}

static int parse_block_item_list(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(block_item_list);

    /* Left recursion in C standard is converted to right recursion */
    /* block-item-list -> block-item block-item-list | block-item */
    if (!parse_block_item(p, PARSE_CURRENT_NODE)) goto exit;
    PARSE_MATCHED();

    parse_block_item_list(p, PARSE_CURRENT_NODE);

exit:
    PARSE_FUNC_END();
}

static int parse_block_item(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(block_item);

    if (parse_declaration(p, PARSE_CURRENT_NODE)) goto matched;
    if (parse_statement(p, PARSE_CURRENT_NODE)) goto matched;

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_jump_statement(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(jump_statement);

    if (parse_expect(p, "return")) {
        parse_expression(p, PARSE_CURRENT_NODE);
        if (parse_expect(p, ";")) goto matched;
    }

    goto exit;

matched:
    PARSE_MATCHED();

exit:
    PARSE_FUNC_END();
}

static int parse_function_definition(parser* p, parse_node* parent_node) {
    PARSE_FUNC_START(function_definition);

    if (!parse_declaration_specifiers(p, PARSE_CURRENT_NODE) ||
            parser_has_error(p)) goto exit;

    if (!parse_declarator(p, PARSE_CURRENT_NODE) ||
            parser_has_error(p)) goto exit;

    if (!parse_compound_statement(p, PARSE_CURRENT_NODE) ||
            parser_has_error(p)) goto exit;

    /* TODO generate code */
    PARSE_MATCHED();
exit:
    PARSE_FUNC_END();
}

/* Return 1 if next token read matches provided token, 0 otherwise */
/* The token is consumed if it matches the provided token */
static int parse_expect(parser * p, const char* match_token) {
    char* token;
    if ((token = read_token(p)) == NULL || parser_get_error(p) != ec_noerr) {
        return 0;
    }

    if (strequ(token, match_token)) {
        consume_token(token);
        return 1;
    }
    return 0;
}

/* ============================================================ */
/* Initialization and configuration */

/* Parses cli args and processes them */
/* NOTE: will not clean up file handles at exit */
/* Returns non zero if error */
static int handle_cli_arg(parser* p, int argc, char** argv) {
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
    parser p = {.rf = NULL, .of = NULL};
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

    parse_node node;
    parse_function_definition(&p, &node);
    if (p.ecode != ec_noerr) {
        LOGF("Error during parsing: %d\n", p.ecode);
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

