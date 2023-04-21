#include "tree.h"

#include "common.h"

/* Maps TNodeType to string (index with symbol type) */
#define TNODE_TYPE(name__) #name__,
const char* tnode_type_str[] = {TNODE_TYPES};
#undef TNODE_TYPES

const char* tt_str(TNodeType tt) {
    return tnode_type_str[(int)tt];
}

ErrorCode tnode_alloc(TNode** node_ptr) {
    ASSERT(node_ptr != NULL, "Node pointer is null");

    *node_ptr = ccalloc(1, sizeof(TNode));
    if (*node_ptr == NULL) return ec_badalloc;
    return ec_noerr;
}

/* Deletes members only, assumes children have been handled */
static void tnode_destruct_members(TNode* node) {
    cfree(node->child);
    cfree(node);
}

void tnode_destruct(TNode* node) {
    if (node == NULL) return;

    for (int i = 0; i < node->child_count; ++i) {
        if (node->child[i] != NULL) {
            tnode_destruct(node->child[i]);
        }
    }
    tnode_destruct_members(node);
}

ErrorCode tnode_attach(TNode* node, TNode* new_node) {
    ASSERT(node != NULL, "Node is null");
    ASSERT(new_node != NULL, "New node is null");

    /* Resize */
    if (node->child_count >= node->child_capacity) {
        node->child_capacity = node->child_capacity * 2 + 1;
        TNode** new_buf = cmalloc(node->child_capacity * sizeof(TNode));
        if (new_buf == NULL) return ec_badalloc;

        for (int i = 0; i < node->child_count; ++i) {
            new_buf[i] = node->child[i];
        }
        cfree(node->child);
        node->child = new_buf;
    }

    node->child[node->child_count++] = new_node;
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
    return node->child_count;
}

TNode* tnode_child(TNode* node, int i) {
    ASSERT(node != NULL, "Node is null");
    if (i >= 0) {
        ASSERT(i < node->child_count, "Child index out of range");
        return node->child[i];
    }

    /* Index backwads */
    ASSERT(node->child_count + i >= 0, "Child reverse index out of range");
    return node->child[node->child_count + i];
}

ErrorCode tnode_replace_child(TNode* node, TNode** new_child_ptr, int i) {
    ASSERT(node != NULL, "Node is null");

    if (i >= 0) {
        ASSERT(i < node->child_count, "Child index out of range");
    }
    else {
        ASSERT(node->child_count + i >= 0, "Child reverse index out of range");
        i = node->child_count + i;
    }

    tnode_destruct(node->child[i]);

    ErrorCode ecode;
    if ((ecode = tnode_alloc(&node->child[i])) != ec_noerr) return ecode;

    *new_child_ptr = node->child[i];
    return ec_noerr;
}

TNodeType tnode_type(TNode* node) {
    ASSERT(node != NULL, "Node is null");
    return node->type;
}

TNodeData* tnode_data(TNode* node) {
    ASSERT(node != NULL, "Node is null");
    return &node->data;
}

void tnode_set(TNode* node, TNodeType tt, void* data) {
    ASSERT(node != NULL, "Node is null");
    node->type = tt;
    switch (tt) {
        /* 6.4 Lexical elements */
        case tt_identifier:
            node->data.identifier = *(TNodeIdentifier*)data;
            break;
        case tt_new_identifier:
            node->data.new_identifier = *(TNodeNewIdentifier*)data;
            break;
        case tt_constant:
            node->data.constant = *(TNodeConstant*)data;
            break;

        /* 6.5 Expressions */
        case tt_unary_expression:
            node->data.unary_expression = *(TNodeUnaryExpression*)data;
            break;
        case tt_binary_expression:
            node->data.binary_expression = *(TNodeBinaryExpression*)data;
            break;
        case tt_assignment_expression:
            node->data.assignment_expression = *(TNodeAssignmentExpression*)data;
            break;
        case tt_expression:
            break;

        /* 6.7 Declarators */
        case tt_declaration:
            break;
        case tt_declaration_specifiers:
            node->data.declaration_specifiers = *(TNodeDeclarationSpecifiers*)data;
            break;
        case tt_pointer:
            node->data.pointer = *(TNodePointer*)data;
            break;
        case tt_parameter_type_list:
        case tt_parameter_list:
            break;

        /* 6.8 Statements and blocks */
        case tt_compound_statement:
            break;
        case tt_jump_statement:
            node->data.jump_statement = *(TNodeJumpStatement*)data;
            break;

        /* 6.9 External definitions */
        case tt_function_definition:
            break;

        default:
            ASSERT(0, "Unimplemented");
            break;
    }
}

