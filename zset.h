#pragma once
#include "avl.h"
#include "hashtable.h"

#include <string>
// #include <bits/stdc++.h>
using namespace std;


struct ZSet {
   AVLNode *root = NULL;
   HMap hmap;
};

struct ZNode {
   AVLNode avl_node;
   HNode h_node;
    
   double score = 0;
   size_t sz;
   char str[0];
};

ZNode *zset_new(ZSet *zset, const char *str, size_t sz);
ZNode *zset_lookup(ZSet *zset, const char *str, size_t sz);

// Find node with greatest score <= input (score, name), name = NULL refers to just find on basis of input (score) only
ZNode *floor_or_equal(ZSet *zset, double score, char *name = NULL, size_t sz = 0);

// Find node with greatest score >= input (score, name), name = NULL refers to just find on basis of input (score) only
ZNode *ceil_or_equal(ZSet *zset, double score, char *name = NULL, size_t sz = 0);

bool zset_insert(ZSet *zset, const char *str, size_t sz, double score);
void zset_update(ZSet *zset, ZNode *znode, string str);
void zset_delete(ZSet *zset, ZNode *znode);
int64_t zset_rank(ZSet *zset, ZNode *znode);
int64_t zset_count(ZSet *zset, double min_v, double max_v);

// Offset from a ZNode
ZNode *zset_offset(ZNode *znode, uint32_t offset); 


