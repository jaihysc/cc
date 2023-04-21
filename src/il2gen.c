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
static ErrorCode cg_assignment_expression(IL2Gen* il2, Symbol** sym, TNode* node, Block* blk);
static ErrorCode cg_expression(IL2Gen* il2, TNode* node, Block* blk);
/* 6.7 Declarators */
static ErrorCode cg_declaration(IL2Gen* il2, TNode* node, Block* blk);
/* 6.8 Statements and blocks */
static ErrorCode cg_compound_statement(IL2Gen* il2, TNode* node, Block* blk);
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
        case tt_assignment_expression:
            ecode = cg_assignment_expression(il2, sym, node, blk);
            break;
        default:
            ASSERT(0, "Unknown node type");
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
            /* FIXME
        case TNodeBinaryExpression_logic_and:
            ecode = block_add_ilstat(blk, il2stat_make(
                        il2_land, *sym, lresult, rresult));
            break;
        case TNodeBinaryExpression_logic_or:
            ecode = block_add_ilstat(blk, il2stat_make(
                        il2_lor, *sym, lresult, rresult));
            break;
            */
        default:
            ASSERT(0, "Unknown node type");
            break;
    }
    return ecode;
}

static ErrorCode cg_assignment_expression(IL2Gen* il2, Symbol** sym, TNode* node, Block* blk) {
    ErrorCode ecode;
    TNode* lchild = tnode_child(node, 0);
    TNode* rchild = tnode_child(node, 1);

    Symbol* lresult;
    if ((ecode = call_cg(il2, &lresult, lchild, blk)) != ec_noerr) return ecode;

    Symbol* rresult;
    if ((ecode = call_cg(il2, &rresult, rchild, blk)) != ec_noerr) return ecode;
    return ec_noerr;
}

static ErrorCode cg_expression(IL2Gen* il2, TNode* node, Block* blk) {
    ErrorCode ecode;
    for (int i = 0; i < tnode_count_child(node); ++i) {
        TNode* child = tnode_child(node, i);

        Symbol* sym;
        if ((ecode = call_cg(il2, &sym, child, blk)) != ec_noerr) return ecode;
    }
    return ec_noerr;
}

static ErrorCode cg_declaration(IL2Gen* il2, TNode* node, Block* blk) {
    ErrorCode ecode;
    TNode* initializer = tnode_child(node, 3);

    Symbol* sym;
    if ((ecode = call_cg(il2, &sym, initializer, blk)) != ec_noerr) return ecode;
    return ec_noerr;
}

static ErrorCode cg_compound_statement(IL2Gen* il2, TNode* node, Block* blk) {
    ErrorCode ecode;
    for (int i = 0; i < tnode_count_child(node); ++i) {
        TNode* child = tnode_child(node, i);
        switch (tnode_type(child)) {
            case tt_declaration:
                ecode = cg_declaration(il2, child, blk);
                break;
            case tt_expression:
                ecode = cg_expression(il2, child, blk);
                break;
            case tt_jump_statement:
                ecode = cg_jump_statement(il2, child, blk);
                break;
            default:
                ASSERT(0, "Unknown node type");
                break;
        }
        if (ecode != ec_noerr) return ecode;
    }
    return ecode;
}

static ErrorCode cg_jump_statement(IL2Gen* il2, TNode* node, Block* blk) {
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

    if ((ecode = cg_compound_statement(
                    il2, compound_stat, blk)) != ec_noerr) return ecode;
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
