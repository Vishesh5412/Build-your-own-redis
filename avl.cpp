#include <bits/stdc++.h>
#include "avl.h"
using namespace std;

#define container_of(ptr, T, member) \
    ((T *)((char *)ptr - offsetof(T, member)))

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

    AVLNode *prev = NULL, *curr = node, *par = node->par;

    char dir[2] = {'\0', '\0'};

    while (curr)
    {
        update(curr);

        dir[0] = prev ? ((prev == curr->left) ? 'L' : 'R') : '\0';
        int bf = getBF(curr);
        AVLNode **from = par ? (curr == par->left) ? &(par->left) : &(par->right) : NULL;

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

        // a sliding window of size 2
        dir[1] = dir[0];

        *from = curr; // updating pointer from parent
        prev = curr;
        curr = par;
        par = curr ? curr->par : NULL;
    }
    return curr;
}

AVLNode *avl_fix_del(AVLNode *node, char dir)
{
    if (node == NULL)
        return NULL;

    assert(dir == 'L' || dir == 'R');
    AVLNode *prev = NULL, *curr = node, *par = node->par;
    while (curr)
    {
        update(curr); // update h, cnt
        
        int bf = getBF(curr);
        AVLNode **from = par ? (curr == par->left) ? &(par->left) : &(par->right) : NULL;

        if (bf < -1 || bf > 1)
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

        *from = curr;
        prev = curr;
        curr = par;
        dir = (prev == curr->left) ? 'L' : 'R';

        par = (curr) ? curr->par : NULL;
    }
    return curr;
}

AVLNode *avl_del(AVLNode *node)
{
    if (node == NULL)
        return NULL;

    if (node->left == NULL || node->right == NULL)
    {
        AVLNode *par = node->par;
        AVLNode *child = node->left ? node->left : node->right;

        char dir = par ? ((par->left == node) ? 'L' : 'R') : '\0';

        AVLNode **from = par ? ((node == par->left) ? &(par->left) : &(par->right)) : NULL;
        *from = child ? child : NULL;

        if (child)
            child->par = par;

        if (!par)
        {
            return child;
        }

        return avl_fix_del(par, dir);
    }

    AVLNode *i_s = node->right;
    while (i_s->left)
        i_s = i_s->left;

    if (i_s->par == node) // when successor is just the right of node to be deleted
    {
        AVLNode **from = (node->par) ? (node == (node->par)->left ? &(node->par->left) : &(node->par->right)) : NULL;
        if (node->par)
            *from = i_s;

        node->right = i_s->right; // to use later by i_s
        *i_s = *node;
        i_s->left->par = i_s;

        return avl_fix_del(i_s, 'R');
    }

    AVLNode *par = i_s->par; // node from which fix fxn will called
    
    par->left = i_s->right;
    if (i_s->right)
    i_s->right->par = par;
    
    *i_s = *node;
    
    (i_s->left)->par = i_s;
    (i_s->right)->par = i_s;
     
    // rechanging pointers associated with node
    AVLNode **from = (node->par) ? (node == (node->par)->left ? &(node->par->left) : &(node->par->right)) : NULL;
    if (node->par)
        *from = i_s;
    
    return avl_fix_del(par, 'L');
}
