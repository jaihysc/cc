#include "tree.h"

#include "common.h"

ErrorCode tnode_alloc(TNode** node_ptr) {
    ASSERT(node_ptr != NULL, "Node pointer is null");

    *node_ptr = ccalloc(1, sizeof(TNode));
    if (*node_ptr == NULL) return ec_badalloc;
    return ec_noerr;
}

void tnode_destruct(TNode* node) {
    if (node == NULL) return;

    for (int i = 0; i < MAX_TNODE_CHILD; ++i) {
        if (node->child[i] != NULL) {
            tnode_destruct(node->child[i]);
        }
    }
    cfree(node);
}

ErrorCode tnode_attach(TNode* node, TNode* new_node) {
    ASSERT(node != NULL, "Node is null");
    ASSERT(new_node != NULL, "New node is null");

    /* First first available spot */
    int i = 0;
    while (1) {
        if (i >= MAX_TNODE_CHILD) {
            return ec_tnode_childexceed;
        }
        if (node->child[i] == NULL) {
            node->child[i] = new_node;
            break;
        }
        ++i;
    }
    return ec_noerr;
}

ErrorCode tnode_alloca(TNode** node_ptr, TNode* parent) {
    ASSERT(node_ptr != NULL, "Node is null");
    ASSERT(parent != NULL, "Parent is null");
    ErrorCode ecode;

    ecode = tnode_alloc(node_ptr);
    if (ecode == ec_noerr) {
        ecode = tnode_attach(parent, *node_ptr);
        if (ecode == ec_noerr) return ec_noerr;

        tnode_destruct(*node_ptr);
        return ecode;
    }
    return ecode;
}

int tnode_count_child(TNode* node) {
    ASSERT(node != NULL, "Node is null");

    int count = 0;
    while (count < MAX_TNODE_CHILD) {
        if (node->child[count] == NULL) {
            break;
        }
        ++count;
    }
    return count;
}

TNode* tnode_child(TNode* node, int i) {
    int childs = tnode_count_child(node);

    if (i >= 0) {
        ASSERT(i < MAX_TNODE_CHILD, "Child index out of range");
        return node->child[i];
    }

    /* Index backwads */
    ASSERT(childs + i >= 0, "Child reverse index out of range");
    return node->child[childs + i];
}

SymbolType tnode_type(TNode* node) {
    ASSERT(node != NULL, "Node is null");
    return node->type;
}

TNodeData* tnode_data(TNode* node) {
    ASSERT(node != NULL, "Node is null");
    return &node->data;
}

int tnode_variant(TNode* node) {
    ASSERT(node != NULL, "Node is null");
    return node->variant;
}

void tnode_set(TNode* node, SymbolType st, void* data, int var) {
    ASSERT(node != NULL, "Node is null");
    node->type = st;
    switch (st) {
        /* 6.4 Lexical elements */
        case st_identifier:
            node->data.identifier = *(TNodeIdentifier*)data;
            break;
        case st_decimal_constant:
            node->data.decimal_constant = *(TNodeDecimalConstant*)data;
            break;
        case st_octal_constant:
            node->data.octal_constant = *(TNodeOctalConstant*)data;
            break;

        /* 6.5 Expressions */
        case st_unary_expression:
            node->data.unary_expression = *(TNodeUnaryExpression*)data;
            break;
        case st_multiplicative_expression:
            node->data.multiplicative_expression = *(TNodeMultiplicativeExpression*)data;
            break;
        case st_additive_expression:
            node->data.additive_expression = *(TNodeAdditiveExpression*)data;
            break;
        case st_relational_expression:
            node->data.relational_expression = *(TNodeRelationalExpression*)data;
            break;
        case st_equality_expression:
            node->data.equality_expression = *(TNodeEqualityExpression*)data;
            break;
        case st_logical_and_expression:
            node->data.logical_and_expression = *(TNodeLogicalAndExpression*)data;
            break;
        case st_logical_or_expression:
            node->data.logical_or_expression = *(TNodeLogicalOrExpression*)data;
            break;
        case st_assignment_expression:
            node->data.assignment_expression = *(TNodeAssignmentExpression*)data;
            break;
        case st_expression:
            break;

        /* 6.7 Declarators */
        case st_declaration:
            break;
        case st_declaration_specifiers:
            node->data.declaration_specifiers = *(TNodeDeclarationSpecifiers*)data;
            break;
        case st_pointer:
            node->data.pointer = *(TNodePointer*)data;
            break;
        case st_parameter_type_list:
            break;

        /* 6.8 Statements and blocks */
        case st_compound_statement:
            break;
        case st_jump_statement:
            node->data.jump_statement = *(TNodeJumpStatement*)data;
            break;
        default:
            ASSERT(0, "Unimplemented");
            break;
    }
    node->variant = var;
}

