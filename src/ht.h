#ifndef HT_H
#define HT_H

#include "shared.h"

// This is specifically for keyword lookups for the lexer
// which is why the value type is int instead of void*
// I might add another implementation when needed later
typedef struct _entry {
    const char *key;
    int value;
} kw_entry;

typedef struct _htab {
    kw_entry *entries;
    size_t cap;
} kw_ht;

// tested on 128, 256, 512, 1024
// 128 and 256 result in relatively slower lookups due to lots of collisions
// 1024 performs only slightly better than 512 so 512 it is
// since we know how many keyword entries will be made, there's no need for resizing
#define KW_LEN 512

/* Reference: www.ietf.org/archive/id/draft-eastlake-fnv-21.html */
#define FNV_OFFSET_BASIS 14695981039346656037UL
#define FNV_PRIME        1099511628211UL

kw_ht *ht_init();
void ht_set(kw_ht *table, const char *key, int value);
int ht_get(kw_ht *table, const char *key);

#endif
