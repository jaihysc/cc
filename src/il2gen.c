#include "il2gen.h"

#include "common.h"

ErrorCode il2_construct(IL2Gen* il2, Cfg* cfg, Symtab* stab, Tree* tree) {
    il2->cfg = cfg;
    il2->stab = stab;
    il2->tree = tree;
    return ec_noerr;
}

/* Codegen (cg) functions assume the parse tree is valid
   Those accepting Symbol** stores the Symbol* representing the result of the
   operator at the provided location */

/* 6.4 Lexical elements */
static ErrorCode cg_identifier(IL2Gen* il2, Symbol** sym, TNode* node, Block* blk);
static ErrorCode cg_constant(IL2Gen* il2, Symbol** sym, TNode* node, Block* blk);
/* 6.5 Expressions */
static ErrorCode cg_unary_expression(IL2Gen* il2, Symbol** sym, TNode* node, Block* blk);
static ErrorCode cg_binary_expression(IL2Gen* il2, Symbol** sym, TNode* node, Block* blk);
static ErrorCode cg_logical_and_expression(IL2Gen* il2, Symbol** sym, TNode* node, Block* blk);
static ErrorCode cg_logical_or_expression(IL2Gen* il2, Symbol** sym, TNode* node, Block* blk);
static ErrorCode cg_assignment_expression(IL2Gen* il2, Symbol** sym, TNode* node, Block* blk);
/* 6.7 Declarators */
static ErrorCode cg_declaration(IL2Gen* il2, TNode* node, Block* blk);
/* 6.8 Statements and blocks */
static ErrorCode cg_compound_statement(IL2Gen* il2, TNode* node, Block* blk);
static ErrorCode cg_selection_statement(IL2Gen* il2, TNode* node, Block* blk);
static ErrorCode cg_while_statement(IL2Gen* il2, TNode* node, Block* blk);
static ErrorCode cg_do_statement(IL2Gen* il2, TNode* node, Block* blk);
static ErrorCode cg_for_statement(IL2Gen* il2, TNode* node, Block* blk);
static ErrorCode cg_jump_statement(IL2Gen* il2, TNode* node, Block* blk);
/* 6.9 External definitions */
static ErrorCode cg_function_definition(IL2Gen* il2, TNode* node);

/* Calls the appropriate cg_ based on the type of the TNode type */
static ErrorCode call_cg(IL2Gen* il2, Symbol** sym, TNode* node, Block* blk) {
    ASSERT(il2 != NULL, "IL2Gen is null");
    ASSERT(sym != NULL, "Symbol is null");
    ASSERT(node != NULL, "TNode is null");
    ASSERT(blk != NULL, "Block is null");

    ErrorCode ecode;
    switch (tnode_type(node)) {
        case tt_identifier:
            ecode = cg_identifier(il2, sym, node, blk);
            break;
        case tt_constant:
            ecode = cg_constant(il2, sym, node, blk);
            break;
        case tt_unary_expression:
            ecode = cg_unary_expression(il2, sym, node, blk);
            break;
        case tt_binary_expression:
            ecode = cg_binary_expression(il2, sym, node, blk);
            break;
        case tt_logical_and_expression:
            ecode = cg_logical_and_expression(il2, sym, node, blk);
            break;
        case tt_logical_or_expression:
            ecode = cg_logical_or_expression(il2, sym, node, blk);
            break;
        case tt_assignment_expression:
            ecode = cg_assignment_expression(il2, sym, node, blk);
            break;
        default:
            ASSERT(0, "Unknown node type");
            break;
    }
    return ecode;
}

