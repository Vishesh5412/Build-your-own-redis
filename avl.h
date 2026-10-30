#pragma once
#include <stddef.h>
#include <stdint.h>

struct AVLNode
{
   struct AVLNode *left = NULL;
   struct AVLNode *right = NULL;
   struct AVLNode *par = NULL;
   uint32_t h = 1;
   uint32_t cnt = 1;
};

inline uint32_t avl_height(AVLNode *node)
{
   if (node == NULL) return 0;
   return (1 + (node->left ? (node->left)->h : 0) + (node->right ? (node->right)->h : 0));
}

inline uint32_t avl_cnt(AVLNode *node) {
   if (node == NULL) return 0;
   return (1 + (node->left ? (node->left)->cnt : 0) + (node->right ? (node->right)->cnt : 0));
}

AVLNode *avl_fix_add(AVLNode *node); // make sure tree is avl
AVLNode *avl_del(AVLNode *node);