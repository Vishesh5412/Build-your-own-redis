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

static void tree_add(ZSet *zset, ZNode *new_node)
{
    AVLNode *avl_node = &(new_node->avl_node);

    AVLNode **from = &(zset->root), *curr = zset->root, *par = NULL;
    while (curr)
    {
        ZNode *znode = container_of(curr, ZNode, avl_node);

        if (znode->score == new_node->score)
        {
            size_t min_len = min(znode->sz, new_node->sz);
            int res = strncmp(znode->str, new_node->str, min_len);

            from = (res > 0 || min_len == new_node->sz) ? &(znode->avl_node.left) : &(znode->avl_node.right);
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

ZNode *floor_or_equal(ZSet *zset, double score, char *name, size_t sz)
{
    if (zset == NULL)
        return NULL;

    ZNode *ans = NULL;

    AVLNode *curr = zset->root;
    while (curr)
    {
        ZNode *znode = container_of(curr, ZNode, avl_node);

        if (znode->score > score)
        {
            curr = curr->left;
        }
        else if (znode->score < score || !name)
        {

            ans = znode;
            curr = curr->right;
        }
        else // score equal, name have to be lexically greater or equal than znode->str
        {
            size_t min_len = min(znode->sz, sz);
            size_t max_len = max(znode->sz, sz);
            if (strncmp(znode->str, name, min_len) <= 0 && max_len == sz)
            {
                ans = znode;
                curr = curr->right;
            }
        }
    }

    return ans;
}

ZNode *ceil_or_equal(ZSet *zset, double score, char *name, size_t sz)
{ // find node with greatest score <= input (score, name), name = NULL refers to just find on basis of input (score) only
    if (zset == NULL)
        return NULL;

    ZNode *ans = NULL;

    AVLNode *curr = zset->root;
    while (curr)
    {
        ZNode *znode = container_of(curr, ZNode, avl_node);

        if (znode->score < score)
        {
            curr = curr->right;
        }
        else if (znode->score > score || !name)
        {

            ans = znode;
            curr = curr->left;
        }
        else // score equal, znode->str have to be lexically greater or equal than name
        {
            size_t min_len = min(znode->sz, sz);
            size_t max_len = max(znode->sz, sz);
            if (strncmp(znode->str, name, min_len) >= 0 && max_len == znode->sz)
            {
                ans = znode;
                curr = curr->right;
            }
        }
    }

    return ans;
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
    zset->root = avl_del(&znode->avl_node);
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
    tree_add(zset, znode);

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

    zset->root = avl_del(&(znode->avl_node));
}

int64_t zset_rank(ZSet *zset, ZNode *znode)
{ // rank of first tuple >= (score, name)
    if (zset == NULL || znode == NULL)
        return 0; // invalid
    return avl_rank(&znode->avl_node);
}

int64_t zset_count(ZSet *zset, double min_v, double max_v)
{
    if (min_v > max_v)
        return 0;

    ZNode *znode1 = ceil_or_equal(zset, min_v);  // for min_v
    ZNode *znode2 = floor_or_equal(zset, max_v); // for max_v

    if (znode1 == NULL || znode2 == NULL)
        return 0;
    return (zset_rank(zset, znode2) - zset_rank(zset, znode1) + 1);
}

ZNode *zset_offset(ZNode *znode, uint32_t offset)
{
    assert(znode);
    if (offset == 0)
        return znode;

    AVLNode *root = &(znode->avl_node);

    uint32_t l_cnt = root->left ? (root->left)->cnt : 0;
    uint32_t r_cnt = root->right ? (root->right)->cnt : 0;

    if (r_cnt < offset)
    {
        AVLNode *par = root->par;
        if (!par)
            return NULL;

        ZNode *z_par = container_of(par, ZNode, avl_node);
        if (root == par->left)
            return zset_offset(z_par, offset - (r_cnt + 1));

        return zset_offset(z_par, offset + (l_cnt + 1));
    }

    // find inorder successor
    root = root->right;
    while (root->left)
        root = root->left;

    ZNode *z_right = container_of(root, ZNode, avl_node);
    return zset_offset(z_right, offset - 1);
}