/* Calls the appropriate cg_ based on the type of the TNode type */
static ErrorCode call_cgs(IL2Gen* il2, TNode* node, Block* blk) {
    ASSERT(il2 != NULL, "IL2Gen is null");
    ASSERT(node != NULL, "TNode is null");
    ASSERT(blk != NULL, "Block is null");

    ErrorCode ecode;
    Symbol* sym; /* Not used, discards result from call_cg */
    switch (tnode_type(node)) {
        case tt_declaration:
            ecode = cg_declaration(il2, node, blk);
            break;
        case tt_compound_statement:
            ecode = cg_compound_statement(il2, node, blk);
            break;
        case tt_selection_statement:
            ecode = cg_selection_statement(il2, node, blk);
            break;
        case tt_while_statement:
            ecode = cg_while_statement(il2, node, blk);
            break;
        case tt_do_statement:
            ecode = cg_do_statement(il2, node, blk);
            break;
        case tt_for_statement:
            ecode = cg_for_statement(il2, node, blk);
            break;
        case tt_jump_statement:
            ecode = cg_jump_statement(il2, node, blk);
            break;
        default:
            /* Evaluate and discard symbol */
            ecode = call_cg(il2, &sym, node, blk);
            break;
    }
    return ecode;
}

static ErrorCode cg_identifier(IL2Gen* il2, Symbol** sym, TNode* node, Block* blk) {
    TNodeIdentifier* data = (TNodeIdentifier*)tnode_data(node);
    *sym = data->symbol;
    return ec_noerr;
}

static ErrorCode cg_constant(IL2Gen* il2, Symbol** sym, TNode* node, Block* blk) {
    TNodeConstant* data = (TNodeConstant*)tnode_data(node);
    *sym = data->symbol;
    return ec_noerr;
}

static ErrorCode cg_unary_expression(IL2Gen* il2, Symbol** sym, TNode* node, Block* blk) {
    ErrorCode ecode;
    TNodeUnaryExpression* data = (TNodeUnaryExpression*)tnode_data(node);
    TNode* child = tnode_child(node, 0);

    Symbol* child_result;
    if ((ecode = call_cg(il2, &child_result, child, blk)) != ec_noerr) return ecode;

    /* FIXME take common type */
    if ((ecode = symtab_add_temporary(
                    il2->stab, sym, type_int)) != ec_noerr) return ecode;

    switch (data->type) {
        case TNodeUnaryExpression_ref:
            ecode = block_add_ilstat(blk, il2stat_make2(
                        il2_mad, *sym, child_result));
            break;
        case TNodeUnaryExpression_deref:
            ecode = block_add_ilstat(blk, il2stat_make(
                        il2_mfi, *sym, child_result, symtab_constant_zero(il2->stab)));
            break;
        case TNodeUnaryExpression_negate:
            ecode = block_add_ilstat(blk, il2stat_make2(
                        il2_not, *sym, child_result));
            break;
        default:
            ASSERT(0, "Unknown node type");
            break;
    }
    return ec_noerr;
}

static ErrorCode cg_binary_expression(IL2Gen* il2, Symbol** sym, TNode* node, Block* blk) {
    ErrorCode ecode;
    TNodeBinaryExpression* data = (TNodeBinaryExpression*)tnode_data(node);
    TNode* lchild = tnode_child(node, 0);
    TNode* rchild = tnode_child(node, 1);

    Symbol* lresult;
    if ((ecode = call_cg(il2, &lresult, lchild, blk)) != ec_noerr) return ecode;

    Symbol* rresult;
    if ((ecode = call_cg(il2, &rresult, rchild, blk)) != ec_noerr) return ecode;

    /* FIXME take common type */
    if ((ecode = symtab_add_temporary(
                    il2->stab, sym, type_int)) != ec_noerr) return ecode;

    switch (data->type) {
        case TNodeBinaryExpression_add:
            ecode = block_add_ilstat(blk, il2stat_make(
                        il2_add, *sym, lresult, rresult));
            break;
        case TNodeBinaryExpression_sub:
            ecode = block_add_ilstat(blk, il2stat_make(
                        il2_sub, *sym, lresult, rresult));
            break;
        case TNodeBinaryExpression_mul:
            ecode = block_add_ilstat(blk, il2stat_make(
                        il2_mul, *sym, lresult, rresult));
            break;
        case TNodeBinaryExpression_div:
            ecode = block_add_ilstat(blk, il2stat_make(
                        il2_div, *sym, lresult, rresult));
            break;
        case TNodeBinaryExpression_mod:
            ecode = block_add_ilstat(blk, il2stat_make(
                        il2_mod, *sym, lresult, rresult));
            break;
        case TNodeBinaryExpression_l:
            ecode = block_add_ilstat(blk, il2stat_make(
                        il2_cl, *sym, lresult, rresult));
            break;
        case TNodeBinaryExpression_g:
            ecode = block_add_ilstat(blk, il2stat_make(
                        il2_cl, *sym, rresult, lresult));
            break;
        case TNodeBinaryExpression_le:
            ecode = block_add_ilstat(blk, il2stat_make(
                        il2_cle, *sym, lresult, rresult));
            break;
        case TNodeBinaryExpression_ge:
            ecode = block_add_ilstat(blk, il2stat_make(
                        il2_cle, *sym, rresult, lresult));
            break;
        case TNodeBinaryExpression_e:
            ecode = block_add_ilstat(blk, il2stat_make(
                        il2_ce, *sym, lresult, rresult));
            break;
        case TNodeBinaryExpression_ne:
            ecode = block_add_ilstat(blk, il2stat_make(
                        il2_cne, *sym, lresult, rresult));
            break;
        default:
            ASSERT(0, "Unknown node type");
            break;
    }
    return ecode;
}

