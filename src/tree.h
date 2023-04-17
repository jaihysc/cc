/* The tree built from the program */
#ifndef TREE_H
#define TREE_H

#include "constant.h"
#include "errorcode.h"

/* 6.4 Lexical elements */
typedef struct {
    char token[MAX_TOKEN_LEN + 1];
} TNodeIdentifier;

typedef struct {
    char token[MAX_TOKEN_LEN + 1];
} TNodeConstant;

/* 6.5 Expressions */
typedef struct {
    enum {
        TNodeUnaryExpression_none,
        TNodeUnaryExpression_ref,
        TNodeUnaryExpression_deref,
        TNodeUnaryExpression_negate,
    } type;
} TNodeUnaryExpression;

typedef struct {
    enum {
        TNodeBinaryExpression_none,
        TNodeBinaryExpression_add,
        TNodeBinaryExpression_sub,
        TNodeBinaryExpression_mul,
        TNodeBinaryExpression_div,
        TNodeBinaryExpression_mod,
        TNodeBinaryExpression_le,
        TNodeBinaryExpression_ge,
        TNodeBinaryExpression_leq,
        TNodeBinaryExpression_geq,
        TNodeBinaryExpression_eq,
        TNodeBinaryExpression_neq,
        TNodeBinaryExpression_logic_and,
        TNodeBinaryExpression_logic_or,
    } type;
} TNodeBinaryExpression;

typedef struct {
    enum {
        TNodeAssignmentExpression_none,
        TNodeAssignmentExpression_assign,
        TNodeAssignmentExpression_mul,
        TNodeAssignmentExpression_div,
        TNodeAssignmentExpression_mod,
        TNodeAssignmentExpression_add,
        TNodeAssignmentExpression_sub,
        TNodeAssignmentExpression_shl,
        TNodeAssignmentExpression_shr,
        TNodeAssignmentExpression_and,
        TNodeAssignmentExpression_xor,
        TNodeAssignmentExpression_or
    } type;
} TNodeAssignmentExpression;

/* 6.7 Declarators */
typedef struct {
} TNodeDeclarationSpecifiers;

typedef struct {
    int pointers;
} TNodePointer;

/* 6.8 Statements and blocks */
typedef struct {
    enum {
        TNodeJumpStatement_continue,
        TNodeJumpStatement_break,
        TNodeJumpStatement_return
    } type;
} TNodeJumpStatement;

typedef union {
    TNodeIdentifier identifier;
    TNodeConstant constant;

    TNodeUnaryExpression unary_expression;
    TNodeBinaryExpression binary_expression;
    TNodeAssignmentExpression assignment_expression;

    TNodeDeclarationSpecifiers declaration_specifiers;
    TNodePointer pointer;
    TNodeJumpStatement jump_statement;
} TNodeData;

/* Sorted by Annex A */
/* 6.4 Lexical elements */
/* 6.5 Expressions */
/* 6.7 Declarators */
/* 6.8 Statements and blocks */
/* 6.9 External definitions */
#define TNODE_TYPES                        \
    TNODE_TYPE(root)                       \
                                           \
    TNODE_TYPE(identifier)                 \
    TNODE_TYPE(constant)                   \
                                           \
    TNODE_TYPE(unary_expression)           \
    TNODE_TYPE(binary_expression)          \
    TNODE_TYPE(assignment_expression)      \
    TNODE_TYPE(expression)                 \
                                           \
    TNODE_TYPE(declaration)                \
    TNODE_TYPE(declaration_specifiers)     \
    TNODE_TYPE(pointer)                    \
    TNODE_TYPE(parameter_type_list)        \
    TNODE_TYPE(parameter_list)             \
                                           \
    TNODE_TYPE(compound_statement)         \
    TNODE_TYPE(block_item)                 \
    TNODE_TYPE(jump_statement)

#define TNODE_TYPE(name__) tt_ ## name__,
/* tt_ force compiler to choose data type with negative values
   as negative values indicate a token is stored */
typedef enum {tt_= -1, TNODE_TYPES} TNodeType;
#undef TNODE_TYPE

/* Converts TNodeType to string */
const char* tt_str(TNodeType tt);

typedef struct TNode {
    struct TNode** child; /* Array of TNode* */
    int child_count;
    int child_capacity;

    TNodeType type;
    TNodeData data;
} TNode;

/* Allocates a new TNode, stored at node_ptr */
ErrorCode tnode_alloc(TNode** node_ptr);

/* Allocates a new TNode, stored at node_ptr
   attached to parent */
ErrorCode tnode_alloca(TNode** node_ptr, TNode* parent);

/* Frees TNode and children */
void tnode_destruct(TNode* node);

/* Attaches TNode new_node onto TNode node
   node takes ownership of new_node */
ErrorCode tnode_attach(TNode* node, TNode* new_node);

/* Counts number of children for given node */
int tnode_count_child(TNode* node);

/* Retrieves the child at index for node
   negative to index backwards (-1 means last child, -2 second last) */
TNode* tnode_child(TNode* node, int i);

/* Retrieves the type for node */
TNodeType tnode_type(TNode* node);

/* Retrieves data for node
   Cast into the appropriate type based on TNodeType */
TNodeData* tnode_data(TNode* node);

/* Sets TNodeData data of type TNodeType for node
   Contents of TNodeData is copied into node */
void tnode_set(TNode* node, TNodeType st, void* data);

/* For the subtree starting at node,
   remove each node if provided cmp function returns 1 */
ErrorCode tnode_remove_if(TNode* node, int (*cmp)(TNode*));

typedef struct {
    TNode* root;
} Tree;

ErrorCode tree_construct(Tree* tree);
void tree_destruct(Tree* tree);

/* Returns the root of the tree */
TNode* tree_root(Tree* tree);

void debug_print_tree(Tree* tree);

#endif
