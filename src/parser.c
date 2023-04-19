#include "parser.h"

#include "common.h"
#include "globals.h"

/* Function used to determine if TNode should be removed
   Used in tnode_remove_if */
static int cmp_remove_tnode(TNode* node) {
    /* Do not remove unary expressions with an operator */
    if (tnode_type(node) == tt_unary_expression) {
        TNodeUnaryExpression* n = (TNodeUnaryExpression*)tnode_data(node);
        return n->type == TNodeUnaryExpression_none;
    }
    return tnode_count_child(node) == 1;
}

ErrorCode parser_construct(Parser* p, Lexer* lex, Symtab* symtab, Tree* tree) {
    p->lex = lex;
    p->symtab = symtab;
    p->tree = tree;
    return ec_noerr;
}

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
    ASSERT(parent != NULL, "Parent node is null");

/* Call at end on function */
#define PARSE_FUNC_END()                                                   \
    if (g_debug_print_parse_recursion) {                                   \
    debug_parse_func_recursion_depth--;                                    \
    for (int i__ = 0; i__ < debug_parse_func_recursion_depth; ++i__) {     \
        LOG("  ");                                                         \
    }                                                                      \
    LOGF("<%d\n", debug_parse_func_recursion_depth);                       \
    }

/* Deletes child nodes of current location */
#define PARSE_TRIM_TREE()                                           \
    if (g_debug_print_parse_recursion) {LOG("Parse tree clear\n");} \
    if (g_debug_print_parse_tree) {debug_print_parse_tree(p);}      \
    tree_detach_node_child(p, node__)

/* Sorted by Annex A.2 in C99 standard */
/* Return code indicates whether symbol was successfully parsed */
/* TNode* is the parent node to attach additional nodes to */

