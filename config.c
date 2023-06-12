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

#include "strutils.h"
#include "config.h"

const char *config_type_strs[] = {
    "webroot",
    "header",
    "mimeheader",
    "index",
    "autoindex"
};

/* Config */
string_node_t *listen_list = NULL;
location_node_t *location_list = NULL, *location_current = NULL;


string_node_t *
string_list_push(string_node_t **head, const char *str, size_t len) {
    if (!head) return NULL;
    string_node_t *end = NULL;
    string_node_t *new = malloc(sizeof(string_node_t));
    if (*head) {
        end = *head;
        while (end->next) end = end->next;
        end->next = new;
        new->prev = *head;
    } else {
        *head = new;
    }
    new->next = NULL;
    new->str = stralloccpy(str, len);
    return new;
}

location_node_t *
location_list_push(location_node_t **head, const char *loc, size_t len) {
    if (!head) return NULL;
    location_node_t *end = NULL;
    location_node_t *new = malloc(sizeof(location_node_t));
    if (*head) {
        end = *head;
        while (end->next) end = end->next;
        end->next = new;
        new->prev = *head;
    } else {
        *head = new;
    }
    new->next = NULL;
    new->location = stralloccpy(loc, len);
    new->config = NULL;
    return new;
}

location_node_t *
location_list_find(location_node_t *head, const char *loc) {
    if (!head) return NULL;
    location_node_t *ptr = head;
    while (ptr) {
        if (strcmp(ptr->location, loc) == 0) return ptr;
        ptr = ptr->next;
    }
    return NULL;
}

config_node_t *
config_list_push(config_node_t **head, config_type_t type,
    const char *p1, const char *p2)
{
    if (!head) return NULL;
    config_node_t *end = NULL;
    config_node_t *new = malloc(sizeof(config_node_t));
    if (*head) {
        end = *head;
        while (end->next) end = end->next;
        end->next = new;
        new->prev = *head;
    } else {
        *head = new;
    }
    new->next = NULL;
    new->type = type;
    if (p1)
        new->param1 = stralloccpy(p1, strlen(p1));
    if (p2)
        new->param2 = stralloccpy(p2, strlen(p2));
    return new;
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
    size_t n = 0;
    const char *nl = strchr(key, '\n');
    if (nl) n = nl - key; else n = strlen(key);
    const char *v = strnchr(key, n, ' ');
    if (!v) return NULL;
    return findalnum(v);
}

int
substrchk(const char *str, const char *substr) {
    return strncmp(str, substr, strlen(substr)) == 0;
}



int
config_parse(const char *config) {
    size_t config_length = strlen(config);
    string_node_t *listen_list_current = NULL, *listen_list_prev = NULL;

    int line = 0;
    const char *ptr = config, *key, *value, *value_end;
    size_t value_length;
    char p1[1024];
    char p2[1024];
    int argc = 0;

    while (ptr && *ptr && ptr < ptr + config_length) {
        key = findalnum(ptr);
        if (!key) { /* Ignore empty lines */
            ptr = strchr(ptr, '\n');
            continue;
        }
        value = find_value(key);
        if (value) 
            value_end = strchr(value, '\n');
        else
            value_end = strchr(key, '\n');
        value_length = value_end - value;

        /* Extract arguments */
        argc = 0;
        if (value) {
            argc++;
            const char* separator = strnchr(value, value_length, ' ');
            if (separator) {
                size_t p1len = separator - value;
                strncpy(p1, value, p1len);
                p1[p1len] = '\0';
                size_t p2len = value_end - (separator + 1);
                strncpy(p2, separator + 1, p2len);
                p2[p2len] = '\0';
                argc++;
            } else {
                strncpy(p1, value, value_length);
                p1[value_length] = '\0';
            }
        }


        /* Valid keys */
        /* Context-less */
        if (substrchk(key, "listen ")) { /* address/port */
            if (argc != 1) {
                printf("Error: Wrong amount of arguments, line %d\n", line);
                continue;
            }
            string_list_push(&listen_list, value, value_length);
        }
        else if (substrchk(key, "location ")) {
            if (argc != 1) {
                printf("Error: Wrong amount of arguments, line %d\n", line);
                continue;
            }

            if (location_list_find(location_list, p1)) {
                printf("Warning: Duplicated location, line %d\n", line);
            } else {
                location_current = location_list_push(&location_list,
                    p1, strlen(p1));
            }
        }
        /* Location context */
        else if (substrchk(key, "webroot ")) { /* Root of location in fs */
            if (argc != 1) {
                printf("Error: Wrong amount of arguments, line %d\n", line);
                continue;
            }
            config_list_push(&location_current->config, CONFIG_ROOT, p1, NULL);
        }
        else if (substrchk(key, "index ")) { /* Default index file */
            if (argc != 1) {
                printf("Error: Wrong amount of arguments, line %d\n", line);
                continue;
            }
            config_list_push(&location_current->config, CONFIG_INDEX, p1, NULL);
        }
        else if (substrchk(key, "autoindex")) { /* No parameters, enable */
            config_list_push(&location_current->config, CONFIG_AUTOINDEX,
                NULL, NULL);
        }
        else if (substrchk(key, "mimeheader")) { /* No parameters, enable */
            config_list_push(&location_current->config, CONFIG_MIMEHEADER,
                NULL, NULL);
        }
        else if (substrchk(key, "header ")) { /* Header and value */
            if (argc != 2) {
                printf("Error: Wrong amount of arguments, line %d\n", line);
                continue;
            }
            
            config_list_push(&location_current->config, CONFIG_HEADER,
                p1, p2);
        }
        
        else {
            printf("Warning: Invalid config key, line %d\n", line);
        }

        ptr = value_end + 1;
        line++;
    }

    /* Check config */
    if (!location_list) printf("Error: No location\n");

    location_node_t *location_current = location_list;
    while (location_current) {    
        int haspoint = 0;    
        config_node_t *config_current = location_current->config;
        while (config_current) {
            if (config_current->type == CONFIG_ROOT) haspoint = 1;
            config_current = config_current->next;
        }
        if (!haspoint) printf("Error: No point in location %s\n",
            location_current->location);

        location_current = location_current->next;
    }

    return 0;
}