static ErrorCode cg_logical_and_expression(IL2Gen* il2, Symbol** sym, TNode* node, Block* blk) {
    ErrorCode ecode;

    /* Short-circuit logical and */

    Symbol* label_false;
    Symbol* label_end;
    if ((ecode = symtab_add_label(il2->stab, &label_false)) != ec_noerr) return ecode;;
    if ((ecode = symtab_add_label(il2->stab, &label_end)) != ec_noerr) return ecode;;

    for (int i = 0; i < tnode_count_child(node); ++i) {
        TNode* child = tnode_child(node, i);

        Symbol* child_result;
        if ((ecode = call_cg(il2, &child_result, child, blk)) != ec_noerr) return ecode;

        if ((ecode = block_add_ilstat(
                        blk, il2stat_make2(
                            il2_jz, label_false, child_result))) != ec_noerr) return ecode;
    }

    if ((ecode = symtab_add_temporary(
                    il2->stab, sym, type_int)) != ec_noerr) return ecode;

    /* True */
    if ((ecode = block_add_ilstat(blk, il2stat_make2(
                        il2_mov, *sym, symtab_constant_one(il2->stab)))) != ec_noerr) return ecode;
    if ((ecode = block_add_ilstat(
                    blk, il2stat_make1(il2_jmp, label_end))) != ec_noerr) return ecode;

    /* False */
    if ((ecode = block_add_ilstat(
                    blk, il2stat_make1(il2_lab, label_false))) != ec_noerr) return ecode;
    if ((ecode = block_add_ilstat(blk, il2stat_make2(
                        il2_mov, *sym, symtab_constant_zero(il2->stab)))) != ec_noerr) return ecode;

    /* End */
    if ((ecode = block_add_ilstat(
                    blk, il2stat_make1(il2_lab, label_end))) != ec_noerr) return ecode;
    return ec_noerr;
}

static ErrorCode cg_logical_or_expression(IL2Gen* il2, Symbol** sym, TNode* node, Block* blk) {
    ErrorCode ecode;

    /* Short-circuit logical or */

    Symbol* label_true;
    Symbol* label_end;
    if ((ecode = symtab_add_label(il2->stab, &label_true)) != ec_noerr) return ecode;;
    if ((ecode = symtab_add_label(il2->stab, &label_end)) != ec_noerr) return ecode;;

    for (int i = 0; i < tnode_count_child(node); ++i) {
        TNode* child = tnode_child(node, i);

        Symbol* child_result;
        if ((ecode = call_cg(il2, &child_result, child, blk)) != ec_noerr) return ecode;

        if ((ecode = block_add_ilstat(
                        blk, il2stat_make2(
                            il2_jnz, label_true, child_result))) != ec_noerr) return ecode;
    }

    if ((ecode = symtab_add_temporary(
                    il2->stab, sym, type_int)) != ec_noerr) return ecode;

    /* False */
    if ((ecode = block_add_ilstat(blk, il2stat_make2(
                        il2_mov, *sym, symtab_constant_zero(il2->stab)))) != ec_noerr) return ecode;
    if ((ecode = block_add_ilstat(
                    blk, il2stat_make1(il2_jmp, label_end))) != ec_noerr) return ecode;

    /* True */
    if ((ecode = block_add_ilstat(
                    blk, il2stat_make1(il2_lab, label_true))) != ec_noerr) return ecode;
    if ((ecode = block_add_ilstat(blk, il2stat_make2(
                        il2_mov, *sym, symtab_constant_one(il2->stab)))) != ec_noerr) return ecode;

    /* End */
    if ((ecode = block_add_ilstat(
                    blk, il2stat_make1(il2_lab, label_end))) != ec_noerr) return ecode;
    return ec_noerr;
}

