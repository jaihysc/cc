/* Intermediate Language (IL) generator */
/* codegen functions assume the parse tree is valid */

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
                                 /* ^^^ The symbol repeatedly incremented */
    SymbolId result_id = sym_id; /* Value of result from previous operation */

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
        /* Type of previous result */
        Type type = symtab_get_type(p, result_id);

        if (parse_node_type(n) == st_expression) {
            SymbolId index = cg_expression(p, n);
            result_id = cg_array_subscript(p, result_id, index);
            /* ^ New                          ^ Old
               Dereference old result_id to get new result_id */
        }
        else if (parse_node_type(n) == st_argument_expression_list) {
            /* Function arguments */
            vec_t(SymbolId) arg_id;
            vec_construct(&arg_id);
            do {
                SymbolId id =
                    cg_assignment_expression(p, parse_node_child(n, 0));
                vec_push_back(&arg_id, cg_nlval(p, id));
                n = parse_node_child(n, 1);
            }
            while (n != NULL);

            /* Emit the IL for call AFTER cg for all the arguments,
               as the arguments may emit additional IL since they are
               expressions */

            /* Function call result */
            SymbolId temp_id = cg_make_temporary(p, type_return(&type));

            parser_output_il(p, "call $s,$s", temp_id, result_id);
            for (int i = 0; i < vec_size(&arg_id); ++i) {
                parser_output_il(p, ",$s", vec_at(&arg_id, i));
            }
            parser_output_il(p, "\n");

            vec_destruct(&arg_id);
            result_id = temp_id;
        }
        else {
            const char* token = parser_get_token(p, parse_node_token_id(n));
            /* Postfix increment, decrement */
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

