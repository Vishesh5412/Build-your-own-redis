#pragma once
#include "avl.h"

#include <string>
// #include <bits/stdc++.h>
using namespace std;


struct ZSet {
   AVLNode *root = NULL;
   HMap hmap;
};

struct ZNode {
   AVLNode *avl_node = NULL;
   HNode h_node;
    
   double score = 0;
   size_t sz;
   char str[0];
};

ZNode *zset_new(ZSet *zset, const char *str, size_t sz);
ZNode *zset_lookup(ZSet *zset, const char *c, size_t sz);
bool zset_insert(ZSet *zset, const char *c, size_t sz); // 
void zset_update(ZSet *zset, ZNode *znode, string str);