static ErrorCode cg_assignment_expression(IL2Gen* il2, Symbol** sym, TNode* node, Block* blk) {
    ErrorCode ecode;
    TNodeAssignmentExpression* data = (TNodeAssignmentExpression*)tnode_data(node);
    TNode* lchild = tnode_child(node, 0);
    TNode* rchild = tnode_child(node, 1);

    Symbol* lresult;
    if ((ecode = call_cg(il2, &lresult, lchild, blk)) != ec_noerr) return ecode;

    Symbol* rresult;
    if ((ecode = call_cg(il2, &rresult, rchild, blk)) != ec_noerr) return ecode;

    /* FIXME
    cg_com_type_rtol(p, opl_id, &opr_id); */

    if (data->type == TNodeAssignmentExpression_assign) {
        /* Assignment */
        if ((ecode = block_add_ilstat(
                        blk, il2stat_make2(
                            il2_mov, lresult, rresult))) != ec_noerr) return ecode;
    }
    else {
        /* Compound assignment */
        IL2Ins ins;
        switch (data->type) {
            case TNodeAssignmentExpression_mul:
                ins = il2_mul;
                break;
            case TNodeAssignmentExpression_div:
                ins = il2_div;
                break;
            case TNodeAssignmentExpression_mod:
                ins = il2_mod;
                break;
            case TNodeAssignmentExpression_add:
                ins = il2_add;
                break;
            case TNodeAssignmentExpression_sub:
                ins = il2_sub;
                break;
            default:
                ASSERT(0, "Unimplemented");
                break;
        }
        if ((ecode = block_add_ilstat(
                        blk, il2stat_make(
                            ins, lresult, lresult, rresult))) != ec_noerr) return ecode;
    }

    /* Left side holds results of expression */
    *sym = lresult;

    return ec_noerr;
}

static ErrorCode cg_declaration(IL2Gen* il2, TNode* node, Block* blk) {
    ErrorCode ecode;
    TNodeIdentifier* identifier = (TNodeIdentifier*)tnode_data(tnode_child(node, 2));
    TNode* initializer = tnode_child(node, 3);

    Symbol* result;
    if ((ecode = call_cg(il2, &result, initializer, blk)) != ec_noerr) return ecode;

    /* Save the result */
    if ((ecode = block_add_ilstat(
                    blk, il2stat_make2(
                        il2_mov, identifier->symbol, result))) != ec_noerr) return ecode;
    return ec_noerr;
}

static ErrorCode cg_compound_statement(IL2Gen* il2, TNode* node, Block* blk) {
    ErrorCode ecode = ec_noerr;
    for (int i = 0; i < tnode_count_child(node); ++i) {
        TNode* child = tnode_child(node, i);
        if ((ecode = call_cgs(il2, child, blk)) != ec_noerr) return ecode;
    }
    return ecode;
}

