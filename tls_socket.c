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

    socket.c: TCP socket server

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <tls.h>

#include <pthread.h>

#include "config.h"
#include "strutils.h"
#include "log.h"
#include "http.h"
#include "socket_util.h"

#include "tls_socket.h"

/* Vars */
fd_thread_node_t *tls_listen_socket_list = NULL;

static struct tls *sctx = NULL; /* server tls context */

int
tls_socket_listen(struct addrinfo *addr, unsigned short port) {
    
}

void *
tls_receive_loop(void *ptr) {
    
}

void *
tls_accept_loop(void *ptr) {
    
}

int
tls_socket_listen_accept(struct addrinfo *ai, unsigned short port) {
    
}

int
tls_server_start(string_node_t *tls_listen_list, const char *cert_file,
    const char *cert_key_file)
{
    string_node_t *listen_current = tls_listen_list;
    char addrstr[128];
    char portstr[8];
    struct addrinfo *ai;
    int port = 0;

    /* Initialize LibreSSL libtls */
    struct tls_config *cfg = NULL;
    char *mem;
    size_t mem_len;
    
    if (tls_init() != 0) {
        printf("Error initializing libtls: tls_init\n");
        return -1;
    }

    /* Create config */
    if ((cfg = tls_config_new()) == NULL) {
        printf("Error creatitng config: tls_config_new\n");
        return -1;
    }

    /* Certificate */
    if ((mem = tls_load_file(cert_file, &mem_len, NULL)) == NULL) {
        printf("Error reading cert: tls_load_file\n");
        return -1;
    }
	if (tls_config_set_cert_mem(cfg, mem, mem_len) != 0) {
        printf("Error assigning cert: tls_config_set_cert_mem\n");
        return -1;
    }
    /* Certificate key */
    if ((mem = tls_load_file(cert_key_file, &mem_len, NULL)) == NULL) {
        printf("Error reading cert key: tls_load_file\n");
        return -1;
    }
	if (tls_config_set_key_mem(cfg, mem, mem_len) != 0) {
        printf("Error assigning cert key: tls_config_set_key_mem\n");
        return -1;
    }

    /* Create server context */
    if ((sctx = tls_server()) == NULL) {
        printf("Error creating server context: tls_server\n");
        return -1;
    }
    /* Assign config to context */
    if (tls_configure(sctx, cfg) != 0) {
        printf("Error applying config to ctx: tls_configure\n");
        return -1;
    }


    /* Standard TCP listen on configured endpoints */
    while (listen_current) {
        char *colon = strchr(listen_current->str, '/');
        if (colon) {
            /* get address */
            strsub(addrstr, 128, listen_current->str,
                colon - listen_current->str);
            /* get port */
            strsub(portstr, 8, colon + 1, 8);
            port = atoi(portstr);

            if (host_resolve(addrstr, &ai) < 0) {
                printf("Error resolving listen address %s: %s\n",
                    addrstr, strerror(errno));
            }

            ai_addr_str(ai, addrstr, 256, 1);

            int lfd = tls_socket_listen_accept(ai, port);
            printf("listening %s:%d %d\n", addrstr, port, lfd);
        } else { /* assume port */
            port = atoi(listen_current->str);

            if (port == 0) printf("Error invalid port or wrong separator (/)\n");

            host_resolve("0.0.0.0", &ai);
            ai_addr_str(ai, addrstr, 256, 1);
            int lfd = tls_socket_listen_accept(ai, port);

            printf("listening %s:%d %d\n", addrstr, port, lfd);
        }

        listen_current = listen_current->next;
    }
}
