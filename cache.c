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

static hashtable_t file_cache;

void
cache_init() {
    hashtable_new(&file_cache, CACHE_SIZE);
}

int
cached_stat(const char *file, struct stat *buf) {
    nodedata_t *cached_entry = hashtable_get(&file_cache, file);
    if (!cached_entry) {
        /* Cache miss */
        int r = stat(file, buf);
        if (r == 0) {
            nodedata_t new_entry;
            new_entry.stat_data = *buf;
            hashtable_insert(&file_cache, file, new_entry);
            console_log(LOG_DBG, "\t", "Cached stat for ", file);
        }
        return r;
    } else {
        /* Cache hit - assume entry has stat */
        *buf = cached_entry->stat_data;
        console_log(LOG_DBG, "\t", "Cache stat hit for ", file);
        return 0;
    }
}

FILE *
cached_fopen(const char *filename, const char *modes) {

}

size_t
cached_fread(void *ptr, size_t size, size_t n, FILE *stream) {

}

int
cached_fclose(FILE *stream) {

}
