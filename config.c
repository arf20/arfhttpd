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

    config.c: Config parser

*/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>


/* Config */
const char *webroot = NULL;


int
isnotspace(char c) {
    int t = (c != ' ') && (c != '\t');
    return t;
}

const char *
findalnum(const char *str) {
    if (!str) return NULL;
    while (*str && !isnotspace(*str)) str++;
    return str;
}

const char *
find_value(const char *key) {
    const char *v = strchr(key, ' ');
    return findalnum(v);
}

int
substrchk(const char *str, const char *substr) {
    return strncmp(str, substr, strlen(substr)) == 0;
}

char *
stralloccpy(const char *start, size_t length) {
    char *str = malloc(length);
    strncpy(str, start, length);
}

void
config_parse(const char *config) {
    const char *key, *value, *value_end;
    size_t value_length;
    while (*config) {
        key = findalnum(config);
        value = find_value(key);
        value_end = strchr(value, '\n');
        value_length = value_end - value;

        if (substrchk(key, "webroot ")) {
            webroot = stralloccpy(value, value_length);
        }

        config = value_end + 1;
    }
}