ErrorCode tree_construct(Tree* tree) {
    ASSERT(tree != NULL, "Tree is null");
    ErrorCode ecode;

    if ((ecode = tnode_alloc(&tree->root)) != ec_noerr) return ecode;
    tree->root->type = st_root;
    return ec_noerr;
}

void tree_destruct(Tree* tree) {
    tnode_destruct(tree->root);
}

TNode* tree_root(Tree* tree) {
    ASSERT(tree != NULL, "Tree is null");
    return tree->root;
}

/* Prints out the parse tree
   branch stores the branch string, e.g., "| |   | "
   i_branch is index of null terminator in branch
   max_branch_len is maximum characters which can be put in branch */
static void debug_tnode_walk(
        Tree* tree, TNode* node,
        char* branch, int i_branch, const int max_branch_len) {
    ASSERT(tree != NULL, "Tree is null");

    /* Is symbol type */
    LOGF("%s", st_str(node->type));
    switch (node->type) {
        /* 6.4 Lexical elements */
        case st_identifier:
            {
                TNodeIdentifier* data = (TNodeIdentifier*)&node->data;
                LOGF("(%s)", data->token);
            }
            break;
        case st_decimal_constant:
            {
                TNodeDecimalConstant* data = (TNodeDecimalConstant*)&node->data;
                LOGF("(%s)", data->token);
            }
            break;
        case st_octal_constant:
            {
                TNodeOctalConstant* data = (TNodeOctalConstant*)&node->data;
                LOGF("(%s)", data->token);
            }
            break;

        /* 6.5 Expressions */
        case st_unary_expression:
            {
                TNodeUnaryExpression* data = (TNodeUnaryExpression*)&node->data;
                if (data->type == TNodeUnaryExpression_ref) {
                    LOG("(& ref)");
                }
                else if (data->type == TNodeUnaryExpression_deref) {
                    LOG("(* deref)");
                }
                else if (data->type == TNodeUnaryExpression_negate) {
                    LOG("(!)");
                }
            }
            break;
        case st_multiplicative_expression:
            {
                TNodeMultiplicativeExpression* data = (TNodeMultiplicativeExpression*)&node->data;
                if (data->type == TNodeMultiplicativeExpression_mul) {
                    LOG("(*)");
                }
                else if (data->type == TNodeMultiplicativeExpression_div) {
                    LOG("(/)");
                }
                else if (data->type == TNodeMultiplicativeExpression_mod) {
                    LOG("(%)");
                }
            }
            break;
        case st_additive_expression:
            {
                TNodeAdditiveExpression* data = (TNodeAdditiveExpression*)&node->data;
                if (data->type == TNodeAdditiveExpression_add) {
                    LOG("(+)");
                }
                else if (data->type == TNodeAdditiveExpression_sub) {
                    LOG("(-)");
                }
            }
            break;
        case st_relational_expression:
            {
                TNodeRelationalExpression* data = (TNodeRelationalExpression*)&node->data;
                if (data->type == TNodeRelationalExpression_le) {
                    LOG("(<)");
                }
                else if (data->type == TNodeRelationalExpression_ge) {
                    LOG("(>)");
                }
                else if (data->type == TNodeRelationalExpression_leq) {
                    LOG("(<=)");
                }
                else if (data->type == TNodeRelationalExpression_geq) {
                    LOG("(>=)");
                }
            }
            break;
        case st_equality_expression:
            {
                TNodeEqualityExpression* data = (TNodeEqualityExpression*)&node->data;
                if (data->type == TNodeEqualityExpression_eq) {
                    LOG("(==)");
                }
                else if (data->type == TNodeEqualityExpression_neq) {
                    LOG("(!=)");
                }
            }
            break;
        case st_logical_and_expression:
            {
                TNodeLogicalAndExpression* data = (TNodeLogicalAndExpression*)&node->data;
                if (data->type == TNodeLogicalAndExpression_and) {
                    LOG("(&&)");
                }
            }
            break;
        case st_logical_or_expression:
            {
                TNodeLogicalOrExpression* data = (TNodeLogicalOrExpression*)&node->data;
                if (data->type == TNodeLogicalOrExpression_or) {
                    LOG("(||)");
                }
            }
            break;
        case st_assignment_expression:
            {
                TNodeAssignmentExpression* data = (TNodeAssignmentExpression*)&node->data;
                switch (data->type) {
                    case TNodeAssignmentExpression_none:
                        break;
                    case TNodeAssignmentExpression_assign:
                        LOG("(=)");
                        break;
                    case TNodeAssignmentExpression_mul:
                        LOG("(*=)");
                        break;
                    case TNodeAssignmentExpression_div:
                        LOG("(/=)");
                        break;
                    case TNodeAssignmentExpression_mod:
                        LOG("(%=)");
                        break;
                    case TNodeAssignmentExpression_add:
                        LOG("(+=)");
                        break;
                    case TNodeAssignmentExpression_sub:
                        LOG("(-=)");
                        break;
                    case TNodeAssignmentExpression_shl:
                        LOG("(<<=)");
                        break;
                    case TNodeAssignmentExpression_shr:
                        LOG("(>>=)");
                        break;
                    case TNodeAssignmentExpression_and:
                        LOG("(&=)");
                        break;
                    case TNodeAssignmentExpression_xor:
                        LOG("(^=)");
                        break;
                    case TNodeAssignmentExpression_or:
                        LOG("(or)");
                        break;
                    default:
                        ASSERT(0, "Unimplemented");
                    break;
                }
            }
            break;

        /* 6.7 Declarators */
        case st_declaration_specifiers:
            {
                //TNodeDeclarationSpecifiers* data = (TNodeDeclarationSpecifiers*)&node->data;
                //LOGF("(%s)\n", data->token);
            }
            break;
        case st_pointer:
            {
                TNodePointer* data = (TNodePointer*)&node->data;
                LOGF("(%d)", data->pointers);
            }
            break;

        /* 6.8 Statements and blocks */
        case st_jump_statement:
            {
                TNodeJumpStatement* data = (TNodeJumpStatement*)&node->data;
                if (data->type == TNodeJumpStatement_continue) {
                    LOGF("(continue)");
                }
                else if (data->type == TNodeJumpStatement_break) {
                    LOGF("(break)");
                }
                else if (data->type == TNodeJumpStatement_return) {
                    LOGF("(return)");
                }
            }
            break;
        default:
            break;
    }
    LOGF("\n");

    /* iterate through children */
    int child_count = 0;
    while (1) {
        if (child_count >= MAX_TNODE_CHILD) {
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
            debug_tnode_walk(
                    tree, node->child[i], branch, i_branch + 2, max_branch_len);
        }
        /* Restore original branch */
        branch[i_branch] = '\0';
    }
}

