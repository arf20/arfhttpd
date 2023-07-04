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

    cache.c: File cache

*/

#include "cache.h"

#include "hashmap.h"
#include "log.h"

#include <sys/stat.h>
#include <sys/inotify.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include <limits.h>
#include <string.h>
#include <stdlib.h>


static char resolved_path[PATH_MAX];

static hashtable_t file_cache;

static int infd = 0;

/* Open file list */
typedef struct openfile_node_s {
    CACHED_FILE *stream;
    htdata_t *cache_entry;
    struct openfile_node_s *prev;
    struct openfile_node_s *next;
} openfile_node_t;

static openfile_node_t *openfile_list = NULL;

openfile_node_t *
openfile_list_push(openfile_node_t **head, CACHED_FILE *stream, htdata_t *cache_entry) {
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
openfile_list_find(openfile_node_t *head, const CACHED_FILE *stream) {
    if (!head) return NULL;
    openfile_node_t *ptr = head;
    while (ptr) {
        if (ptr->stream == stream) return ptr;
        ptr = ptr->next;
    }
    return NULL;
}

void
openfile_list_remove(openfile_node_t **head, const CACHED_FILE *stream) {
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


hashtable_node_t *
hashtable_find_wd(int wd) {
    /* @lenny this uhh how are you supposed to fuckin iterate through your
       hash table already */
    hashtable_node_t *file_cache_current = NULL;
    for (int i = 0; i < file_cache.size; i++) {
        file_cache_current = file_cache.table + i;
        if (!file_cache_current->key) continue;
        while (file_cache_current) {
            if (file_cache_current->data.wd == wd)
                return file_cache_current;
            file_cache_current = file_cache_current->next;
        }
    }
    return NULL;
}


int
min(int a, int b) {
    return a > b ? b : a;
}

void *inotify_poll_loop(void *ptr);

/* Exports */

int
cache_init() {
    /* Allocate hash table */
    hashtable_new(&file_cache, CACHE_SIZE);
    /* Initialise inotify API */
    infd = inotify_init1(IN_NONBLOCK);
    if (infd == -1) {
        printf("Error initialising inotify_init1\n");
    }
    /* Begin inotify poller thread */
    pthread_t inpoll_thread;
    pthread_create(&inpoll_thread, NULL, inotify_poll_loop, NULL);
    pthread_detach(inpoll_thread);
}

int
cached_stat(const char *file, struct stat *buf) {
    file = realpath(file, resolved_path); /* Accesses disk, find alternative */
    if (!file) {
        return -1;
    }
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
                htdata_t new_data;
                new_data.flags = 0;
                new_data.content_buff = NULL;
                new_data.content_size = 0;
                new_data.stat_data = *buf;
                SET_CACHED_STAT(new_data.flags);
                /* Add inotify watch */
                new_data.wd = inotify_add_watch(infd, file, IN_MODIFY);
                if (new_data.wd < 0)
                    console_log(LOG_ERR, "\t", "Cannot watch ", file);
                else console_log(LOG_DBG, "\t", "Watching ", file);
                hashtable_insert(&file_cache, file, new_data);
            }
            console_log(LOG_DBG, "\t", "Cached stat for ", file);
        }
        return r;
    }
}

CACHED_FILE *
cached_fopen(const char *filename, const char *modes) {
    if (!filename || !modes) return NULL;
    htdata_t *cache_entry = hashtable_get(&file_cache, filename);
    if (cache_entry && IS_CACHED_CONTENT(cache_entry->flags)) {
        /* Cache hit - create our own virtual FILE stream */
        CACHED_FILE *f = malloc(sizeof(CACHED_FILE));
        f->actual_stream = NULL;
        f->_offset = 0;
        openfile_list_push(&openfile_list, f, cache_entry);
        console_log(LOG_DBG, "\t", "fopen cache hit ", filename);
        return f;
    } else {
        /* Cache miss - passthrough */
        CACHED_FILE *f = malloc(sizeof(CACHED_FILE));
        f->_offset = 0;
        f->actual_stream = fopen(filename, modes);
        if (cache_entry) {
            openfile_list_push(&openfile_list, f, cache_entry);
        } else {
            /* Insert hash table */
            htdata_t new_data = { 0 };
            /* Add inotify watch */
            new_data.wd = inotify_add_watch(infd, filename, IN_MODIFY);
            if (new_data.wd < 0)
                console_log(LOG_ERR, "\t", "Cannot watch ", filename);
            else console_log(LOG_DBG, "\t", "Watching ", filename);
            hashtable_node_t *new_node = 
                hashtable_insert(&file_cache, filename, new_data);
            openfile_list_push(&openfile_list, f, &new_node->data);
        }
        console_log(LOG_DBG, "\t", "fopen cache miss ", filename);
        return f;
    }
}

size_t
cached_fread(void *ptr, size_t size, size_t n, CACHED_FILE *stream) {
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
        if (!stream->actual_stream) return -1;
        int r = fread(ptr, size, n, stream->actual_stream) * size;
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
                if (feof(stream->actual_stream))
                    SET_CACHED_CONTENT(openfile->cache_entry->flags);
        } 
        console_log(LOG_DBG, "\t", "fread cache miss ", NULL);
        return r;
    }
}

int
cached_fclose(CACHED_FILE *stream) {
    if (stream->actual_stream) fclose(stream->actual_stream);
    openfile_list_remove(&openfile_list, stream);
    console_log(LOG_DBG, "\t", "fclose ", NULL);
    return 0;
}


/* inotify invalidator */
void *
inotify_poll_loop(void *ptr) {
    int poll_num = 0;
    nfds_t nfds = 1;
    struct pollfd fds;
    fds.fd = infd;
    fds.events = POLLIN;

    char buf[4096]
        __attribute__ ((aligned(__alignof__(struct inotify_event))));
    const struct inotify_event *event;
    size_t len;

    while (1) {
        poll_num = poll(&fds, nfds, -1);
        if (poll_num == -1) {
            if (errno == EINTR)
                continue;
            console_log(LOG_ERR, "\t", "Error polling inotify ", NULL);
        }

        if (fds.revents & POLLIN) {
            /* inotify events are available. */
            len = read(infd, buf, sizeof(buf));
            if (len == -1 && errno != EAGAIN) {
                console_log(LOG_ERR, "\t", "Error reading inotify ", NULL);
            }

            if (len <= 0)
                continue;
            
            for (char *ptr = buf; ptr < buf + len;
                ptr += sizeof(struct inotify_event) + event->len)
            {
                event = (const struct inotify_event *) ptr;
                /* Find corresponding cache entry */
                hashtable_node_t *entry = hashtable_find_wd(event->wd);
                if (entry) {
                    /* Assuming IN_MODIFIED, clear cached flags */
                    CLEAR_CACHED_STAT(entry->data.flags);
                    CLEAR_CACHED_CONTENT(entry->data.flags);
                    free(entry->data.content_buff);
                    entry->data.content_buff = NULL;
                    entry->data.content_size = 0;

                    char key[256];
                    strncpy(key, entry->key, entry->key_len);
                    key[entry->key_len] = '\0';
                    console_log(LOG_DBG, "\t", "Cache invalidated for ", key);
                }
            }
        }
    }
}

