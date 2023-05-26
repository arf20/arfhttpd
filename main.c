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

    httpd.c: Program entry point

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <pthread.h>

#include "config.h"
#include "socket.h"

int
file_read(const char *path, char **buff) {
    FILE *f = fopen(path, "r");
    if (f == NULL) return -1;
    fseek(f, 0L, SEEK_END);
    size_t s = ftell(f);
    fseek(f, 0L, SEEK_SET);
    *buff = malloc(s);
    fread(*buff, 1, s, f);
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

    printf("webroot %s\n", webroot);

    /* Start accept threads */
    server_start(listen_list);

    /* Join accept threads (wait for them to exit) */
    if (!listen_socket_list) {
        printf("Nothing to accept\n");
        exit(1);
    }
    fd_thread_node_t *listen_socket_list_current = listen_socket_list;
    while (listen_socket_list_current) {
        if (pthread_join(listen_socket_list_current->thread, NULL) != 0) {
            printf("Error joining accept thread\n");
            exit(1);
        }
        listen_socket_list_current = listen_socket_list_current->next;
    }


    return 0;
}