static TNode* tree_remove_single_child_impl(TNode* node) {
    int child_count = 0;
    int last_child_idx = 0;

    for (int i = 0; i < MAX_TNODE_CHILD; ++i) {
        if (node->child[i] != NULL) {
            last_child_idx = i;

            TNode* new_child = tree_remove_single_child_impl(node->child[i]);
            if (new_child) {
                tnode_destruct(node->child[i]);
                node->child[i] = new_child;
            }
            ++child_count;
        }
    }

    /* Return not NULL to indicate parent must delete this node
       replace the child with child */
    if (child_count == 1) {
        TNode* child = node->child[last_child_idx];
        node->child[last_child_idx] = NULL;
        return child;
    }
    return NULL;
}

void tree_remove_single_child(TNode* node) {
    for (int i = 0; i < MAX_TNODE_CHILD; ++i) {
        if (node->child[i] != NULL) {
            TNode* new_child = tree_remove_single_child_impl(node->child[i]);
            if (new_child) {
                tnode_destruct(node->child[i]);
                node->child[i] = new_child;
            }
        }
    }
}

void debug_print_tree(Tree* tree) {
    ASSERT(tree != NULL, "Tree is null");

    LOG("Parse tree:\n");
    /* 2 characters per node, 200 nodes max */
    int max_branch = 200 * 2;

    char branch[max_branch];
    branch[0] = '\0';
    debug_tnode_walk(tree, tree->root, branch, 0, max_branch);
}
