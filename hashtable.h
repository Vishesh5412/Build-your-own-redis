#pragma once
#include <stddef.h>
#include <stdint.h>

struct HNode
{
    HNode *next = NULL;
    uint64_t hcode = 0;
};

struct HTab
{
    HNode **tab = NULL; // array of nodes
    size_t mask = 0; // total_bucket - 1 ie. 2 ^^ n - 1
    size_t size = 0; // total (_k, _v)
};

// stores two hash table for progressive resizing
struct HMap
{
    HTab newer;
    HTab older;
    size_t migrate_pos = 0; // current bucket idx in tab waiting to migrate in newer
};

HNode *hm_lookup(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *));
void hm_insert(HMap *hmap, HNode *hnode);
HNode *hm_delete(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *));
size_t hm_size(HMap *hmap);
void hm_clear(HMap *hmap);

