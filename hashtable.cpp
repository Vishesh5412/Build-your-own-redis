#include "hashtable.h"
#include <assert.h>
#include <stdlib.h>

const size_t k_max_load_factor = 8;
static void h_init(HTab *htab, size_t n)
{
    assert(n & (n - 1) == 0);
    htab->tab = (HNode **)calloc(n, sizeof(HNode *));
    htab->mask = n - 1;
}


static void h_insert(HTab *htab, HNode *hnode)
{
    uint64_t hcode = hnode->hcode;
    
    size_t idx = (int)hcode & htab->mask;
    HNode *node = htab->tab[idx];
    hnode->next = node;
    htab->tab[idx] = hnode;
    htab->size++;
}

static void h_detach(HTab *htab, int idx) // updated it
{
    htab->tab[idx] = htab->tab[idx]->next;
    htab->size--;
}

static void h_trigger_rehashing(HMap *hmap)
{
    assert((hmap->older).tab == NULL); // check leakage
    size_t sz = (hmap->newer).size;

    hmap->older = hmap->newer;
    h_init(&(hmap->newer), (sz + 1) * 2);
    hmap->migrate_pos = 0;
}

const size_t k_rehashing_work = 128; // constant work will be done during insert(), delete(), lookup() ie. shift <= 128 hNode

static void hm_help_rehashing(HMap *hmap)
{
    size_t curr_work = 0;
    while (curr_work < k_rehashing_work && hmap->older.size > 0)
    {
        size_t idx = hmap->migrate_pos;
        while (!(hmap->older).tab[idx])
        {
            hmap->migrate_pos++;
            idx = hmap->migrate_pos;
        }

        HNode *hnode = hmap->older.tab[idx];
        h_detach(&(hmap->older), idx);
        h_insert(&(hmap->newer), hnode);
        curr_work++;
    }

    if (hmap->older.tab && hmap->older.size == 0)
    {
        free(hmap->older.tab);
        hmap->migrate_pos = 0;
        hmap->older = HTab{};
    }
}

static void hm_insert(HMap *hmap, HNode *hnode)
{
    if (!hmap->newer.tab)
        h_init(&(hmap->newer), 4); // intital tab size 4
    h_insert(&(hmap->newer), hnode);

    if (!hmap->older.tab) // if already rehashing is taking place
    {
        size_t total_bucket = hmap->newer.mask + 1;
        if (total_bucket * k_max_load_factor <= (hmap->newer.size))
            h_trigger_rehashing(hmap);
    }

    hm_help_rehashing(hmap);
}


void hm_clear(HMap *hmap)
{
    free(hmap->newer.tab);
    free(hmap->older.tab);
    *hmap = HMap{};
} // check for memory leak

size_t hm_size(HMap *hmap)
{
    return (hmap->newer.size + hmap->older.size);
}