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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "config.h"

/* Config */
const char *webroot = NULL;
string_node_t *listen_list = NULL;
location_node_t *location_list = NULL;

string_node_t *
string_list_new(string_node_t *prev) {
    string_node_t *list = malloc(sizeof(string_node_t));
    list->prev = prev;
    list->next = NULL;
}

fd_thread_node_t *
fd_thread_list_new(fd_thread_node_t *prev) {
    fd_thread_node_t *list = malloc(sizeof(fd_thread_node_t));
    list->prev = prev;
    list->next = NULL;
}

location_node_t *
location_list_new(location_node_t *prev) {
    location_node_t *list = malloc(sizeof(location_node_t));
    list->prev = prev;
    list->next = NULL;
}

config_node_t *
config_list_new(config_node_t *prev) {
    config_node_t *list = malloc(sizeof(config_node_t));
    list->prev = prev;
    list->next = NULL;
}

/* str utils */
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
    size_t config_length = strlen(config);
    string_node_t *listen_list_current = NULL, *listen_list_prev = NULL;

    int line = 0;
    const char *ptr = config, *key, *value, *value_end;
    size_t value_length;
    while (ptr && *ptr && ptr < ptr + config_length) {
        key = findalnum(ptr);
        if (!key) { /* Ignore empty lines */
            ptr = strchr(ptr, '\n');
            continue;
        }
        value = find_value(key);
        if (!value) { /* Ignore missing values */
            ptr = strchr(ptr, '\n');
            continue;
        }
        value_end = strchr(value, '\n');
        value_length = value_end - value;

        /* Valid keys */
        if (substrchk(key, "webroot ")) {
            webroot = stralloccpy(value, value_length);
        } else if (substrchk(key, "listen ")) {
            listen_list_current = string_list_new(listen_list_prev);
            if (!listen_list) listen_list = listen_list_current;
            if (listen_list_prev) listen_list_prev->next = listen_list_current;
            listen_list_current->str = stralloccpy(value, value_length);
            listen_list_prev = listen_list_current;
        } else if (substrchk(key, "location ")) {
            
        } else {
            printf("Invalid config key, line %d\n", line);
        }

        ptr = value_end + 1;
        line++;
    }
}