/* 6.4 Lexical elements */
static ErrorCode parse_identifier(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_constant(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_integer_constant(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_decimal_constant(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_octal_constant(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_hexadecimal_constant(Parser*, TNode* parent, int* matched);
/* 6.5 Expressions */
static ErrorCode parse_primary_expression(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_postfix_expression(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_postfix_expression_2(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_argument_expression_list(Parser*, TNode* parent, int* matched);
static ErrorCode parse_unary_expression(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_cast_expression(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_multiplicative_expression(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_additive_expression(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_shift_expression(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_relational_expression(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_equality_expression(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_and_expression(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_exclusive_or_expression(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_inclusive_or_expression(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_logical_and_expression(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_logical_or_expression(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_conditional_expression(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_assignment_expression(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_expression(Parser* p, TNode* parent, int* matched);
/* 6.7 Declarators */
static ErrorCode parse_declaration(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_declaration_specifiers(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_init_declarator_list(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_init_declarator(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_declarator(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_direct_declarator(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_parameter_type_list(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_parameter_list(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_parameter_declaration(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_type_name(Parser*, TNode* parent, int* matched);
static ErrorCode parse_abstract_declarator(Parser*, TNode* parent, int* matched);
static ErrorCode parse_initializer(Parser* p, TNode* parent, int* matched);
/* 6.8 Statements and blocks */
static ErrorCode parse_statement(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_compound_statement(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_block_item(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_expression_statement(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_selection_statement(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_iteration_statement(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_jump_statement(Parser* p, TNode* parent, int* matched);
/* 6.9 External definitions */
static ErrorCode parse_external_declaration(Parser* p, TNode* parent, int* matched);
static ErrorCode parse_function_definition(Parser* p, TNode* parent, int* matched);
/* Helpers */
static ErrorCode parse_expect(Parser* p, const char* match_token, int* matched);

/* identifier */
static ErrorCode parse_identifier(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(identifier);
    ErrorCode ecode = ec_noerr;
    *matched = 0;

    const char* token;
    if ((ecode = lexer_getc(p->lex, &token)) != ec_noerr) goto exit;

    if (tok_isidentifier(token)) {
        TNode* node;
        if ((ecode = tnode_alloca(&node, parent)) != ec_noerr) goto exit;

        TNodeIdentifier data;
        strcopy(token, data.token);
        tnode_set(node, tt_identifier, &data);

        lexer_consume(p->lex);
        *matched = 1;
        goto exit;
    }

exit:
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_constant(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(constant);
    ErrorCode ecode;
    *matched = 0;

    int has_match;
    if ((ecode = parse_integer_constant(p, parent, &has_match)) != ec_noerr) goto exit;
    if (has_match) goto matched;

    /* Incomplete */

    goto exit;

matched:
    *matched = 1;

exit:
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_integer_constant(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(integer_constant);
    ErrorCode ecode;
    *matched = 0;

    int has_match;
    if ((ecode = parse_decimal_constant(
                    p, parent, &has_match)) != ec_noerr) goto exit;
    if (has_match) goto matched;

    if ((ecode = parse_octal_constant(
                    p, parent, &has_match)) != ec_noerr) goto exit;
    if (has_match) goto matched;

    if ((ecode = parse_hexadecimal_constant(
                    p, parent, &has_match)) != ec_noerr) goto exit;
    if (has_match) goto matched;

    /* Incomplete */

    goto exit;

matched:
    *matched = 1;

exit:
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_decimal_constant(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(decimal_constant);
    ErrorCode ecode;
    *matched = 0;

    const char* token;
    if ((ecode = lexer_getc(p->lex, &token)) != ec_noerr) goto exit;

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

    TNodeConstant data;
    strcopy(token, data.token);

    TNode* node;
    if ((ecode = tnode_alloca(&node, parent)) != ec_noerr) goto exit;
    tnode_set(node, tt_constant, &data);

    lexer_consume(p->lex);
    *matched = 1;

exit:
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_octal_constant(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(octal_constant);
    ErrorCode ecode;
    *matched = 0;

    /* Left recursion in C standard is converted to right recursion */
    /* octal-constant
       -> 0 octal-constant-2(opt) */
    /* octal-constant-2
       -> octal-digit octal-constant-2(opt) */

    const char* token;
    if ((ecode = lexer_getc(p->lex, &token)) != ec_noerr) goto exit;

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

    TNodeConstant data;
    strcopy(token, data.token);

    TNode* node;
    if ((ecode = tnode_alloca(&node, parent)) != ec_noerr) goto exit;
    tnode_set(node, tt_constant, &data);

    lexer_consume(p->lex);
    *matched = 1;

exit:
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_hexadecimal_constant(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(hexadecimal_constant);
    ErrorCode ecode;
    *matched = 0;

    const char* token;
    if ((ecode = lexer_getc(p->lex, &token)) != ec_noerr) goto exit;

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

    /* FIXME
    tree_attach_token(p, parent, token);
    */
    lexer_consume(p->lex);
    *matched = 1;

exit:
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_primary_expression(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(primary_expression);
    ErrorCode ecode;
    *matched = 0;

    int has_match;
    if ((ecode = parse_identifier(p, parent, &has_match)) != ec_noerr) goto exit;
    if (has_match) goto matched;

    if ((ecode = parse_constant(p, parent, &has_match)) != ec_noerr) goto exit;
    if (has_match) goto matched;

    if ((ecode = parse_expect(p, "(", &has_match)) != ec_noerr) goto exit;
    if (has_match) {
        if ((ecode = parse_expression(p, parent, &has_match)) != ec_noerr) goto exit;
        if (!has_match) {
            ERRMSG("Expected expression\n");
            ecode = ec_syntaxerr;
            goto exit;
        }

        if ((ecode = parse_expect(p, ")", &has_match)) != ec_noerr) goto exit;
        if (!has_match) {
            ERRMSG("Expected ')'\n");
            ecode = ec_syntaxerr;
            goto exit;
        }
        goto matched;
    }

    goto exit;

matched:
    *matched = 1;

exit:
    PARSE_FUNC_END();
    return ecode;
}

//static ErrorCode parse_postfix_expression(Parser* p, TNode* parent, int* matched) {
//    PARSE_FUNC_START(postfix_expression);
//    ErrorCode ecode;
//    *matched = 0;
//
//    /* Left recursion in C standard is converted to right recursion */
//    /* postfix-expression
//       -> primary-expression postfix-expression-2(opt)
//        | ( type-name ) { initializer-list } postfix-expression-2(opt)
//        | ( type-name ) { initializer-list , } postfix-expression-2(opt) */
//    /* postfix-expression-2
//       -> [ expression ] postfix-expression-2(opt)
//        | ( argument-expression-list(opt) ) postfix-expression-2(opt)
//        | . identifier postfix-expression-2(opt)
//        | -> identifier postfix-expression-2(opt)
//        | ++ postfix-expression-2(opt)
//        | -- postfix-expression-2(opt) */
//
//    int has_match;
//    if ((ecode = parse_primary_expression(
//                    p, parent, &has_match)) != ec_noerr) goto exit;
//    if (has_match) {
//        *matched = 1;
//        /* FIXME
//        if ((ecode = parse_postfix_expression_2(
//                        p, parent, &has_match)) != ec_noerr) goto exit;
//                        */
//    }
//
//    /* Incomplete */
//
//    PARSE_FUNC_END();
//    return ecode;
//}
//
//static ErrorCode parse_postfix_expression_2(Parser* p, TNode* parent, int* matched) {
//    PARSE_FUNC_START(postfix_expression);
//    ErrorCode ecode;
//    *matched = 0;
//
//    /* Array subscript */
//    if (parse_expect(p, "[")) {
//        if (!parse_expression(p, parent)) goto exit;
//        if (!parse_expect(p, "]")) goto exit;
//        PARSE_MATCHED();
//
//        parse_postfix_expression_2(p, parent);
//    }
//    /* Function call */
//    else if (parse_expect(p, "(")) {
//        if (!parse_argument_expression_list(p, parent)) goto exit;
//        if (!parse_expect(p, ")")) goto exit;
//        PARSE_MATCHED();
//
//        parse_postfix_expression_2(p, parent);
//    }
//    /* Postfix increment, decrement */
//    else if (parse_expect(p, "++")) {
//        tree_attach_token(p, parent, "++");
//        PARSE_MATCHED();
//
//        parse_postfix_expression_2(p, parent);
//    }
//    else if (parse_expect(p, "--")) {
//        tree_attach_token(p, parent, "--");
//        PARSE_MATCHED();
//
//        parse_postfix_expression_2(p, parent);
//    }
//
//    /* Incomplete */
//
//exit:
//    PARSE_FUNC_END();
//    return ecode;
//}
//
//static ErrorCode parse_argument_expression_list(Parser* p, TNode* parent, int* matched) {
//    /* Left recursion in C standard is converted to right recursion
//       argument-expression-list
//       -> assignment-expression
//        | assignment-expression , argument-expression-list */
//    PARSE_FUNC_START(argument_expression_list);
//    ErrorCode ecode;
//    *matched = 0;
//
//    if (parse_assignment_expression(p, parent)) {
//        if (parse_expect(p, ",")) {
//            if (parse_argument_expression_list(p, parent)) {
//                PARSE_MATCHED();
//            }
//        }
//        else {
//            PARSE_MATCHED();
//        }
//    }
//
//    PARSE_FUNC_END();
//    return ecode;
//}

static ErrorCode parse_unary_expression(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(unary_expression);
    ErrorCode ecode;
    *matched = 0;

    /* Recursive as unary-operator needs a cast-expression, not a unary-expression */

    int attached_node = 0;
    TNode* node;
    if ((ecode = tnode_alloc(&node)) != ec_noerr) goto exit;

    TNodeUnaryExpression data;
    data.type = TNodeUnaryExpression_none;

    const char* token;
    if ((ecode = lexer_getc(p->lex, &token)) != ec_noerr) goto exit;

    int has_match;
    /* Prefix increment, decrement */
    if (strequ(token, "++")) {
        lexer_consume(p->lex);
        /* Incomplete */
    }
    else if (strequ(token, "--")) {
        lexer_consume(p->lex);
        /* Incomplete */
    }
    else if (strequ(token, "&")) {
        data.type = TNodeUnaryExpression_ref;
        lexer_consume(p->lex);
        if ((ecode = parse_cast_expression(p, node, &has_match)) != ec_noerr) goto exit;
        if (!has_match) {
            ERRMSG("Expected cast-expression\n");
            goto exit;
        }
        *matched = 1;
    }
    else if (strequ(token, "*")) {
        data.type = TNodeUnaryExpression_deref;
        lexer_consume(p->lex);
        if ((ecode = parse_cast_expression(p, node, &has_match)) != ec_noerr) goto exit;
        if (!has_match) {
            ERRMSG("Expected cast-expression\n");
            goto exit;
        }
        *matched = 1;
    }
    else if (strequ(token, "!")) {
        data.type = TNodeUnaryExpression_negate;
        lexer_consume(p->lex);
        if ((ecode = parse_cast_expression(p, node, &has_match)) != ec_noerr) goto exit;
        if (!has_match) {
            ERRMSG("Expected cast-expression\n");
            goto exit;
        }
        *matched = 1;
    }
    else {
        if ((ecode = parse_primary_expression(p, node, &has_match)) != ec_noerr) goto exit;
        if (has_match) *matched = 1;
    }

    /* Incomplete */

    if (*matched) {
        if ((ecode = tnode_attach(parent, node)) != ec_noerr) goto exit;
        tnode_set(node, tt_unary_expression, &data);
        attached_node = 1;
    }

exit:
    if (!attached_node) tnode_destruct(node);
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_cast_expression(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(cast_expression);
    ErrorCode ecode;
    *matched = 0;

    /* This comes first as the below consumes ( */
    if ((ecode = parse_unary_expression(p, parent, matched)) != ec_noerr) goto exit;

    /*
    if (parse_expect(p, "(")) {
        if (!parse_type_name(p, parent)) goto exit;
        if (!parse_expect(p, ")")) goto exit;
        if (!parse_cast_expression(p, parent)) goto exit;

        PARSE_MATCHED();
    }
    */

exit:
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_multiplicative_expression(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(multiplicative_expression);
    ErrorCode ecode;
    *matched = 0;

    /* Left recursion in C standard is converted to right recursion */
    /* NOTE: This does affect order of operations, operations involving
       multiplicative_expression nodes if preorder traversal
       e.g., a * b / c in the order of encountering them */
    /* multiplicative-expression
       -> cast-expression * multiplicative-expression
        | cast-expression / multiplicative-expression
        | cast-expression % multiplicative-expression
        | cast-expression */

    int attached_node = 0;
    TNode* node;

    while (1) {
        if ((ecode = tnode_alloc(&node)) != ec_noerr) goto exit;
        attached_node = 0;

        TNodeBinaryExpression data;
        data.type = TNodeBinaryExpression_none;

        int has_match;
        if ((ecode = parse_cast_expression(
                        p, node, &has_match)) != ec_noerr) goto exit;
        if (!has_match) break;

        const char* token;
        if ((ecode = lexer_getc(p->lex, &token)) != ec_noerr) goto exit;

        if (strequ(token, "*")) {
            data.type = TNodeBinaryExpression_mul;
            lexer_consume(p->lex);
        }
        else if (strequ(token, "/")) {
            data.type = TNodeBinaryExpression_div;
            lexer_consume(p->lex);
        }
        else if (strequ(token, "%")) {
            data.type = TNodeBinaryExpression_mod;
            lexer_consume(p->lex);
        }

        if ((ecode = tnode_attach(parent, node)) != ec_noerr) goto exit;
        tnode_set(node, tt_binary_expression, &data);
        parent = node;
        attached_node = 1;
        *matched = 1;
    }

exit:
    if (!attached_node) tnode_destruct(node);
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_additive_expression(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(additive_expression);
    ErrorCode ecode;
    *matched = 0;

    /* Left recursion in C standard is converted to right recursion */
    /* NOTE: This does affect order of operations, operations involving
       multiplicative_expression nodes is preorder traversal
       e.g., a + b - c in the order of encountering them */
    /* additive-expression -> multiplicative-expression + additive-expression
                            | multiplicative-expression - additive-expression
                            | multiplicative-expression */
    int attached_node = 0;
    TNode* node;

    while (1) {
        if ((ecode = tnode_alloc(&node)) != ec_noerr) goto exit;
        attached_node = 0;

        TNodeBinaryExpression data;
        data.type = TNodeBinaryExpression_none;

        int has_match;
        if ((ecode = parse_multiplicative_expression(
                        p, node, &has_match)) != ec_noerr) goto exit;
        if (!has_match) break;

        const char* token;
        if ((ecode = lexer_getc(p->lex, &token)) != ec_noerr) goto exit;

        if (strequ(token, "+")) {
            data.type = TNodeBinaryExpression_add;
            lexer_consume(p->lex);
        }
        else if (strequ(token, "-")) {
            data.type = TNodeBinaryExpression_sub;
            lexer_consume(p->lex);
        }

        if ((ecode = tnode_attach(parent, node)) != ec_noerr) goto exit;
        tnode_set(node, tt_binary_expression, &data);
        parent = node;
        attached_node = 1;
        *matched = 1;
    }

exit:
    if (!attached_node) tnode_destruct(node);
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_shift_expression(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(shift_expression);
    ErrorCode ecode;
    *matched = 0;

    if ((ecode = parse_additive_expression(
                    p, parent, matched)) != ec_noerr) goto exit;

    /* Incomplete */

exit:
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_relational_expression(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(relational_expression);
    ErrorCode ecode;
    *matched = 0;

    /* Left recursion in C standard is converted to right recursion */
    /* relational-expression
        -> shift-expression
         | shift-expression < relational-expression
         | shift-expression > relational-expression
         | shift-expression <= relational-expression
         | shift-expression >= relational-expression*/

    int attached_node = 0;
    TNode* node;

    while (1) {
        if ((ecode = tnode_alloc(&node)) != ec_noerr) goto exit;
        attached_node = 0;

        TNodeBinaryExpression data;
        data.type = TNodeBinaryExpression_none;

        int has_match;
        if ((ecode = parse_shift_expression(
                        p, node, &has_match)) != ec_noerr) goto exit;
        if (!has_match) break;

        const char* token;
        if ((ecode = lexer_getc(p->lex, &token)) != ec_noerr) goto exit;

        if (strequ(token, "<")) {
            data.type = TNodeBinaryExpression_le;
            lexer_consume(p->lex);
        }
        else if (strequ(token, ">")) {
            data.type = TNodeBinaryExpression_ge;
            lexer_consume(p->lex);
        }
        else if (strequ(token, "<=")) {
            data.type = TNodeBinaryExpression_leq;
            lexer_consume(p->lex);
        }
        else if (strequ(token, ">=")) {
            data.type = TNodeBinaryExpression_geq;
            lexer_consume(p->lex);
        }

        if ((ecode = tnode_attach(parent, node)) != ec_noerr) goto exit;
        tnode_set(node, tt_binary_expression, &data);
        parent = node;
        attached_node = 1;
        *matched = 1;
    }

exit:
    if (!attached_node) tnode_destruct(node);
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_equality_expression(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(equality_expression);
    ErrorCode ecode;
    *matched = 0;

    /* Left recursion in C standard is converted to right recursion */
    /* equality-expression
        -> relational-expression
         | relational-expression == equality-expression
         | relational-expression != equality-expression */

    int attached_node = 0;
    TNode* node;

    while (1) {
        if ((ecode = tnode_alloc(&node)) != ec_noerr) goto exit;
        attached_node = 0;

        TNodeBinaryExpression data;
        data.type = TNodeBinaryExpression_none;

        int has_match;
        if ((ecode = parse_relational_expression(
                        p, node, &has_match)) != ec_noerr) goto exit;
        if (!has_match) break;

        const char* token;
        if ((ecode = lexer_getc(p->lex, &token)) != ec_noerr) goto exit;

        if (strequ(token, "==")) {
            data.type = TNodeBinaryExpression_eq;
            lexer_consume(p->lex);
        }
        else if (strequ(token, "!=")) {
            data.type = TNodeBinaryExpression_neq;
            lexer_consume(p->lex);
        }

        if ((ecode = tnode_attach(parent, node)) != ec_noerr) goto exit;
        tnode_set(node, tt_binary_expression, &data);
        parent = node;
        attached_node = 1;
        *matched = 1;
    }

exit:
    if (!attached_node) tnode_destruct(node);
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_and_expression(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(and_expression);
    ErrorCode ecode;
    *matched = 0;

    if ((ecode = parse_equality_expression(p, parent, matched)) != ec_noerr) goto exit;

    /* Incomplete */

exit:
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_exclusive_or_expression(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(exclusive_or_expression);
    ErrorCode ecode;
    *matched = 0;

    if ((ecode = parse_and_expression(p, parent, matched)) != ec_noerr) goto exit;

    /* Incomplete */

exit:
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_inclusive_or_expression(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(inclusive_or_expression);
    ErrorCode ecode;
    *matched = 0;

    if ((ecode = parse_exclusive_or_expression(p, parent, matched)) != ec_noerr) goto exit;

    /* Incomplete */

exit:
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_logical_and_expression(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(logical_and_expression);
    ErrorCode ecode;
    *matched = 0;

    /* Left recursion in C standard is converted to right recursion */
    /* logical-and-expression
        -> inclusive-or-expression
         | inclusive-or-expression && logical-and-expression */

    int attached_node = 0;
    TNode* node;

    while (1) {
        if ((ecode = tnode_alloc(&node)) != ec_noerr) goto exit;
        attached_node = 0;

        TNodeBinaryExpression data;
        data.type = TNodeBinaryExpression_none;

        int has_match;
        if ((ecode = parse_inclusive_or_expression(
                        p, node, &has_match)) != ec_noerr) goto exit;
        if (!has_match) break;

        const char* token;
        if ((ecode = lexer_getc(p->lex, &token)) != ec_noerr) goto exit;

        if (strequ(token, "&&")) {
            data.type = TNodeBinaryExpression_logic_and;
            lexer_consume(p->lex);
        }

        if ((ecode = tnode_attach(parent, node)) != ec_noerr) goto exit;
        tnode_set(node, tt_binary_expression, &data);
        parent = node;
        attached_node = 1;
        *matched = 1;
    }

exit:
    if (!attached_node) tnode_destruct(node);
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_logical_or_expression(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(logical_or_expression);
    ErrorCode ecode;
    *matched = 0;

    /* Left recursion in C standard is converted to right recursion */
    /* logical-or-expression
        -> logical-and-expression
         | logical-and-expression || logical-or-expression */

    int attached_node = 0;
    TNode* node;

    while (1) {
        if ((ecode = tnode_alloc(&node)) != ec_noerr) goto exit;
        attached_node = 0;

        TNodeBinaryExpression data;
        data.type = TNodeBinaryExpression_none;

        int has_match;
        if ((ecode = parse_logical_and_expression(
                        p, node, &has_match)) != ec_noerr) goto exit;
        if (!has_match) break;

        const char* token;
        if ((ecode = lexer_getc(p->lex, &token)) != ec_noerr) goto exit;

        if (strequ(token, "||")) {
            data.type = TNodeBinaryExpression_logic_or;
            lexer_consume(p->lex);
        }

        if ((ecode = tnode_attach(parent, node)) != ec_noerr) goto exit;
        tnode_set(node, tt_binary_expression, &data);
        parent = node;
        attached_node = 1;
        *matched = 1;
    }

exit:
    if (!attached_node) tnode_destruct(node);
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_conditional_expression(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(conditional_expression);
    ErrorCode ecode;
    *matched = 0;

    if ((ecode = parse_logical_or_expression(p, parent, matched)) != ec_noerr) goto exit;

    /* Incomplete */

exit:
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_assignment_expression(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(assignment_expression);
    ErrorCode ecode;
    *matched = 0;

    /* TODO address concern below */
    /* For certain tokens, unary-expression can match but fail the
       remaining production, it must backtrack to undo the unary-expression
       to attempt another rule */
    /* Cannot check conditional-expression first, assignment-expression may
       match, but parent productions fail to match */

    int attached_node = 0;
    TNode* node;
    while (1) {
        if ((ecode = tnode_alloc(&node)) != ec_noerr) goto exit;
        attached_node = 0;

        TNodeAssignmentExpression data;
        data.type = TNodeAssignmentExpression_none;

        /* To resolve whether to treat as conditional-expression or unary-expression
           | conditional-expression
           | unary-expression assignment-operator assignment-expression
           always parse for a conditional-expression, then check assumption was valid */

        int has_match;
        if ((ecode = parse_conditional_expression(
                        p, node, &has_match)) != ec_noerr) goto exit;
        if (!has_match) break;

        const char* token;
        if ((ecode = lexer_getc(p->lex, &token)) != ec_noerr) goto exit;

        if (strequ(token, "=")) {
            data.type = TNodeAssignmentExpression_assign;
            lexer_consume(p->lex);
        }
        else if (strequ(token, "*=")) {
            data.type = TNodeAssignmentExpression_mul;
            lexer_consume(p->lex);
        }
        else if (strequ(token, "/=")) {
            data.type = TNodeAssignmentExpression_div;
            lexer_consume(p->lex);
        }
        else if (strequ(token, "%=")) {
            data.type = TNodeAssignmentExpression_mod;
            lexer_consume(p->lex);
        }
        else if (strequ(token, "+=")) {
            data.type = TNodeAssignmentExpression_add;
            lexer_consume(p->lex);
        }
        else if (strequ(token, "-=")) {
            data.type = TNodeAssignmentExpression_sub;
            lexer_consume(p->lex);
        }
        else if (strequ(token, "<<=")) {
            data.type = TNodeAssignmentExpression_shl;
            lexer_consume(p->lex);
        }
        else if (strequ(token, ">>=")) {
            data.type = TNodeAssignmentExpression_shr;
            lexer_consume(p->lex);
        }
        else if (strequ(token, "&=")) {
            data.type = TNodeAssignmentExpression_and;
            lexer_consume(p->lex);
        }
        else if (strequ(token, "^=")) {
            data.type = TNodeAssignmentExpression_xor;
            lexer_consume(p->lex);
        }
        else if (strequ(token, "|=")) {
            data.type = TNodeAssignmentExpression_or;
            lexer_consume(p->lex);
        }

        if ((ecode = tnode_attach(parent, node)) != ec_noerr) goto exit;
        tnode_set(node, tt_assignment_expression, &data);
        parent = node;
        *matched = 1;
        attached_node = 1;
    }

exit:
    if (!attached_node) tnode_destruct(node);
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_expression(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(expression);
    ErrorCode ecode;
    *matched = 0;

    int attached_node = 0;
    TNode* node;
    if ((ecode = tnode_alloc(&node)) != ec_noerr) goto exit;
    tnode_set(node, tt_expression, NULL);

    int has_match;
    if ((ecode = parse_assignment_expression(
                    p, node, &has_match)) != ec_noerr) goto exit;
    if (has_match) {
        if ((ecode = tnode_attach(parent, node)) != ec_noerr) goto exit;
        *matched = 1;
        attached_node = 1;
    }

    /* Incomplete */

    /* Cleanup the tree by removing unecessary nodes
       We cannot know ahead of time if there will be an operator applied,
       thus it sometimes creates a node, to realize it is not necessary */
    if ((ecode = tnode_remove_if(node, cmp_remove_tnode)) != ec_noerr) goto exit;

exit:
    if (!attached_node) tnode_destruct(node);
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_declaration(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(declaration);
    ErrorCode ecode;
    *matched = 0;

    int attached_node = 0;
    TNode* node;
    if ((ecode = tnode_alloc(&node)) != ec_noerr) goto exit;
    tnode_set(node, tt_declaration, NULL);

    int has_match;
    if ((ecode = parse_declaration_specifiers(
                    p, node, &has_match)) != ec_noerr) goto exit;
    if (!has_match) goto exit;

    if ((ecode = parse_init_declarator_list(
                    p, node, &has_match)) != ec_noerr) goto exit;

    if ((ecode = parse_expect(p, ";", &has_match)) != ec_noerr) goto exit;
    if (!has_match) {
        ERRMSG("Expected ';'\n");
        ecode = ec_syntaxerr;
        goto exit;
    }

    if ((ecode = tnode_attach(parent, node)) != ec_noerr) goto exit;
    attached_node = 1;
    *matched = 1;

exit:
    if (!attached_node) tnode_destruct(node);
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_declaration_specifiers(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(declaration_specifiers);
    ErrorCode ecode = ec_noerr;
    *matched = 0;

    int has_match; /* Whether has match on this iteration */
    TNodeDeclarationSpecifiers data;

    do {
        has_match = 0;

        const char* token;
        if ((ecode = lexer_getc(p->lex, &token)) != ec_noerr) goto exit;

        if (tok_isstoreclass(token)) {
            // FIXME
            lexer_consume(p->lex);
            has_match = 1;
            *matched = 1;
        }
        else if (tok_istypespec(token)) {
            // FIXME
            lexer_consume(p->lex);
            has_match = 1;
            *matched = 1;
        }
        else if (tok_istypequal(token)) {
            // FIXME
            lexer_consume(p->lex);
            has_match = 1;
            *matched = 1;
        }
        else if (tok_isfuncspec(token)) {
            // FIXME
            lexer_consume(p->lex);
            has_match = 1;
            *matched = 1;
        }
    } while (has_match);

    if (*matched) {
        TNode* node;
        if ((ecode = tnode_alloca(&node, parent)) != ec_noerr) goto exit;
        tnode_set(node, tt_declaration_specifiers, &data);
    }

exit:
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_init_declarator_list(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(init_declarator_list);
    ErrorCode ecode;
    *matched = 0;

    int has_match;
    if ((ecode = parse_init_declarator(p, parent, &has_match)) != ec_noerr) goto exit;
    if (!has_match) goto exit;

    /* FIXME
    if ((ecode = parse_expect(p, ",", &has_match)) != ec_noerr) goto exit;
    if (has_match) {
        if (!parse_init_declarator(p, PARSE_CURRENT_NODE)) goto exit;
    }
    */

    *matched = 1;

exit:
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_init_declarator(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(init_declarator);
    ErrorCode ecode;
    *matched = 0;

    int has_match;
    if ((ecode = parse_declarator(p, parent, &has_match)) != ec_noerr) goto exit;
    if (!has_match) goto exit;

    if ((ecode = parse_expect(p, "=", &has_match)) != ec_noerr) goto exit;
    if (has_match) {
        if ((ecode = parse_initializer(p, parent, &has_match)) != ec_noerr) goto exit;
        if (!has_match) {
            ERRMSG("Expected initializer\n");
            ecode = ec_syntaxerr;
            goto exit;
        }
    }

    *matched = 1;

exit:
    PARSE_FUNC_END();
    return ecode;
}

//static ErrorCode parse_specifier_qualifier_list(Parser* p, TNode* parent, int* matched) {
//    PARSE_FUNC_START(specifier_qualifier_list);
//    ErrorCode ecode = ec_noerr;
//    *matched = 0;
//
//    const char* token;
//    if ((ecode = lexer_getc(p->lex, &token)) != ec_noerr) goto exit;
//
//    if (tok_istypespec(token)) {
//        // TODO
//        *matched = 1;
//        /* parse_specifier_qualifier_list(p, PARSE_CURRENT_NODE, &has_match); */
//
//        /* Incomplete */
//    }
//    else if (tok_istypequal(token)) {
//        // TODO
//        *matched = 1;
//        /* parse_specifier_qualifier_list(p, PARSE_CURRENT_NODE, &has_match); */
//
//        /* Incomplete */
//    }
//
//exit:
//    PARSE_FUNC_END();
//    return ecode;
//}

static ErrorCode parse_declarator(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(declarator);
    ErrorCode ecode = ec_noerr;
    *matched = 0;

    int matched_ptr;
    int pointers = 0;
    while (1) {
        if ((ecode = parse_expect(p, "*", &matched_ptr)) != ec_noerr) goto exit;
        if (matched_ptr) ++pointers;
        else break;
    }

    TNode* node;
    if ((ecode = tnode_alloca(&node, parent)) != ec_noerr) goto exit;

    TNodePointer data;
    data.pointers = pointers;
    tnode_set(node, tt_pointer, &data);

    int matched_dirdecl;
    if ((ecode = parse_direct_declarator(p, parent, &matched_dirdecl)) != ec_noerr) goto exit;

    if (matched_ptr && !matched_dirdecl) {
        ERRMSG("Expected direct-declarator\n");
        ecode = ec_syntaxerr;
    }
    else if (matched_dirdecl) {
        *matched = 1;
    }

exit:
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_direct_declarator(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(direct_declarator);
    ErrorCode ecode = ec_noerr;
    *matched = 0;

    int has_match;
    if ((ecode = parse_identifier(p, parent, &has_match)) != ec_noerr) goto exit;
    if (!has_match) {
        ERRMSG("Expected identifier\n");
        ecode = ec_syntaxerr;
        goto exit;
    }

    *matched = 1;

    /* Incomplete */
    /*
    if ((ecode = parse_expect(p, "[", &has_match)) != ec_noerr) goto exit;
    if (has_match) {
        if (!parse_assignment_expression(p, PARSE_CURRENT_NODE)) goto exit;

        if ((ecode = parse_expect(p, "]", &has_match)) != ec_noerr) goto exit;
        if (!has_match) goto exit;
    }
    */

    if ((ecode = parse_expect(p, "(", &has_match)) != ec_noerr) goto exit;
    if (has_match) {
        if ((ecode = parse_parameter_type_list(
                        p, parent, &has_match)) != ec_noerr) goto exit;
        if (!has_match) {
            ERRMSG("Expected parameter-type-list\n");
            ecode = ec_syntaxerr;
            goto exit;
        }

        if ((ecode = parse_expect(p, ")", &has_match)) != ec_noerr) goto exit;
        if (!has_match) {
            ERRMSG("Expected ')'\n");
            ecode = ec_syntaxerr;
            goto exit;
        }
    }

exit:
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_parameter_type_list(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(parameter_type_list);
    ErrorCode ecode = ec_noerr;
    *matched = 0;

    int attached_node = 0;
    TNode* node;
    if ((ecode = tnode_alloc(&node)) != ec_noerr) goto exit;
    tnode_set(node, tt_parameter_type_list, NULL);

    int has_match;
    if ((ecode = parse_parameter_list(p, node, &has_match)) != ec_noerr) goto exit;
    if (has_match) {
        *matched = 1;
        if ((ecode = tnode_attach(parent, node)) != ec_noerr) goto exit;
        attached_node = 1;
    }

exit:
    if (!attached_node) tnode_destruct(node);
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_parameter_list(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(parameter_list);
    ErrorCode ecode = ec_noerr;
    *matched = 0;

    /* Left recursion in C standard is converted to right recursion */
    /* parameter-list -> parameter-declaration
                       | parameter-declaration , parameter-list */

    TNode* node;
    while (1) {
        if ((ecode = tnode_alloc(&node)) != ec_noerr) goto exit;

        int has_match;
        if ((ecode = parse_parameter_declaration(
                        p, node, &has_match)) != ec_noerr) goto exit;
        if (!has_match) break;

        /* If comma, must have another parameter-list */
        if ((ecode = parse_expect(p, ",", &has_match)) != ec_noerr) goto exit;

        if ((ecode = tnode_attach(parent, node)) != ec_noerr) goto exit;
        tnode_set(node, tt_parameter_list, NULL);
        *matched = 1;
    }

exit:
    /* Upon exiting, it always has an unattached node */
    tnode_destruct(node);
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_parameter_declaration(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(parameter_declaration);
    ErrorCode ecode = ec_noerr;
    *matched = 0;

    int has_match;
    if ((ecode = parse_declaration_specifiers(
                    p, parent, &has_match)) != ec_noerr) goto exit;
    if (!has_match) goto exit;

    if ((ecode = parse_declarator(
                    p, parent, &has_match)) != ec_noerr) goto exit;
    if (!has_match) goto exit;
    *matched = 1;

exit:
    PARSE_FUNC_END();
    return ecode;
}

//static ErrorCode parse_type_name(Parser* p, TNode* parent, int* matched) {
//    PARSE_FUNC_START(type_name);
//    ErrorCode ecode = ec_noerr;
//    *matched = 0;
//
//    int has_match;
//    if ((ecode = parse_specifier_qualifier_list(
//                    p, parent, &has_match)) != ec_noerr) goto exit;
//    if (!has_match) goto exit;
//
//    if ((ecode = parse_abstract_declarator(
//                    p, parent, &has_match)) != ec_noerr) goto exit;
//    *matched = 1;
//
//exit:
//    PARSE_FUNC_END();
//    return ecode;
//}

//static ErrorCode parse_abstract_declarator(Parser* p, TNode* parent, int* matched) {
//    PARSE_FUNC_START(abstract_declarator);
//    ErrorCode ecode = ec_noerr;
//    *matched = 0;
//
//    int matched_ptr;
//    int pointers = 0;
//    while (1) {
//        if ((ecode = parse_expect(p, "*", &matched_ptr)) != ec_noerr) goto exit;
//        if (matched_ptr) ++pointers;
//        else break;
//    }
//
//    /* Incomplete */
//
//    *matched = 1;
//
//exit:
//    PARSE_FUNC_END();
//    return ecode;
//}

static ErrorCode parse_initializer(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(initializer);
    ErrorCode ecode;
    *matched = 0;

    int has_match;
    if ((ecode = parse_assignment_expression(
                    p, parent, &has_match)) != ec_noerr) goto exit;
    if (has_match) goto matched;

    /* Incomplete */

    goto exit;

matched:
    *matched = 1;

    /* Cleanup the tree by removing unecessary nodes
       We cannot know ahead of time if there will be an operator applied,
       thus it sometimes creates a node, to realize it is not necessary */
    if ((ecode = tnode_remove_if(
                    tnode_child(parent, -1), cmp_remove_tnode)) != ec_noerr) goto exit;

exit:
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_statement(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(statement);
    ErrorCode ecode;
    *matched = 0;

    int has_match;
    /*
       TODO
    if (parse_compound_statement(p, PARSE_CURRENT_NODE)) goto matched;
    */
    if ((ecode = parse_expression_statement(
                    p, parent, &has_match)) != ec_noerr) goto exit;
    if (has_match) goto matched;
    /*
    if (parse_selection_statement(p, PARSE_CURRENT_NODE)) goto matched;
    if (parse_iteration_statement(p, PARSE_CURRENT_NODE)) goto matched;
    */
    if ((ecode = parse_jump_statement(p, parent, &has_match)) != ec_noerr) goto exit;
    if (has_match) goto matched;

    /* Incomplete */
    goto exit;

matched:
    *matched = 1;

exit:
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_compound_statement(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(compound_statement);
    ErrorCode ecode;
    *matched = 0;

    int has_match;
    if ((ecode = parse_expect(p, "{", &has_match)) != ec_noerr) goto exit;
    if (!has_match) goto exit;

    TNode* node;
    if ((ecode = tnode_alloca(&node, parent)) != ec_noerr) goto exit;
    tnode_set(node, tt_compound_statement, NULL);

    symtab_push_scope(p->symtab);
    /* block-item-list nodes are not needed for il generation */
    do {
        if ((ecode = parse_block_item(p, node, &has_match)) != ec_noerr) goto exit;
    } while (has_match);

    if ((ecode = parse_expect(p, "}", &has_match)) != ec_noerr) goto exit;
    if (!has_match) {
        ERRMSG("Expected '}' to end compound statement\n");
        ecode = ec_syntaxerr;
        goto exit;
    }
    *matched = 1;
    symtab_pop_scope(p->symtab);

exit:
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_block_item(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(block_item);
    ErrorCode ecode;
    *matched = 0;

    int has_match;
    if ((ecode = parse_declaration(p, parent, &has_match)) != ec_noerr) goto exit;
    if (has_match) {
        *matched = 1;
        goto exit;
    }

    if ((ecode = parse_statement(p, parent, &has_match)) != ec_noerr) goto exit;
    if (has_match) {
        *matched = 1;
        goto exit;
    }

exit:
    PARSE_FUNC_END();
    return ecode;
}

static ErrorCode parse_expression_statement(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(expression_statement);
    ErrorCode ecode;
    *matched = 0;

    int has_match;
    if ((ecode = parse_expression(p, parent, &has_match)) != ec_noerr) goto exit;
    if (!has_match) goto exit;

    if ((ecode = parse_expect(p, ";", &has_match)) != ec_noerr) goto exit;
    if (!has_match) {
        ERRMSG("Expected ';'\n");
        ecode = ec_syntaxerr;
        goto exit;
    }

    *matched = 1;

exit:
    PARSE_FUNC_END();
    return ecode;
}

//static ErrorCode parse_selection_statement(Parser* p, TNode* parent, int* matched) {
//    PARSE_FUNC_START(selection_statement);
//
//    /* Generate as follows: (if only, no else):
//       evaluate expression
//       jz false
//         code when true
//       false: */
//    /* Generate as follows: (if else):
//       evaluate expression
//       jz false
//         code when true
//         jmp end
//       false:
//         code when false
//       end: */
//
//    /* Validate that the production matches if "if" seen
//       as it is not possible to backtrack after il generation and
//       parse tree is cleared */
//
//    if (!parse_expect(p, "if")) goto exit;
//    if (!parse_expect(p, "(")) {
//        ERRMSG("Expected '('\n");
//        goto syntaxerr;
//    }
//    if (!parse_expression(p, PARSE_CURRENT_NODE)) {
//        ERRMSG("Expected expession\n");
//        goto syntaxerr;
//    }
//    if (!parse_expect(p, ")")) {
//        ERRMSG("Expected ')'\n");
//        goto syntaxerr;
//    }
//
//    SymbolId lab_false_id = cg_make_label(p);
//
//    SymbolId exp_id = cg_expression(p, parse_node_child(PARSE_CURRENT_NODE, 0));
//    PARSE_TRIM_TREE();
//    cgil_jz(p, lab_false_id, exp_id);
//
//    symtab_push_scope(p);
//    if (!parse_statement(p, PARSE_CURRENT_NODE)) {
//        ERRMSG("Expected statement\n");
//        goto syntaxerr;
//    }
//
//    cg_statement(p, parse_node_child(PARSE_CURRENT_NODE, 0));
//    PARSE_TRIM_TREE();
//    symtab_pop_scope(p);
//
//    if (parse_expect(p, "else")) {
//        SymbolId lab_end_id = cg_make_label(p);
//        cgil_jmp(p, lab_end_id);
//        cgil_lab(p, lab_false_id);
//
//        symtab_push_scope(p);
//        if (!parse_statement(p, PARSE_CURRENT_NODE)) {
//            ERRMSG("Expected statement\n");
//            goto syntaxerr;
//        }
//
//        cg_statement(p, parse_node_child(PARSE_CURRENT_NODE, 0));
//        PARSE_TRIM_TREE();
//        symtab_pop_scope(p);
//
//        cgil_lab(p, lab_end_id);
//    }
//    else {
//        cgil_lab(p, lab_false_id);
//    }
//
//    /* Incomplete */
//
//    PARSE_MATCHED();
//    goto exit;
//
//syntaxerr:
//    parser_set_error(p, ec_syntaxerr);
//
//exit:
//    PARSE_FUNC_END();
//}
//
//static ErrorCode parse_iteration_statement(Parser* p, TNode* parent, int* matched) {
//    PARSE_FUNC_START(iteration_statement);
//
//    if (parse_expect(p, "while")) {
//        /* Generate as follows:
//           eval expr1
//           jz end
//           loop:
//           statement
//           loop_body_end:
//           eval expr1
//           jnz loop
//           end: */
//        if (!parse_expect(p, "(")) {
//            ERRMSG("Expected '('\n");
//            goto syntaxerr;
//        }
//        if (!parse_expression(p, PARSE_CURRENT_NODE)) {
//            ERRMSG("Expected expression\n");
//            goto syntaxerr;
//        }
//        if (!parse_expect(p, ")")) {
//            ERRMSG("Expected ')'\n");
//            goto syntaxerr;
//        }
//
//        SymbolId lab_loop_id = cg_make_label(p);
//        SymbolId lab_body_end_id = cg_make_label(p);
//        SymbolId lab_end_id = cg_make_label(p);
//        symtab_push_cat(p, sc_lab_loopbodyend, lab_body_end_id);
//        symtab_push_cat(p, sc_lab_loopend, lab_end_id);
//
//        SymbolId exp_id =
//            cg_expression(p, parse_node_child(PARSE_CURRENT_NODE, 0));
//        cgil_jz(p, lab_end_id, exp_id);
//        cgil_lab(p, lab_loop_id);
//
//        symtab_push_scope(p);
//        if (!parse_statement(p, PARSE_CURRENT_NODE)) {
//            ERRMSG("Expected statement\n");
//            goto syntaxerr;
//        }
//        cg_statement(p, parse_node_child(PARSE_CURRENT_NODE, 1));
//        symtab_pop_scope(p);
//        symtab_pop_cat(p, sc_lab_loopbodyend);
//        symtab_pop_cat(p, sc_lab_loopend);
//
//        cgil_lab(p, lab_body_end_id);
//
//        exp_id = cg_expression(p, parse_node_child(PARSE_CURRENT_NODE, 0));
//        cgil_jnz(p, lab_loop_id, exp_id);
//        cgil_lab(p, lab_end_id);
//        PARSE_TRIM_TREE();
//
//        PARSE_MATCHED();
//    }
//    else if (parse_expect(p, "do")) {
//        /* Generate as follows:
//           loop:
//           statement
//           loop_body_end:
//           eval expr1
//           jnz loop
//           end: */
//        SymbolId lab_loop_id = cg_make_label(p);
//        SymbolId lab_body_end_id = cg_make_label(p);
//        SymbolId lab_end_id = cg_make_label(p);
//        symtab_push_cat(p, sc_lab_loopbodyend, lab_body_end_id);
//        symtab_push_cat(p, sc_lab_loopend, lab_end_id);
//
//        cgil_lab(p, lab_loop_id);
//
//        symtab_push_scope(p);
//        if (!parse_statement(p, PARSE_CURRENT_NODE)) {
//            ERRMSG("Expected statement\n");
//            goto syntaxerr;
//        }
//        cg_statement(p, parse_node_child(PARSE_CURRENT_NODE, 0));
//        PARSE_TRIM_TREE();
//
//        symtab_pop_scope(p);
//        symtab_pop_cat(p, sc_lab_loopbodyend);
//        symtab_pop_cat(p, sc_lab_loopend);
//
//        cgil_lab(p, lab_body_end_id);
//
//        if (!parse_expect(p, "while")) {
//            ERRMSG("Expected 'while'\n");
//            goto syntaxerr;
//        }
//        if (!parse_expect(p, "(")) {
//            ERRMSG("Expected '('\n");
//            goto syntaxerr;
//        }
//        if (!parse_expression(p, PARSE_CURRENT_NODE)) {
//            ERRMSG("Expected expression\n");
//            goto syntaxerr;
//        }
//        if (!parse_expect(p, ")")) {
//            ERRMSG("Expected ')'\n");
//            goto syntaxerr;
//        }
//        if (!parse_expect(p, ";")) {
//            ERRMSG("Expected ';'\n");
//            goto syntaxerr;
//        }
//        SymbolId exp_id =
//            cg_expression(p, parse_node_child(PARSE_CURRENT_NODE, 0));
//        cgil_jnz(p, lab_loop_id, exp_id);
//        PARSE_TRIM_TREE();
//
//        cgil_lab(p, lab_end_id);
//
//        PARSE_MATCHED();
//    }
//    else if (parse_expect(p, "for")) {
//        /* Generate as follows:
//           expr1 / declaration
//           eval expr2
//           jz end
//           loop:
//           statement
//           loop_body_end:
//           expr3
//           eval expr2
//           jnz loop
//           end: */
//
//        symtab_push_scope(p); /* Scope for declaration */
//        if (!parse_expect(p, "(")) {
//            ERRMSG("Expected '('\n");
//            goto syntaxerr;
//        }
//
//        /* expression-1 or declaration */
//        if (parse_expression(p, PARSE_CURRENT_NODE)) {
//            cg_expression(p, parse_node_child(PARSE_CURRENT_NODE, 0));
//            if (!parse_expect(p, ";")) {
//                ERRMSG("Expected ';'\n");
//                goto syntaxerr;
//            }
//            PARSE_TRIM_TREE();
//        }
//        else if (parse_declaration(p, PARSE_CURRENT_NODE)) {
//            cg_declaration(p, parse_node_child(PARSE_CURRENT_NODE, 0));
//            PARSE_TRIM_TREE();
//        }
//        else {
//            ERRMSG("Expected expression or declaration\n");
//            goto syntaxerr;
//        }
//
//        /* expression-2 (Controlling expression)
//           Stored as child 0 */
//        if (!parse_expression(p, PARSE_CURRENT_NODE)) {
//            ERRMSG("Expected expession\n");
//            goto syntaxerr;
//        }
//        if (!parse_expect(p, ";")) {
//            ERRMSG("Expected ';'\n");
//            goto syntaxerr;
//        }
//
//        /* expression-3
//           Stored as child 1 */
//        if (!parse_expression(p, PARSE_CURRENT_NODE)) {
//            ERRMSG("Expected expession\n");
//            goto syntaxerr;
//        }
//        if (!parse_expect(p, ")")) {
//            ERRMSG("Expected ')'\n");
//            goto syntaxerr;
//        }
//
//        SymbolId lab_loop_id = cg_make_label(p);
//        SymbolId lab_body_end_id = cg_make_label(p);
//        SymbolId lab_end_id = cg_make_label(p);
//        symtab_push_cat(p, sc_lab_loopbodyend, lab_body_end_id);
//        symtab_push_cat(p, sc_lab_loopend, lab_end_id);
//
//        SymbolId exp2_id =
//            cg_expression(p, parse_node_child(PARSE_CURRENT_NODE, 0));
//        cgil_jz(p, lab_end_id, exp2_id);
//        cgil_lab(p, lab_loop_id);
//
//        symtab_push_scope(p);
//        if (!parse_statement(p, PARSE_CURRENT_NODE)) {
//            ERRMSG("Expected statement\n");
//            goto syntaxerr;
//        }
//        cg_statement(p, parse_node_child(PARSE_CURRENT_NODE, 2));
//        symtab_pop_scope(p);
//        symtab_pop_cat(p, sc_lab_loopbodyend);
//        symtab_pop_cat(p, sc_lab_loopend);
//
//        cgil_lab(p, lab_body_end_id);
//
//        /* expression-3 (At loop end)
//           expression-2 (Controlling expression) */
//        cg_expression(p, parse_node_child(PARSE_CURRENT_NODE, 1));
//        exp2_id = cg_expression(p, parse_node_child(PARSE_CURRENT_NODE, 0));
//        PARSE_TRIM_TREE();
//        cgil_jnz(p, lab_loop_id, exp2_id);
//        cgil_lab(p, lab_end_id);
//
//        symtab_pop_scope(p); /* Scope for declaration */
//
//        PARSE_MATCHED();
//    }
//
//    /* Incomplete */
//
//    goto exit;
//
//syntaxerr:
//    parser_set_error(p, ec_syntaxerr);
//
//exit:
//    PARSE_FUNC_END();
//}

static ErrorCode parse_jump_statement(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(jump_statement);
    ErrorCode ecode;
    *matched = 0;

    TNodeJumpStatement data;
    int has_match;
    if ((ecode = parse_expect(p, "continue", &has_match)) != ec_noerr) goto exit;
    if (has_match) {
        data.type = TNodeJumpStatement_continue;
        goto matched;
    }

    if ((ecode = parse_expect(p, "break", &has_match)) != ec_noerr) goto exit;
    if (has_match) {
        data.type = TNodeJumpStatement_break;
        goto matched;
    }

    if ((ecode = parse_expect(p, "return", &has_match)) != ec_noerr) goto exit;
    if (has_match) {
        data.type = TNodeJumpStatement_return;
        goto matched;
    }

    goto exit;

matched:
    ;
    TNode* node;
    if ((ecode = tnode_alloca(&node, parent)) != ec_noerr) goto exit;
    tnode_set(node, tt_jump_statement, &data);

    if ((ecode = parse_expression(p, node, &has_match)) != ec_noerr) goto exit;;

    if ((ecode = parse_expect(p, ";", &has_match)) != ec_noerr) goto exit;
    if (!has_match) {
        ERRMSG("Expected ';'\n");
        ecode = ec_syntaxerr;
        goto exit;
    }
    *matched = 1;

exit:
    PARSE_FUNC_END();
    return ecode;
}

ErrorCode parse_translation_unit(Parser* p) {
    /* Repeatedly parse external-declaration until the end
       can be either a function-definition OR declaration
       We parse both until we can disambiguate */
    /* external-declaration
        -> function-definition
         | declaration */

    ErrorCode ecode;
    TNode* node = tree_root(p->tree);

    int matched;
    if ((ecode = parse_external_declaration(
                    p, node, &matched)) != ec_noerr) goto exit;

    debug_print_tree(p->tree);
exit:
    return ecode;
}

static ErrorCode parse_external_declaration(Parser* p, TNode* parent, int* matched) {
    PARSE_FUNC_START(external_declaration);
    ErrorCode ecode = ec_noerr;
    *matched = 0;

    int attached_node = 0;
    TNode* node;
    if ((ecode = tnode_alloc(&node)) != ec_noerr) goto exit;

    /* Must be a declaration-specifiers */
    int has_match;
    if ((ecode = parse_declaration_specifiers(p, node, &has_match)) != ec_noerr) goto exit;
    if (!has_match) {
        ERRMSG("Expected declaration-specifiers\n");
        ecode = ec_syntaxerr;
        goto exit;
    }

    /* declaration -> declaration-specifiers ; */
    /* Useless declaration */
    const char* token;
    if ((ecode = lexer_getc(p->lex, &token)) != ec_noerr) goto exit;
    if (strequ(token, ";")) {
        lexer_consume(p->lex);
        goto exit;
    }

    /* Must be a declarator */
    if ((ecode = parse_declarator(p, node, &has_match)) != ec_noerr) goto exit;
    if (!has_match) {
        ERRMSG("Expected declarator\n");
        ecode = ec_syntaxerr;
        goto exit;
    }

    /* Incomplete */

    /* Decide whether funciton or variable based last node */
    TNodeType type = tnode_type(tnode_child(node, -1));
    if (type == tt_parameter_type_list) {
        /* function-definition */

        if ((ecode = parse_compound_statement(
                        p, node, &has_match)) != ec_noerr) goto exit;
        if (!has_match) {
            ERRMSG("Expected compound-statement\n");
            ecode = ec_syntaxerr;
            goto exit;
        }

        if ((ecode = tnode_attach(parent, node)) != ec_noerr) goto exit;
        tnode_set(node, tt_function_definition, NULL);
        attached_node = 1;
        *matched = 1;
    }

exit:
    if (!attached_node) tnode_destruct(node);
    PARSE_FUNC_END();
    return ecode;
}

/* Return 1 if next token read matches provided token, 0 otherwise */
/* The token is consumed if it matches the provided token */
static ErrorCode parse_expect(Parser* p, const char* match_token, int* matched) {
    ErrorCode ecode;
    *matched = 0;

    const char* token;
    if ((ecode = lexer_getc(p->lex, &token)) != ec_noerr) return ecode;

    if (strequ(token, match_token)) {
        lexer_consume(p->lex);
        *matched = 1;
    }
    return ecode;
}

