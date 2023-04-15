#include "tree.h"

#include "common.h"

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
        case st_identifier:
            node->data.identifier = *(TNodeIdentifier*)data;
            break;
        case st_decimal_constant:
            node->data.decimal_constant = *(TNodeDecimalConstant*)data;
            break;
        case st_additive_expression:
            node->data.additive_expression = *(TNodeAdditiveExpression*)data;
        case st_expression:
            break;
        case st_declaration_specifiers:
            node->data.declaration_specifiers = *(TNodeDeclarationSpecifiers*)data;
            break;
        case st_pointer:
            node->data.pointer = *(TNodePointer*)data;
            break;
        case st_parameter_type_list:
            break;
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
    cmemzero(tree, sizeof(Tree));
    return ec_noerr;
}

TNode* tree_root(Tree* tree) {
    ASSERT(tree != NULL, "Tree is null");
    return &tree->root;
}

ErrorCode tree_attach(
        Tree* tree, TNode** tnode_ptr, TNode* parent) {
    ASSERT(tree != NULL, "Tree is null");
    ASSERT(parent != NULL, "Parent node is null");

    if (tree->i_buf >= MAX_TREE_NODE) {
        return ec_tnode_exceed;
    }

    TNode* node = tree->buf + tree->i_buf;
    ++tree->i_buf;

    /* Zero out children (may have previous values) */
    for (int i = 0; i < MAX_TNODE_CHILD; ++i) {
        node->child[i] = NULL;
    }

    /* Link parent node to child */
    int i = 0;
    while (1) {
        if (i >= MAX_TNODE_CHILD) {
            return ec_tnode_childexceed;
        }
        if (parent->child[i] == NULL) {
            parent->child[i] = node;
            break;
        }
        ++i;
    }

    *tnode_ptr = node;
    return ec_noerr;
}

void tree_detach_child(Tree* tree, TNode* node) {
    ASSERT(tree != NULL, "Tree is null");
    ASSERT(node != NULL, "Parse node is null");

    /* Remove children */
    for (int i = 0; i < MAX_TNODE_CHILD; ++i) {
        node->child[i] = NULL;
    }
    /* Free memory of children */
    int i_node = (int)(node - tree->buf); /* Index current node */
    tree->i_buf = i_node + 1;
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
        case st_additive_expression:
            {
                TNodeAdditiveExpression* data = (TNodeAdditiveExpression*)&node->data;
                if (data->type == TNodeAdditiveExpression_add) {
                    LOGF("(add)");
                }
                else if (data->type == TNodeAdditiveExpression_sub) {
                    LOGF("(sub)");
                }
            }
            break;
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

void debug_print_tree(Tree* tree) {
    ASSERT(tree != NULL, "Tree is null");

    LOG("Parse tree:\n");
    /* 2 characters per node */
    int max_branch = MAX_TREE_NODE * 2;

    char branch[max_branch];
    branch[0] = '\0';
    debug_tnode_walk(tree, &tree->root, branch, 0, max_branch);
}
