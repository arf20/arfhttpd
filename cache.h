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

#ifndef _CACHE_H
#define _CACHE_H

#include <stdio.h>
#include <sys/stat.h>

#define CACHE_SIZE  65536

/* Our own FILE type */
typedef struct CACHED_FILE_s {
    size_t _offset;
    FILE *actual_stream;
} CACHED_FILE;

void cache_init();
int cached_stat(const char *file, struct stat *buf);
CACHED_FILE *cached_fopen(const char *filename, const char *modes);
size_t cached_fread(void *ptr, size_t size, size_t n, CACHED_FILE *stream);
int cached_fclose(CACHED_FILE *stream);

#endif
