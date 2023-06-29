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
    htdata_t *cache_entry;
    struct openfile_node_s *prev;
    struct openfile_node_s *next;
} openfile_node_t;

static openfile_node_t *openfile_list = NULL;

openfile_node_t *
openfile_list_push(openfile_node_t **head, FILE *stream, htdata_t *cache_entry) {
    if (!head) return NULL;
    openfile_node_t *end = NULL;
    openfile_node_t *new = malloc(sizeof(openfile_node_t));
    if (*head) {
        end = *head;
        while (end->next) end = end->next;
        end->next = new;
        new->prev = end;
    } else {
        *head = new;
        new->prev = NULL;
    }
    new->next = NULL;
    new->stream = stream;
    new->cache_entry = cache_entry;
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
    
    if (*head == elem)
        *head = elem->next;

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


int
min(int a, int b) {
    return a > b ? b : a;
}


void
cache_init() {
    hashtable_new(&file_cache, CACHE_SIZE);
}

int
cached_stat(const char *file, struct stat *buf) {
    file = realpath(file, resolved_path);
    htdata_t *cache_entry = hashtable_get(&file_cache, file);
    if (cache_entry && IS_CACHED_STAT(cache_entry->flags)) {
        /* Cache hit */
        *buf = cache_entry->stat_data;
        console_log(LOG_DBG, "\t", "Cache stat hit for ", file);
        return 0;
    } else {
        /* Cache miss */
        int r = stat(file, buf);
        if (r == 0) {
            if (cache_entry) {
                /* Hash table hit */
                if (!IS_CACHED_STAT(cache_entry->flags))
                cache_entry->stat_data = *buf;
                SET_CACHED_STAT(cache_entry->flags);
            } else {
                /* Insert hash table */
                htdata_t new_entry;
                new_entry.flags = 0;
                new_entry.content_buff = NULL;
                new_entry.content_size = 0;
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
    htdata_t *cache_entry = hashtable_get(&file_cache, filename);
    if (cache_entry && IS_CACHED_CONTENT(cache_entry->flags)) {
        /* Cache hit - create our own virtual FILE stream */
        FILE *f = malloc(sizeof(FILE));
        memset(f, 0, sizeof(FILE));
        openfile_list_push(&openfile_list, f, cache_entry);
        console_log(LOG_DBG, "\t", "fopen cache hit ", filename);
        return f;
    } else {
        /* Cache miss - passthrough */
        FILE *f = fopen(filename, modes);
        if (cache_entry) {
            openfile_list_push(&openfile_list, f, cache_entry);
        } else {
            /* Insert hash table */
            htdata_t new_data = { 0 };
            hashtable_node_t *new_node = 
                hashtable_insert(&file_cache, filename, new_data);
            openfile_list_push(&openfile_list, f, &new_node->data);
        }
        console_log(LOG_DBG, "\t", "fopen cache miss ", filename);
        return f;
    }
}

size_t
cached_fread(void *ptr, size_t size, size_t n, FILE *stream) {
    openfile_node_t *openfile = openfile_list_find(openfile_list, stream);
    if (!openfile) return -1;
    if (IS_CACHED_CONTENT(openfile->cache_entry->flags)) {
        /* Cache hit - stream is virtual - copy off cache */
        size_t ncopy = min(size * n,
            openfile->cache_entry->content_size - stream->_offset);
        memcpy(ptr, openfile->cache_entry->content_buff + stream->_offset,
            ncopy);
        stream->_offset += ncopy;
        console_log(LOG_DBG, "\t", "fread cache hit ", NULL);
        return ncopy;
    } else {
        /* Cache miss - stream is real - read off disk */
        int r = fread(ptr, size, n, stream) * size;
        if (r > 0) {
            /* Read success - copy to cache */
            /* Alloc */
            if (!openfile->cache_entry->content_buff) {
                /* Nothing there - new malloc */
                openfile->cache_entry->content_buff = malloc(r);
            } else {
                /* Otherwise realloc */
                openfile->cache_entry->content_buff =
                    realloc(openfile->cache_entry->content_buff,
                        openfile->cache_entry->content_size);
            }
            memcpy(openfile->cache_entry->content_buff + 
                openfile->cache_entry->content_size, ptr, r);

            openfile->cache_entry->content_size += r;

            if (r != n * size)
                if (feof(stream))
                    SET_CACHED_CONTENT(openfile->cache_entry->flags);
        } 
        console_log(LOG_DBG, "\t", "fread cache miss ", NULL);
        return r;
    }
}

int
cached_fclose(FILE *stream) {
    openfile_list_remove(&openfile_list, stream);
    console_log(LOG_DBG, "\t", "fclose ", NULL);
    return 0;
}
