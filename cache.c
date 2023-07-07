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
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <pthread.h>

#include <limits.h>
#include <string.h>
#include <stdlib.h>


static char resolved_path[PATH_MAX];

static hashtable_t file_cache;

static int infd = 0;


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

const char *
cached_open(const char *filename, size_t *size) {
    if (!filename) return NULL;
    htdata_t *cache_entry = hashtable_get(&file_cache, filename);
    if (cache_entry && IS_CACHED_CONTENT(cache_entry->flags)) {
        /* Cache hit */
        console_log(LOG_DBG, "\t", "Content cache hit ", filename);
        *size = cache_entry->content_size;
        return cache_entry->content_buff;
    } else {
        /* Cache miss - mmap file */
        struct stat sb;
        int fd = open(filename, O_RDONLY);
        if (fd < 0)
            return NULL;

        if (cache_entry && IS_CACHED_STAT(cache_entry->flags))
            sb = cache_entry->stat_data;
        else
            cached_stat(filename, &sb);

        /* cached_stat creates entry, get it */
        cache_entry = hashtable_get(&file_cache, filename);

        size_t _size = sb.st_size;

        char *ptr = mmap(NULL, _size, PROT_READ,
            MAP_PRIVATE, fd, 0);

        close(fd);

        /* it should by all means exist but just in case */
        if (cache_entry) {
            cache_entry->content_buff = ptr;
            cache_entry->content_size = _size;
            SET_CACHED_CONTENT(cache_entry->flags);
        } else {
            /* Insert hash table */
            htdata_t new_cache_entry = { 0 };
            new_cache_entry.content_buff = ptr;
            new_cache_entry.content_size = _size;
            /* Add inotify watch */
            new_cache_entry.wd = inotify_add_watch(infd, filename, IN_MODIFY);
            if (new_cache_entry.wd < 0)
                console_log(LOG_ERR, "\t", "Cannot watch ", filename);
            else console_log(LOG_DBG, "\t", "Watching ", filename);
            hashtable_node_t *new_node = 
                hashtable_insert(&file_cache, filename, new_cache_entry);
            SET_CACHED_CONTENT(cache_entry->flags);
        }
        console_log(LOG_DBG, "\t", "Content cache miss ", filename);
        *size = _size;
        return cache_entry->content_buff;
    }
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
                    munmap(entry->data.content_buff, entry->data.content_size);
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

