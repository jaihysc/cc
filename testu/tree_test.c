#include "CuTest.h"

#include "tree.h"

static void AttachDetachNode(CuTest* tc) {
    Tree tree;
    CuAssertIntEquals(tc, tree_construct(&tree), ec_noerr);

    TNode* node;
    tnode_alloca(&node, tree_root(&tree));

    TNode* node_1;
    tnode_alloca(&node_1, node);

    TNode* node_2;
    tnode_alloc(&node_2);

    tnode_attach(node, node_2);

    tree_destruct(&tree);
}

CuSuite* TreeGetSuite() {
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, AttachDetachNode);
    return suite;
}
