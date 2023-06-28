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

    cache.c: file cache

*/

#include "cache.h"

#include "hashmap.h"
#include "log.h"

#include <sys/stat.h>

#include <limits.h>
#include <string.h>
#include <stdlib.h>


static char resolved_path[PATH_MAX];

static hashtable_t file_cache;

/* Open file list */
typedef struct openfile_node_s {
    FILE *stream;
    struct openfile_node_s *prev;
    struct openfile_node_s *next;
} openfile_node_t;

static openfile_node_t *openfile_list = NULL;

openfile_node_t *
openfile_list_push(openfile_node_t **head, FILE *stream) {
    if (!head) return NULL;
    openfile_node_t *end = NULL;
    openfile_node_t *new = malloc(sizeof(openfile_node_t));
    if (*head) {
        end = *head;
        while (end->next) end = end->next;
        end->next = new;
        new->prev = *head;
    } else {
        *head = new;
    }
    new->next = NULL;
    new->stream = stream;
    return new;
}

openfile_node_t *
openfile_list_find(openfile_node_t *head, const FILE *stream) {
    if (!head) return NULL;
    openfile_node_t *ptr = head;
    while (ptr) {
        if (ptr->stream == stream) return ptr;
        ptr = ptr->next;
    }
    return NULL;
}

void
openfile_list_remove(openfile_node_t **head, const FILE *stream) {
    if (!head) return;
    openfile_node_t *elem = openfile_list_find(*head, stream);
    
    if (!elem->prev)
        if (elem->next)
            elem->next->prev == NULL;

    if (!elem->next)
        if (elem->next)
            elem->next->prev == NULL;

    if (elem->next && elem->prev) {
        elem->prev->next = elem->next;
        elem->next->prev = elem->prev;
    }
    
    free(elem->stream);
    free(elem);
}


void
cache_init() {
    hashtable_new(&file_cache, CACHE_SIZE);
}

int
cached_stat(const char *file, struct stat *buf) {
    file = realpath(file, resolved_path);
    htdata_t *cached_entry = hashtable_get(&file_cache, file);
    if (cached_entry && IS_CACHED_STAT(cached_entry->flags)) {
        /* Cache hit */
        *buf = cached_entry->stat_data;
        console_log(LOG_DBG, "\t", "Cache stat hit for ", file);
        return 0;
    } else {
        /* Cache miss */
        int r = stat(file, buf);
        if (r == 0) {
            if (cached_entry) {
                /* Hash table hit */
                if (!IS_CACHED_STAT(cached_entry->flags))
                cached_entry->stat_data = *buf;
                SET_CACHED_STAT(cached_entry->flags);
            } else {
                /* Insert hash table */
                htdata_t new_entry;
                new_entry.flags = 0;
                new_entry.fbuff = NULL;
                new_entry.buffsize = 0;
                new_entry.stat_data = *buf;
                SET_CACHED_STAT(new_entry.flags);
                hashtable_insert(&file_cache, file, new_entry);
            }
            console_log(LOG_DBG, "\t", "Cached stat for ", file);
        }
        return r;
    }
}

FILE *
cached_fopen(const char *filename, const char *modes) {
    if (!filename || !modes) return NULL;
    htdata_t *cached_entry = hashtable_get(&file_cache, filename);
    if (cached_entry && IS_CACHED_CONTENT(cached_entry->flags)) {
        /* Cache hit */
        FILE *f = malloc(sizeof(FILE));
        memset(f, 0, sizeof(FILE));
        openfile_list_push(&openfile_list, f);
        return f;
    } else {
        /* Cache miss - passthrough */
        FILE *f = fopen(filename, modes);
        if (!cached_entry) {
            /* Insert hash table */
            htdata_t new_entry = { 0 };
            hashtable_insert(&file_cache, filename, new_entry);
        }
        openfile_list_push(&openfile_list, f);
        return f;
    }
}

size_t
cached_fread(void *ptr, size_t size, size_t n, FILE *stream) {

}

int
cached_fclose(FILE *stream) {

}
