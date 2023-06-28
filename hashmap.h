/*

    arfhttpd: Yet another HTTP server
    Copyright (C) 2023 arf20 (Ángel Ruiz Fernandez)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#include <sys/stat.h>
#include <stddef.h>

/* 10
   CS */
#define IS_CACHED_STAT(x)       (x & 1)
#define IS_CACHED_CONTENT(x)    ((x >> 1) & 1)
#define SET_CACHED_STAT(x)      (x |= 1)
#define SET_CACHED_CONTENT(x)   (x |= (1 << 1))

typedef struct nodedata_s {
    struct stat stat_data;
    char *fbuff;
    size_t buffsize;
    char flags;
} htdata_t;

typedef struct hashtable_node_s {
    struct hashtable_node_s *next;
    const char *key;
    int key_len;
    htdata_t data;
} hashtable_node_t;

typedef struct hashtable_s {
    hashtable_node_t *table;
    int size;
} hashtable_t;

void hashtable_new(hashtable_t *ht, int size);
void hashtable_insert(hashtable_t *ht, const char *key, htdata_t value);
htdata_t *hashtable_get(hashtable_t *ht, const char *key);
