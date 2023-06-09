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

#include <pthread.h>

#ifndef _CONFIG_H
#define _CONFIG_H

/* Macros */
#define BUFF_SIZE  65535

/* Types */
typedef struct string_node_s {
    const char *str;
    struct string_node_s *prev;
    struct string_node_s *next;
} string_node_t;

typedef struct fd_thread_node_s {
    int fd;
    pthread_t thread;
    struct fd_thread_node_s *prev;
    struct fd_thread_node_s *next;
} fd_thread_node_t;


/* Webserver config types */
typedef enum {
    CONFIG_ROOT,      /* Real path root */
    CONFIG_HEADER,    /* Defines header */
    CONFIG_MIMEHEADER,      /* Enables MIME type header */
    CONFIG_INDEX,     /* Default index file */
    CONFIG_AUTOINDEX  /* Enable autoindex */
} config_type_t;

extern const char *config_type_strs[];

typedef struct config_node_s {
    config_type_t type;
    const char *param1;
    const char *param2;
    struct config_node_s *prev;
    struct config_node_s *next;
} config_node_t;

typedef struct location_node_s {
    const char *location;
    config_node_t *config;
    struct location_node_s *prev;
    struct location_node_s *next;
} location_node_t;

/* Config */
extern string_node_t *listen_list, *tls_listen_list;
extern location_node_t *location_list;
extern const char *cert_file, *cert_key_file;

int config_parse(const char *config);

#endif

