#include "ht.h"
#include <stdint.h>
#include <string.h>

kw_ht *ht_init() {
    kw_ht *table = stil_malloc(sizeof *table);
    table->cap = KW_LEN;
    table->entries = stil_calloc(table->cap, sizeof(kw_entry));
    return table;
}

static uint64_t hash_key(const char *key) {
    uint64_t hash = FNV_OFFSET_BASIS;
    for(const char *c = key; *c; c++) {
        hash ^= (uint64_t)*c;
        hash *= FNV_PRIME;
    }
    return hash;
}

void ht_set(kw_ht *table, const char *key, int value) {
    uint64_t hash = hash_key(key);
    size_t idx = (size_t)(hash & (uint64_t)(table->cap - 1));

    while(table->entries[idx].key != NULL) {
        if(strcmp(key, table->entries[idx].key) == 0) {
            table->entries[idx].value = value;
            return;
        }
        idx++;
        /* stil_warn("COLLISION"); */

        // wrap around and check again
        if(idx >= table->cap) {
            idx = 0;
        }
    }

    table->entries[idx].key = strdup(key);
    table->entries[idx].value = value;
    /* stil_info("Inserted for %s at %zu", key, value); */
}

int ht_get(kw_ht *table, const char *key) {
    uint64_t hash = hash_key(key);
    size_t idx = (size_t)(hash & (uint64_t)(table->cap - 1));

    while(table->entries[idx].key != NULL) {
        if(strcmp(key, table->entries[idx].key) == 0) {
            return table->entries[idx].value;
        }
        idx++;
        if(idx >= table->cap) {
            idx = 0;
        }
    }

    return -1;
}
