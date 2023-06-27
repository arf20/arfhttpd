#include <sys/stat.h>
#include <stddef.h>

typedef struct nodedata_s {
    struct stat stat_data;
    char *fbuff;
    size_t buffsize;
} nodedata_t;

typedef struct hashtable_node_s {
    struct hashtable_node_s *next;
    const char *key;
    int key_len;
    nodedata_t data;
} hashtable_node_t;

typedef struct hashtable_s {
    hashtable_node_t *table;
    int size;
} hashtable_t;

void hashtable_new(hashtable_t *ht, int size);
void hashtable_insert(hashtable_t *ht, const char *key, nodedata_t value);
nodedata_t *hashtable_get(hashtable_t *ht, const char *key);
