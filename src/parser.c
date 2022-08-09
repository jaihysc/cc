/* Preprocessed c file parser */
/* Generated output is imm2 */

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
static int parse_argument_expression_list(Parser*, ParseNode*);
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

    /* Array subscript */
    if (parse_expect(p, "[")) {
        if (!parse_expression(p, PARSE_CURRENT_NODE)) goto exit;
        if (!parse_expect(p, "]")) goto exit;
        PARSE_MATCHED();

        parse_postfix_expression_2(p, PARSE_CURRENT_NODE);
    }
    /* Function call */
    else if (parse_expect(p, "(")) {
        if (!parse_argument_expression_list(p, PARSE_CURRENT_NODE)) goto exit;
        if (!parse_expect(p, ")")) goto exit;
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

static int parse_argument_expression_list(Parser* p, ParseNode* parent) {
    /* Left recursion in C standard is converted to right recursion
       argument-expression-list
       -> assignment-expression
        | assignment-expression , argument-expression-list */
    PARSE_FUNC_START(argument_expression_list);

    if (parse_assignment_expression(p, PARSE_CURRENT_NODE)) {
        if (parse_expect(p, ",")) {
            if (parse_argument_expression_list(p, PARSE_CURRENT_NODE)) {
                PARSE_MATCHED();
            }
        }
        else {
            PARSE_MATCHED();
        }
    }

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


    ParseNode* declspec = parse_node_child(PARSE_CURRENT_NODE, 0);
    ParseNode* declarator = parse_node_child(PARSE_CURRENT_NODE, 1);

    /* name is function name, type is return type */
    SymbolId func_id = cg_extract_symbol(p, declspec, declarator);
    Symbol* sym = symtab_get(p, func_id);

    Type func_type;
    Type ret_type = symbol_type(sym);
    type_constructf(&func_type, &ret_type);
    symbol_set_type(sym, &func_type);
    parser_output_il(p, "func $s,$t,", func_id, func_id);

    ParseNode* dirdeclarator = parse_node_child(declarator, -1);
    ParseNode* dirdeclarator2 = parse_node_child(dirdeclarator, 1);

    const char* tok = parse_node_token(p, parse_node_child(dirdeclarator2, 0));
    if (!strequ( "(", tok)) {
        ERRMSG("Expected (' to begin parameter-type-list");
        goto exit;
    }

    /* FIXME The compound statement creates another scope, meaning that the
       function parameters are inside its own scope, which is non standard
       compliant. Doing so accepts all well formed programs, however fails to
       reject some mal-formed programs so think of another method in the future
       to handle scoping */
    /* symtab_pop_scope is in cg_function_declaration */
    symtab_push_scope(p);

    ParseNode* paramtypelist = parse_node_child(dirdeclarator2, 1);
    ParseNode* paramlist = parse_node_child(paramtypelist, 0);
    cg_parameter_list(p, paramlist);
    parser_output_il(p, "\n");
    PARSE_TRIM_TREE();


    if (!parse_compound_statement(p, PARSE_CURRENT_NODE) ||
            parser_has_error(p)) goto exit2;


    PARSE_MATCHED();
exit2:
    symtab_pop_scope(p);
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

