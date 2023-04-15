/* The tree built from the program */
#ifndef TREE_H
#define TREE_H

#include "constant.h"
#include "symbol.h"

#define MAX_TNODE_CHILD 256 /* Maximum children tnode */

/* 6.4 Lexical elements */
typedef struct {
    char token[MAX_TOKEN_LEN + 1];
} TNodeIdentifier;

typedef struct {
    char token[MAX_TOKEN_LEN + 1];
} TNodeDecimalConstant;

typedef struct {
    char token[MAX_TOKEN_LEN + 1];
} TNodeOctalConstant;

/* 6.5 Expressions */
typedef struct {
    enum {
        TNodeAdditiveExpression_none,
        TNodeAdditiveExpression_add,
        TNodeAdditiveExpression_sub,
    } type;
} TNodeAdditiveExpression;

typedef struct {
    enum {
        TNodeAssignmentExpression_none,
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
    TNodeDecimalConstant decimal_constant;
    TNodeOctalConstant octal_constant;

    TNodeAdditiveExpression additive_expression;
    TNodeAssignmentExpression assignment_expression;

    TNodeDeclarationSpecifiers declaration_specifiers;
    TNodePointer pointer;
    TNodeJumpStatement jump_statement;
} TNodeData;

typedef struct TNode {
    struct TNode* child[MAX_TNODE_CHILD];
    SymbolType type;
    TNodeData data;
    /* The rule which was matched for this node
       Starting with 0 as first, in the order given by Annex A */
    int variant;
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

/* Retrieves the symbol type for node */
SymbolType tnode_type(TNode* node);

/* Retrieves data for node
   Cast into the appropriate type based on SymbolType */
TNodeData* tnode_data(TNode* node);

/* Retrieves the variant for node */
int tnode_variant(TNode* node);

/* Sets TNodeData data of type SymbolType for node
   Contents of TNodeData is copied into node */
void tnode_set(TNode* node, SymbolType st, void* data, int var);

typedef struct {
    TNode* root;
} Tree;

ErrorCode tree_construct(Tree* tree);
void tree_destruct(Tree* tree);

/* Returns the root of the tree */
TNode* tree_root(Tree* tree);

void debug_print_tree(Tree* tree);

#endif
