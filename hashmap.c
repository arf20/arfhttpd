/*
                DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE 
                        Version 2, December 2004 

    Copyright (C) 2023 Mathew Jenkins <jakkleugos@gmail.com> 

    Everyone is permitted to copy and distribute verbatim or modified 
    copies of this license document, and changing it is allowed as long 
    as the name is changed. 

                DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE 
    TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION 

    0. You just DO WHAT THE FUCK YOU WANT TO.

    hashmap.c: What do you think

    Credits to lenny's shitty hashmap
    https://github.com/idkso/datastructs/blob/master/hashmap.c
    
*/

#include "hashmap.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t
hashn(const char *str, int len) {
    uint64_t out = 7;
    for (int i = 0; i < len; i++) {
        out = (out * 31) + str[i];
    }
    return out;
}

static uint64_t
hash(const char *str) {
    return hashn(str, strlen(str));
}

void
hashtable_new(hashtable_t *ht, int size) {
    ht->table = calloc(size, sizeof(hashtable_node_t));
    ht->size = size;
}

hashtable_node_t *
hashtable_rnfind(hashtable_t *ht, const char *key, int key_len) {
    hashtable_node_t *n = ht->table+(hashn(key, key_len) % ht->size);
    
lbl:
    if (n == NULL) return NULL;
    if (n->key_len == 0) return NULL;
    if (n->key_len != key_len) {
        n = n->next;
        goto lbl;
    }
    if (memcmp(n->key, key, key_len) != 0) {
        n = n->next;
        goto lbl;
    }

    return n;
}

hashtable_node_t *
hashtable_nfind(hashtable_t *ht, const char *key, int key_len) {
    hashtable_node_t *n = ht->table + (hashn(key, key_len) % ht->size);
lbl:
    if (n->key_len == 0) {
        n->key_len = key_len;
        n->key = malloc(key_len);
        memcpy((void*)n->key, key, key_len);
        return n;
    }
    
    if (n->key_len != key_len) {
        if (n->next == NULL) goto new;
        n = n->next;
        goto lbl;
    }

    if (memcmp(n->key, key, key_len) != 0) {
        if (n->next == NULL) goto new;
        n = n->next;
        goto lbl;
    } else {
        return n;
    }
new:
    n->next = calloc(1, sizeof(hashtable_node_t));
    goto lbl;
}

void
hashtable_ninsert(hashtable_t *ht, const char *key, 
                       int key_len, htdata_t value) {
    
    hashtable_node_t *n = hashtable_nfind(ht, key, key_len);
    n->data = value;
}

void
hashtable_insert(hashtable_t *ht, const char *key, htdata_t value) {
    hashtable_ninsert(ht, key, strlen(key), value);
}

htdata_t *
hashtable_nget(hashtable_t *ht, const char *key, int key_len) {
    hashtable_node_t *n = hashtable_rnfind(ht, key, key_len);
    if (n == NULL) return NULL;
    return &n->data;
}

htdata_t *
hashtable_get(hashtable_t *ht, const char *key) {
    return hashtable_nget(ht, key, strlen(key));
}


/*
int main(void) {
    int *tmp;
    struct hashtable tbl;

    const char *strings[] = {"balls balls", "balls", "sex", "jimmy", "george", "lenny is the coolest"};
    const int values[] = {69, 420, 421, 469, 42, 22};

    hashtable_new(&tbl, 64);

    struct timespec t1 = {0}, t2 = {0};
    for (int i = 0; i < 6; i++) {
        
        hashtable_insert(&tbl, strings[i], values[i]);

        printf("insert '%s' -> %d took %luns\n", strings[i], values[i], t2.tv_nsec-t1.tv_nsec);
    }

    for (int i = 0; i < 6; i++) {

        tmp = qget(&tbl, strings[i]);

        printf("get '%s' -> %d took %luns\n", strings[i], *tmp, t2.tv_nsec-t1.tv_nsec);
    }
}*/
