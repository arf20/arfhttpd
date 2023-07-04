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

    httpd.c: Program entry point

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <pthread.h>

#include "config.h"
#include "socket.h"
#include "tls_socket.h"
#include "cache.h"
#include "log.h"

/* Text file (no '\0's) */
int
file_read(const char *path, char **buff) {
    FILE *f = fopen(path, "r");
    if (f == NULL) return -1;
    fseek(f, 0L, SEEK_END);
    size_t s = ftell(f);
    fseek(f, 0L, SEEK_SET);
    *buff = malloc(s + 1);
    fread(*buff, 1, s, f);
    (*buff)[s] = '\0';
    return s;
}



int
main(int argc, char **argv) {
    printf("arfhttpd GPLv3+\n");

    char *config = NULL;
    if (file_read("../arfhttpd.conf", &config) < 0) {
        printf("Error reading config file: %s\n", strerror(errno));
        exit(1);
    }

    config_parse(config);

    /* Print config */
    string_node_t *listen_current = listen_list;
    while (listen_current) {
        printf("listen %s\n", listen_current->str);
        listen_current = listen_current->next;
    }

    listen_current = tls_listen_list;
    while (listen_current) {
        printf("listen %s tls\n", listen_current->str);
        listen_current = listen_current->next;
    }

    location_node_t *location_current = location_list;
    while (location_current) {
        printf("location %s\n", location_current->location);
        
        config_node_t *config_current = location_current->config;
        while (config_current) {
            printf("\t%s ", config_type_strs[config_current->type]);
            if (config_current->param1) printf("%s ", config_current->param1);
            if (config_current->param2) printf("%s", config_current->param2);
            printf("\n");
            config_current = config_current->next;
        }

        location_current = location_current->next;
    }

    if (cache_init() < 0) {
        exit(1);
    }

    /* Start accept threads */
    server_start(listen_list);
    tls_server_start(tls_listen_list);

    console_log(LOG_INFO, "\t", "Server started", NULL);

    /* Join accept threads (wait for them to exit) */
    if (!listen_socket_list) {
        console_log(LOG_ERR, "\t", "Nothing to accept", NULL);
        exit(1);
    }
    fd_thread_node_t *listen_socket_current = listen_socket_list;
    while (listen_socket_current) {
        if (pthread_join(listen_socket_current->thread, NULL) != 0) {
            console_log(LOG_ERR, "\t", "Error joining accept thread", NULL);
            exit(1);
        }
        listen_socket_current = listen_socket_current->next;
    }

    if (!tls_listen_socket_list) {
        console_log(LOG_ERR, "\t", "Nothing to accept", NULL);
        exit(1);
    }
    listen_socket_current = tls_listen_socket_list;
    while (listen_socket_current) {
        if (pthread_join(listen_socket_current->thread, NULL) != 0) {
            console_log(LOG_ERR, "\t", "Error joining accept thread", NULL);
            exit(1);
        }
        listen_socket_current = listen_socket_current->next;
    }


    return 0;
}

