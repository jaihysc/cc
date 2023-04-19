#include "il2gen.h"

#include "common.h"

ErrorCode il2_construct(IL2Gen* il2, Cfg* cfg, Tree* tree) {
    il2->cfg = cfg;
    il2->tree = tree;
    return ec_noerr;
}

/* Codegen (cg) functions assume the parse tree is valid */

/* 6.4 Lexical elements */
/* 6.5 Expressions */
/* 6.7 Declarators */
/* 6.8 Statements and blocks */
static ErrorCode cg_compound_statement(IL2Gen* il2, TNode* node, Block* blk);
/* 6.9 External definitions */
static ErrorCode cg_function_definition(IL2Gen* il2, TNode* node);

static ErrorCode cg_compound_statement(IL2Gen* il2, TNode* node, Block* blk) {
    ErrorCode ecode;
    for (int i = 0; i < tnode_count_child(node); ++i) {
        TNode* child = tnode_child(node, i);
        switch (tnode_type(child)) {
            case tt_declaration:
                break;
            case tt_expression:
                break;
            case tt_jump_statement:
                break;
            default:
                ASSERT(0, "Unknown node type");
                break;
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
                cg_function_definition(il2, child);
                break;
            default:
                ASSERT(0, "Unknown node type");
                break;
        }
    }
    return ec_noerr;
}

ErrorCode il2_gen(IL2Gen* il2) {
    return traverse_tree(il2, tree_root(il2->tree));
}
