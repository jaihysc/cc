#include "CuTest.h"

#include "tree.h"

static void AttachDetachNode(CuTest* tc) {
    Tree tree;
    CuAssertIntEquals(tc, tree_construct(&tree), ec_noerr);

    TNode* node;
    tree_attach(&tree, &node, tree_root(&tree));

    TNode* node_1;
    tree_attach(&tree, &node_1, node);

    TNode* node_2;
    tree_attach(&tree, &node_2, node);

    CuAssertIntEquals(tc, tnode_count_child(node), 2);

    tree_detach_child(&tree, node);
    CuAssertIntEquals(tc, tnode_count_child(node), 0);
}

CuSuite* TreeGetSuite() {
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, AttachDetachNode);
    return suite;
}