static ErrorCode cg_selection_statement(IL2Gen* il2, TNode* node, Block* blk) {
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
    ErrorCode ecode;

    TNode* expr = tnode_child(node, 0);
    TNode* statement_true = tnode_child(node, 1);
    TNode* statement_false = NULL;
    if (tnode_count_child(node) == 3) {
        statement_false = tnode_child(node, 2);
    }

    Symbol* label_false;
    Symbol* label_end;
    if ((ecode = symtab_add_label(il2->stab, &label_false)) != ec_noerr) return ecode;
    if ((ecode = symtab_add_label(il2->stab, &label_end)) != ec_noerr) return ecode;

    /* Evaluate expression */
    Symbol* expr_result;
    if ((ecode = call_cg(il2, &expr_result, expr, blk)) != ec_noerr) return ecode;

    if ((ecode = block_add_ilstat(
                    blk, il2stat_make2(
                        il2_jz, label_false, expr_result))) != ec_noerr) return ecode;

    /* Statement when true */
    if ((ecode = call_cgs(il2, statement_true, blk)) != ec_noerr) return ecode;

    /* Has else: jump past statement when false after statement when true */
    if (statement_false != NULL) {
        if ((ecode = block_add_ilstat(
                        blk, il2stat_make1(
                            il2_jmp, label_end))) != ec_noerr) return ecode;
    }

    /* Jump here when false */
    if ((ecode = block_add_ilstat(
                    blk, il2stat_make1(il2_lab, label_false))) != ec_noerr) return ecode;

    /* Statement when false */
    if (statement_false != NULL) {
        if ((ecode = call_cgs(il2, statement_false, blk)) != ec_noerr) return ecode;

        if ((ecode = block_add_ilstat(
                        blk, il2stat_make1(il2_lab, label_end))) != ec_noerr) return ecode;
    }

    return ec_noerr;
}

static ErrorCode cg_while_statement(IL2Gen* il2, TNode* node, Block* blk) {
    /* Generate as follows:
       eval expr1
       jz end
       loop:
       statement
       loop_body_end:
       eval expr1
       jnz loop
       end: */
    ErrorCode ecode;

    TNode* expr = tnode_child(node, 0);
    TNode* statement = tnode_child(node, 1);

    Symbol* label_loop;
    Symbol* label_end;
    if ((ecode = symtab_add_label(il2->stab, &label_loop)) != ec_noerr) return ecode;
    if ((ecode = symtab_add_label(il2->stab, &label_end)) != ec_noerr) return ecode;

    /* Evaluate expression */
    Symbol* expr_result;
    if ((ecode = call_cg(il2, &expr_result, expr, blk)) != ec_noerr) return ecode;

    /* Skip loop if false */
    if ((ecode = block_add_ilstat(
                    blk, il2stat_make2(
                        il2_jz, label_end, expr_result))) != ec_noerr) return ecode;


    /* Loop body */
    if ((ecode = block_add_ilstat(
                    blk, il2stat_make1(il2_lab, label_loop))) != ec_noerr) return ecode;

    if ((ecode = call_cgs(il2, statement, blk)) != ec_noerr) return ecode;

    /* Evaluate expression */
    if ((ecode = call_cg(il2, &expr_result, expr, blk)) != ec_noerr) return ecode;

    /* Repeat loop while true */
    if ((ecode = block_add_ilstat(
                    blk, il2stat_make2(
                        il2_jnz, label_loop, expr_result))) != ec_noerr) return ecode;


    /* End of loop */
    if ((ecode = block_add_ilstat(
                    blk, il2stat_make1(il2_lab, label_end))) != ec_noerr) return ecode;

    return ec_noerr;
}

static ErrorCode cg_do_statement(IL2Gen* il2, TNode* node, Block* blk) {
    /* Generate as follows:
       loop:
       statement
       loop_body_end:
       eval expr1
       jnz loop
       end: */
    ErrorCode ecode;

    TNode* statement = tnode_child(node, 0);
    TNode* expr = tnode_child(node, 1);

    Symbol* label_loop;
    Symbol* label_end;
    if ((ecode = symtab_add_label(il2->stab, &label_loop)) != ec_noerr) return ecode;
    if ((ecode = symtab_add_label(il2->stab, &label_end)) != ec_noerr) return ecode;

    /* Loop body */
    if ((ecode = block_add_ilstat(
                    blk, il2stat_make1(il2_lab, label_loop))) != ec_noerr) return ecode;

    if ((ecode = call_cgs(il2, statement, blk)) != ec_noerr) return ecode;

    /* Evaluate expression */
    Symbol* expr_result;
    if ((ecode = call_cg(il2, &expr_result, expr, blk)) != ec_noerr) return ecode;

    /* Repeat loop while true */
    if ((ecode = block_add_ilstat(
                    blk, il2stat_make2(
                        il2_jnz, label_loop, expr_result))) != ec_noerr) return ecode;

    return ec_noerr;
}

