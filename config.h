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

*/

#include <pthread.h>

#ifndef _CONFIG_H
#define _CONFIG_H

/* Defines */
typedef struct string_node_s {
    const char *str;
    struct string_node_s *prev;
    struct string_node_s *next;
} string_node_t;

typedef struct int_node_s {
    int v;
    pthread_t thread;
    struct int_node_s *prev;
    struct int_node_s *next;
} fd_thread_node_t;

/* Config */
extern const char *webroot;
extern string_node_t *listen_list;

string_node_t *string_list_new(string_node_t *prev);
fd_thread_node_t *int_list_new(fd_thread_node_t *prev);

void config_parse(const char *config);

#endif

