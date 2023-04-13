/* Compiler data structure + functions */

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
    sym->class = sl_normal;
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

/* Sets type for symbol */
static void symbol_set_type(Symbol* sym, const Type* type) {
    ASSERT(sym != NULL, "Symbol is null");
    sym->type = *type;
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

struct Parser {
    FILE* of; /* Output file */

    /* Functions set error code only if an error occurred */
    /* By default this is set as ec_noerr */
    ErrorCode ecode;

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