static ErrorCode cg_for_statement(IL2Gen* il2, TNode* node, Block* blk) {
    /* Generate as follows:
       expr1 / declaration
       eval expr2
       jz end
       loop:
       statement
       loop_body_end:
       eval expr3
       jnz loop
       end: */
    ErrorCode ecode;

    TNode* decl_expr = tnode_child(node, 0);
    TNode* expr2 = tnode_child(node, 1);
    TNode* expr3 = tnode_child(node, 2);
    TNode* statement = tnode_child(node, 3);

    if (tnode_type(decl_expr) == tt_dummy) decl_expr = NULL;
    if (tnode_type(expr2) == tt_dummy) expr2 = NULL;
    if (tnode_type(expr3) == tt_dummy) expr3 = NULL;

    Symbol* label_loop;
    Symbol* label_end;
    if ((ecode = symtab_add_label(il2->stab, &label_loop)) != ec_noerr) return ecode;
    if ((ecode = symtab_add_label(il2->stab, &label_end)) != ec_noerr) return ecode;

    Symbol* expr_result;

    /* Evaluate declaration / expression */
    if (decl_expr) {
        if ((ecode = call_cgs(il2, tnode_child(node, 0), blk)) != ec_noerr) return ecode;
    }


    /* Evaluate expression2 */
    if (expr2) {
        if ((ecode = call_cg(il2, &expr_result, expr2, blk)) != ec_noerr) return ecode;

        /* Skip loop if false */
        if ((ecode = block_add_ilstat(
                        blk, il2stat_make2(
                            il2_jz, label_end, expr_result))) != ec_noerr) return ecode;
    }


    /* Loop body */
    if ((ecode = block_add_ilstat(
                    blk, il2stat_make1(il2_lab, label_loop))) != ec_noerr) return ecode;

    if ((ecode = call_cgs(il2, statement, blk)) != ec_noerr) return ecode;

    /* Evaluate expression3 */
    if (expr3) {
        if ((ecode = call_cg(il2, &expr_result, expr3, blk)) != ec_noerr) return ecode;
    }

    if (expr2) {
        /* Repeat loop while true */
        if ((ecode = call_cg(il2, &expr_result, expr2, blk)) != ec_noerr) return ecode;

        if ((ecode = block_add_ilstat(
                        blk, il2stat_make2(
                            il2_jnz, label_loop, expr_result))) != ec_noerr) return ecode;
    }
    else {
        /* Always jump if no expression2 */
        if ((ecode = block_add_ilstat(
                        blk, il2stat_make1(il2_jmp, label_loop))) != ec_noerr) return ecode;
    }


    /* End of loop */
    if ((ecode = block_add_ilstat(
                    blk, il2stat_make1(il2_lab, label_end))) != ec_noerr) return ecode;
    return ec_noerr;
}

static ErrorCode cg_jump_statement(IL2Gen* il2, TNode* node, Block* blk) {
    ErrorCode ecode;
    TNodeJumpStatement* jump = (TNodeJumpStatement*)tnode_data(node);

    if (jump->type == TNodeJumpStatement_break) {
    }
    else if (jump->type == TNodeJumpStatement_continue) {
    }
    else if (jump->type == TNodeJumpStatement_return) {
        if (tnode_count_child(node) == 1) {
            /* Has something to return */
            TNode* child = tnode_child(node, 0);

            Symbol* result;
            call_cg(il2, &result, child, blk);

            if ((ecode = block_add_ilstat(
                            blk, il2stat_make1(
                                il2_ret, result))) != ec_noerr) return ecode;
        }
        else {
            /* Nothing to return */
            if ((ecode = block_add_ilstat(
                            blk, il2stat_make0(il2_ret))) != ec_noerr) return ecode;
        }
    }
    return ec_noerr;
}

