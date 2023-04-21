/* Converts Tree nodes from parser to Intermediate language 2 (IL2) nodes */
#ifndef IL2GEN_H
#define IL2GEN_H

#include "cfg.h"
#include "symtab.h"
#include "tree.h"

typedef struct {
    Cfg* cfg;
    Symtab* stab;
    Tree* tree;
} IL2Gen;

ErrorCode il2_construct(IL2Gen* il2, Cfg* cfg, Symtab* stab, Tree* tree);

/* Converts Tree nodes from parser to IL2 nodes
   The nodes are stored in the tree */
ErrorCode il2_gen(IL2Gen* il2);

#endif
