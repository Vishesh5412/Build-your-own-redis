// #include <bits/stdc++.h>
#include <string.h>

#include "zset.h"
#include "avl.h"
#include "util.h"
#include "hashtable.h"

using namespace std;

// a helper structure for the hashtable lookup
struct HKey
{
    HNode node;
    const char *str = NULL;
    size_t sz = 0;
};

static bool eq(HNode *n1, HNode *n2) // n1 stored in znode, n2 is pointer stored in hkey
{
    ZNode *e1 = container_of(n1, ZNode, h_node);
    HKey *e2 = container_of(n2, HKey, node);
    if (e1->sz != e2->sz)
        return false;

    return (memcmp((void *)e1->str, (void *)e2->str, e1->sz) == 0); // strings are exactly same
}

// static void tree_add(Container &c, uint32_t val)
static void tree_add(ZSet *zset, AVLNode *avl_node, double score)
{
    AVLNode **from = &(zset->root), *curr = zset->root, *par = NULL;
    while (curr)
    {
        ZNode *znode = container_of(curr, ZNode, avl_node);
        AVLNode *avl_node = znode->avl_node;

        if (znode->score == score)
            return;

        par = curr;
        from = (znode->score > score) ? &(avl_node->left) : &(avl_node->right);
        curr = *from;
    }

    *from = avl_node;
    avl_node->par = par;

    zset->root = avl_fix_add(avl_node);
}

ZNode *zset_new(const char *str, size_t sz, double score)
{
    ZNode *znode = (ZNode *)calloc(1, sizeof(ZNode) + sz);

    znode->avl_node = new AVLNode;
    znode->h_node.hcode = str_hash((const uint8_t *)str, sz);

    znode->score = score;
    znode->sz = sz;
    memcpy(znode->str, str, sz);
    return znode;
}

ZNode *zset_lookup(ZSet *zset, const char *str, size_t sz)
{
    if (zset->root == NULL)
        return NULL;

    HKey hkey;
    hkey.node.hcode = str_hash((uint8_t *)str, sz);
    hkey.str = str;
    hkey.sz = sz;

    HNode *hnode = hm_lookup(&(zset->hmap), &hkey.node, &eq);
    if (!hnode)
        return NULL;

    ZNode *znode = container_of(hnode, ZNode, h_node);
    return znode;
}

bool zset_insert(ZSet *zset, const char *str, size_t sz, double score)
{
    if (zset_lookup(zset, str, sz))
    {
        // zset update
        return false;
    }

    ZNode *znode = zset_new(str, sz, score);
    hm_insert(&zset->hmap, &znode->h_node);
    tree_add(zset, znode->avl_node, score);
    return true;
}

// ZNode *zset_lookup(ZSet *zset, const char *str, size_t sz) {

// }
