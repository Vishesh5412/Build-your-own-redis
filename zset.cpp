#include <bits/stdc++.h>
#include <string.h>

#include "zset.h"
// #include "avl.h"
#include "util.h"
#include "hashtable.h"
#include "avl.h"

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

static void compare(ZNode *node, ZNode *new_node, AVLNode **from)
{
    size_t min_len = min(node->sz, new_node->sz);
    for (size_t i = 0; i < min_len; i++)
    {
        if (node->str[i] == new_node->str[i])
            continue;
        *from = (new_node->str[i] < node->str[i]) ? (node->avl_node.left) : (node->avl_node.right);
        return;
    }
    *from = node->avl_node.right;
}

static void tree_add(ZSet *zset, ZNode *new_node)
{
    AVLNode *avl_node = &(new_node->avl_node);

    AVLNode **from = &(zset->root), *curr = zset->root, *par = NULL;
    while (curr)
    {
        ZNode *znode = container_of(curr, ZNode, avl_node);

        if (znode->score == new_node->score)
        {
            compare(znode, new_node, from);
        }
        else
        {
            from = (znode->score > new_node->score) ? &(curr->left) : &(curr->right);
        }

        par = curr;
        curr = *from;
    }

    *from = avl_node;
    avl_node->par = par;

    zset->root = avl_fix_add(avl_node);
}

static ZNode *zset_new(const char *str, size_t sz, double score)
{
    ZNode *znode = (ZNode *)calloc(1, sizeof(ZNode) + sz);

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

static void zset_update(ZSet *zset, ZNode *znode, double score)
{
    AVLNode *new_root = avl_del(&znode->avl_node);
    zset->root = new_root;

    znode->score = score;
    tree_add(zset, znode);
}

bool zset_insert(ZSet *zset, const char *str, size_t sz, double score)
{
    ZNode *node = zset_lookup(zset, str, sz);
    if (node)
    {
        zset_update(zset, node, score);
        return false;
    }

    ZNode *znode = zset_new(str, sz, score);
    hm_insert(&zset->hmap, &znode->h_node);
    // tree_add(zset, znode);
    
    return true;
}

void zset_delete(ZSet *zset, ZNode *znode)
{
    if (!znode)
        return;

    HKey hkey;
    hkey.node.hcode = str_hash((uint8_t *)znode->str, znode->sz);
    hkey.str = znode->str;
    hkey.sz = znode->sz;

    hm_delete(&(zset->hmap), &(hkey.node), &eq);
    
    // AVLNode *new_root = avl_del(&(znode->avl_node));
    // zset->root = new_root;
    // delete &znode->avl_node;
    // delete znode;
}