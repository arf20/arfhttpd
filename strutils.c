/*

    arfhttpd: Cross-plataform multi-frontend game
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

    strutils.c: string utilities

*/

#include "strutils.h"

#include <string.h>

size_t /* from BSD */
strlcat(char *dst, const char *src, size_t dstsize) {
    int d_len, s_len, offset, src_index;

    /* obtain initial sizes */
    d_len = strlen(dst);
    s_len = strlen(src);

    /* get the end of dst */
    offset = d_len;

    /* append src */
    src_index = 0;
    while(*(src+src_index) != '\0')
    {
        *(dst + offset) = *(src + src_index);
        offset++;
        src_index++;
        /* don't copy more than dstsize characters
           minus one */
        if(offset == dstsize - 1)
            break;
    }
    /* always cap the string! */
    *(dst + offset) = '\0';

    return d_len + s_len;
}

void
strsub(char *dest, size_t destsize, const char *src, size_t n) {
    int i = 0;
    for (i = 0; i < n && i < destsize && src[i]; i++)
        dest[i] = src[i];
    dest[i] = '\0';
}

char *
stralloccpy(const char *start, size_t length) {
    if (!start) return NULL;
    char *str = malloc(length);
    strncpy(str, start, length);
}