ErrorCode tnode_remove_if(TNode* node, int (*cmp)(TNode*)) {
    ASSERT(node != NULL, "Node is null");
    ErrorCode ecode;

    for (int i = 0; i < node->child_count; ++i) {
        TNode* child = node->child[i];

        ASSERT(child != NULL, "TNode child is NULL");
        if ((ecode = tnode_remove_if(child, cmp)) != ec_noerr) return ecode;

        if (!cmp(child)) continue;
        if (child->child_count == 0) {
            /* Shuffle the nodes forward */
            for (int j = i; j < node->child_count - 1; ++j) {
                node->child[j] = node->child[j + 1];
            }
            --node->child_count;
            --i; /* Since next node moved forwards, index must move forwards */
        }
        else {
            /* Replace child with child's first child */
            node->child[i] = child->child[0];

            /* Add remaining child's children to this node */
            for (int j = 1; j < child->child_count; ++j) {
                if ((ecode = tnode_attach(
                                node, child->child[j])) != ec_noerr) return ecode;
            }
            /* Swap the children into the right position
               e.g., A -> B -> D, E
                          C
               becomes A-> D
                           E
                           C */
            for (int j = node->child_count - 1; j - child->child_count + 1 != i; --j) {
                TNode* tmp = node->child[j];
                node->child[j] = node->child[j - child->child_count + 1];
                node->child[j - child->child_count + 1] = tmp;
            }
        }
        tnode_destruct_members(child);
    }
    return ec_noerr;
}

ErrorCode tree_construct(Tree* tree) {
    ASSERT(tree != NULL, "Tree is null");
    ErrorCode ecode;

    if ((ecode = tnode_alloc(&tree->root)) != ec_noerr) return ecode;
    tree->root->type = tt_root;
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
    LOGF("%s", tt_str(node->type));
    switch (node->type) {
        /* 6.4 Lexical elements */
        case tt_identifier:
            {
                TNodeIdentifier* data = (TNodeIdentifier*)&node->data;
                LOGF("(%s)", symbol_token(data->symbol));
            }
            break;
        case tt_new_identifier:
            {
                TNodeNewIdentifier* data = (TNodeNewIdentifier*)&node->data;
                LOGF("(%s)", data->token);
            }
            break;
        case tt_constant:
            {
                TNodeConstant* data = (TNodeConstant*)&node->data;
                LOGF("(%s)", symbol_token(data->symbol));
            }
            break;

        /* 6.5 Expressions */
        case tt_unary_expression:
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
        case tt_binary_expression:
            {
                TNodeBinaryExpression* data = (TNodeBinaryExpression*)&node->data;
                switch (data->type) {
                    case TNodeBinaryExpression_none:
                        break;
                    case TNodeBinaryExpression_add:
                        LOG("(+)");
                        break;
                    case TNodeBinaryExpression_sub:
                        LOG("(-)");
                            break;
                    case TNodeBinaryExpression_mul:
                        LOG("(*)");
                            break;
                    case TNodeBinaryExpression_div:
                        LOG("(/)");
                            break;
                    case TNodeBinaryExpression_mod:
                        LOG("(%)");
                            break;
                    case TNodeBinaryExpression_l:
                        LOG("(<)");
                            break;
                    case TNodeBinaryExpression_g:
                        LOG("(>)");
                            break;
                    case TNodeBinaryExpression_le:
                        LOG("(<=)");
                            break;
                    case TNodeBinaryExpression_ge:
                        LOG("(>=)");
                            break;
                    case TNodeBinaryExpression_e:
                        LOG("(==)");
                            break;
                    case TNodeBinaryExpression_ne:
                        LOG("(!=)");
                            break;
                    case TNodeBinaryExpression_logic_and:
                        LOG("(&&)");
                            break;
                    case TNodeBinaryExpression_logic_or:
                        LOG("(||)");
                        break;
                    default:
                        ASSERT(0, "Unimplemented");
                        break;
                }
            }
            break;
        case tt_assignment_expression:
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
        case tt_declaration_specifiers:
            {
                TNodeDeclarationSpecifiers* data = (TNodeDeclarationSpecifiers*)&node->data;
                LOGF("(%s)", ts_str(data->ts));
            }
            break;
        case tt_pointer:
            {
                TNodePointer* data = (TNodePointer*)&node->data;
                LOGF("(%d)", data->pointers);
            }
            break;

        /* 6.8 Statements and blocks */
        case tt_jump_statement:
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
    for (int i = 0; i < node->child_count; ++i) {
        LOG(branch);

        /* Extend branch for to include this node */
        /* Need to add pipe, space, null terminator
           pipe overwrites existing null terminator */
        if (i_branch + 2 >= max_branch_len) {
            LOG("Max depth exceeded\n");
        }
        else {
            if (i == node->child_count - 1) {
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
    /* 2 characters per node, 200 nodes max */
    int max_branch = 200 * 2;

    char branch[max_branch];
    branch[0] = '\0';
    debug_tnode_walk(tree, tree->root, branch, 0, max_branch);
}