static ErrorCode cg_function_definition(IL2Gen* il2, TNode* node) {
    ErrorCode ecode;

    /*
    TNode* declspec = tnode_child(node, 0);
    TNode* pointer = tnode_child(node, 1);
    TNode* identifier = tnode_child(node, 2);
    TNode* param_type_list = tnode_child(node, 3);
    */
    TNode* compound_stat = tnode_child(node, 4);

    Block* blk;
    if ((ecode = cfg_new_block(il2->cfg, &blk)) != ec_noerr) return ecode;
    if ((ecode = symtab_push_scope(il2->stab)) != ec_noerr) return ecode;

    if ((ecode = cg_compound_statement(
                    il2, compound_stat, blk)) != ec_noerr) return ecode;

    symtab_pop_scope(il2->stab);
    return ec_noerr;
}

static ErrorCode traverse_tree(IL2Gen* il2, TNode* node) {
    ErrorCode ecode;
    for (int i = 0; i < tnode_count_child(node); ++i) {
        TNode* child = tnode_child(node, i);
        switch (tnode_type(child)) {
            case tt_function_definition:
                ecode = cg_function_definition(il2, child);
                break;
            default:
                ASSERT(0, "Unknown node type");
                break;
        }
        if (ecode != ec_noerr) return ecode;
    }
    return ec_noerr;
}

ErrorCode il2_gen(IL2Gen* il2) {
    return traverse_tree(il2, tree_root(il2->tree));
}

ErrorCode il2_write(IL2Gen* il2, const char* filepath) {
    FILE* f = fopen("imm2", "w");
    if (f == NULL) {
        ERRMSG("Failed to open output file\n");
        return ec_writefailed;
    }

    /* Dirty hack to output
       In the future the Cfg gets directly fed to the assembly generator */

    /* Write the addresses of the pointers to give them unique names */

    /* Write symbol table
       skip the first 2 as that is argc, argv */
    if (hvec_size(&il2->stab->symbol) < 2) goto exit;
    Symbol* argc = &hvec_at(&il2->stab->symbol, 0);
    Symbol* argv = &hvec_at(&il2->stab->symbol, 1);

    for (int i = 2; i < hvec_size(&il2->stab->symbol); ++i) {
        Symbol* sym = &hvec_at(&il2->stab->symbol, i);
        Type type = symbol_type(sym);

        const char* type_str;
        switch (type_typespec(&type)) {
            case ts_void:
                type_str = "void";
                break;
            case ts_char:
            case ts_schar:
                type_str = "i8";
                break;
            case ts_uchar:
                type_str = "u8";
                break;
            case ts_short:
                type_str = "i16";
                break;
            case ts_ushort:
                type_str = "u16";
                break;
            case ts_int:
            case ts_long:
                type_str = "i32";
                break;
            case ts_uint:
            case ts_ulong:
                type_str = "u32";
                break;
            case ts_longlong:
                type_str = "i64";
                break;
            case ts_ulonglong:
                type_str = "u64";
                break;
            case ts_float:
                type_str = "f32";
                break;
            case ts_double:
            case ts_ldouble:
                type_str = "f64_";
                break;
            default:
                type_str = "???";
                break;
        }

        if (fprintf(f, "def %s _Z%p\n", type_str, (void*)sym) < 0) goto exit;
    }

    /* Write start of function */
    if (fprintf(f, "func main,i32,i32 _Z%p,i8** _Z%p\n",
                (void*)argc, (void*)argv) < 0) goto exit;

    /* Write IL2 */
    Block* blk = &vec_at(&il2->cfg->blocks, 0);
    for (int i = 0; i < block_ilstat_count(blk); ++i) {
        IL2Statement* stat = block_ilstat(blk, i);

        /* Instruction */
        if (fprintf(f, "%s ", il2_str(il2stat_ins(stat))) < 0) goto exit;

        for (int j = 0; j < il2stat_argc(stat); ++j) {
            /* Argument */
            if (j != 0) {
                if (fprintf(f, ",") < 0) goto exit;
            }

            Symbol* arg = il2stat_arg(stat, j);
            const char* token = symbol_token(arg);

            if ('0' <= token[0] && token[0] <= '9') {
                /* Is constant */
                if (fprintf(f, "%s", symbol_token(arg)) < 0) goto exit;
            }
            else {
                if (fprintf(f, "_Z%p", (void*)arg) < 0) goto exit;
            }
        }
        if (fprintf(f, "\n") < 0) goto exit;
    }

exit:
    fclose(f);
    return ec_noerr;
}
