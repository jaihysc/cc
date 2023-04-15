/* The tree built from the program */
#ifndef TREE_H
#define TREE_H

#include "constant.h"
#include "symbol.h"

#define MAX_TREE_NODE 2000 /* Maximum nodes in tree */
#define MAX_TNODE_CHILD 20 /* Maximum children tnode */

/* 6.4 Lexical elements */
typedef struct {
    char token[MAX_TOKEN_LEN + 1];
} TNodeIdentifier;

typedef struct {
    char token[MAX_TOKEN_LEN + 1];
} TNodeDecimalConstant;

/* 6.5 Expressions */
typedef struct {
    enum {
        TNodeAdditiveExpression_none,
        TNodeAdditiveExpression_add,
        TNodeAdditiveExpression_sub,
    } type;
} TNodeAdditiveExpression;

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
    TNodeAdditiveExpression additive_expression;
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
    /* Ensure root above node buffer for pointer arithmetic to work
       (e.g., calculating index of node). Root treated as index -1
       | root | node1 | node 2 | ... */
    TNode root;
    TNode buf[MAX_TREE_NODE];
    int i_buf; /* Index one past last element */
} Tree;

ErrorCode tree_construct(Tree* tree);

/* Returns the root of the tree */
TNode* tree_root(Tree* tree);

/* Attaches a node of onto the provided parent node
   Stores the attached node in tree_ptr */
ErrorCode tree_attach(
        Tree* tree, TNode** tnode_ptr, TNode* parent);

/* Removes the children of node from the parse tree */
void tree_detach_child(Tree* tree, TNode* node);

void debug_print_tree(Tree* tree);

#endif
