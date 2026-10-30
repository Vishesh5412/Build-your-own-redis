#include <bits/stdc++.h>
#include "avl.h"
using namespace std;

static void update(AVLNode *node)
{
    node->cnt = avl_cnt(node);
    node->h = avl_height(node);
}

static int getBF(AVLNode *node)
{
    uint32_t h1 = node->left ? (node->left)->h : 0;
    uint32_t h2 = node->right ? (node->right)->h : 0;

    return ((int)h1 - (int)h2);
}

static AVLNode *rotate_right(AVLNode *root)
{
    if (root == NULL)
        return NULL;
    AVLNode *new_root = root->left;
    assert(new_root);
    root->left = new_root ? new_root->right : NULL;

    new_root->par = root->par;
    if (new_root->right)
        (new_root->right)->par = root;
    new_root->right = root;

    root->par = new_root;

    update(root);
    update(new_root);
    return new_root;
}

static AVLNode *rotate_left(AVLNode *root)
{
    if (root == NULL)
        return NULL;
    AVLNode *new_root = root->right;
    assert(new_root);
    root->right = new_root ? new_root->left : NULL;

    new_root->par = root->par;
    if (new_root->left)
        (new_root->left)->par = root;
    new_root->left = root;

    root->par = new_root;
    update(root);
    update(new_root);
    return new_root;
}

AVLNode *avl_fix_add(AVLNode *node)
{
    if (node == NULL)
        return NULL;

    AVLNode *curr = node, *par = node->par;

    char dir[2];
    size_t idx = 1;

    while (curr)
    {
        update(curr);

        int bf = getBF(curr);
        if (bf > 1)
        {
            if (dir[0] == 'L' && dir[1] == 'R')
            { // LR rotation
                curr->left = rotate_left(curr->left);
            }
            curr = rotate_right(curr);
        }
        else if (bf < -1)
        {
            if (dir[0] == 'R' && dir[1] == 'L')
            { // RL rotation
                curr->right = rotate_right(curr->right);
            }
            curr = rotate_left(curr);
        }

        if (!par)
            return curr;

        dir[idx] = (curr == par->left) ? 'L' : 'R';
        idx = (idx + 1) % 2; // toggle idx

        curr = par;
        par = curr ? curr->par : NULL;
    }
    return curr;
}

AVLNode *avl_fix_del(AVLNode *node, char dir)
{
    assert(dir == 'L' || dir == 'R');
    if (node == NULL)
        return NULL;

    AVLNode *curr = node, *par = node->par;
    while (curr)
    {
        int bf = getBF(curr);
        if (bf > 1 || bf < 1)
        {
            if (dir == 'L')
            {
                if (getBF(curr->right) == 0)
                {
                    curr->right = rotate_right(curr->right);
                }
                curr = rotate_left(curr);
            }
            else
            {
                if (getBF(curr->left) == -1)
                {
                    curr->left = rotate_left(curr->left);
                }
                curr = rotate_right(curr);
            }
        }

        if (!par)
            return curr;

        dir = (curr == par->left) ? 'L' : 'R';

        curr = par;
        par = curr ? curr->par : NULL;
    }
    return curr;
}

AVLNode *avl_del(AVLNode *node)
{
    if (node == NULL)
        return NULL;

    if (node->left == NULL || node->right == NULL)
    {

        AVLNode *parent = node->par;
        AVLNode *child = node->left ? node->left : node->right;
        char dir = (parent->left == node) ? 'L' : 'R';

        if (child)
        {
            child->par = parent;

            parent->left = (dir == 'L') ? child : parent->left;
            parent->right = (dir == 'R') ? child : parent->right;
        }

        node->left = NULL;
        node->right = NULL;
        node->par = NULL;
        delete node;
        return avl_fix_del(parent, dir);
    }

    AVLNode *i_s = node->right;
    while (i_s->left)
        i_s = i_s->left;

    // swap(contain)
    // swap(*node, *i_s); // swapping with inorder successor
    return avl_del(i_s);
}
