/* The tree built from the program */
#ifndef TREE_H
#define TREE_H

#include "symbol.h"

#define MAX_TREE_NODE 2000 /* Maximum nodes in tree */
#define MAX_TNODE_CHILD 4 /* Maximum children tnode */

typedef struct TNode {
    struct TNode* child[MAX_TNODE_CHILD];
    SymbolType type;
} TNode;

/* Counts number of children for given node */
int tnode_count_child(TNode* node);

/* Retrieves the child at index for node
   negative to index backwards (-1 means last child, -2 second last) */
TNode* tnode_child(TNode* node, int i);

/* Retrieves the symbol type for node */
SymbolType tnode_type(TNode* node);

typedef struct {
    /* Ensure root above node buffer for pointer arithmetic to work
       (e.g., calculating index of node). Root treated as index -1
       | root | node1 | node 2 | ... */
    TNode root;
    TNode buf[MAX_TREE_NODE];
    int i_buf; /* Index one past last element */
} Tree;

ErrorCode tree_construct(Tree* tree);

/* Attaches a node of onto the provided parent node
   null to attach onto root
   Stores the attached node in tree_ptr */
ErrorCode tree_attach(
        Tree* tree, TNode** tnode_ptr, TNode* parent);

/* Removes the children of node from the parse tree */
void tree_detach_child(Tree* tree, TNode* node);

void debug_print_tree(Tree* tree);

#endif
