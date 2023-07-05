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
    /* Set port */
    if (addr->ai_family == AF_INET)
        ((struct sockaddr_in*)addr->ai_addr)->sin_port = htons(port);
    else if (addr->ai_family == AF_INET6)
        ((struct sockaddr_in6*)addr->ai_addr)->sin6_port = htons(port);

    int fd = -1;
    /* Create socket */
    if ((fd = socket(addr->ai_family, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        printf("Error creating socket: %s\n", strerror(errno));
        return -1;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        printf("Error setting option SO_REUSEADDR socket: %s\n", strerror(errno));
        return -1;
    }

    /* Bind socket */
    if (bind(fd, addr->ai_addr, addr->ai_addrlen) < 0) {
        printf("Error binding socket: %s\n", strerror(errno));
        return -1;
    }

    /* Listen on socket */
    if (listen(fd, SOMAXCONN) < 0) {
        printf("Error listening socket: %s\n", strerror(errno));
        return -1;
    }

    return fd;
}

void *
tls_receive_loop(void *ptr) {
    client_t *cs = (client_t*)ptr;
    struct tls *ctx = cs->ctx;
    const char *addrstr = cs->addrstr;
    char recvbuff[BUFF_SIZE];

    while (1) {
        int recvlen = tls_read(ctx, recvbuff, BUFF_SIZE);
        if (recvlen < 0) {
            console_log(LOG_ERR, addrstr, "Error reading TLS client: ",
                strerror(errno));
            return NULL;
        } else if (recvlen == 0) {
            console_log(LOG_DBG, addrstr, "Client disconnected", NULL);
            return NULL;
        } else {
            http_process(cs, recvbuff, recvlen);
            return NULL;
        }
    }
}

void *
tls_accept_loop(void *ptr) {
    int lfd = *(int*)ptr;
    int cfd = -1;
    struct sockaddr sa;
    socklen_t salen = sizeof(struct sockaddr);
    char lfdstr[16];
    struct tls *cctx = NULL; /* client tls context */

    snprintf(lfdstr, 16, "%d", lfd);

    while (1) {
        cfd = accept(lfd, &sa, &salen);
        if (cfd < 0) {
            console_log(LOG_ERR, lfdstr, "Accepting client: ", strerror(errno));
            return NULL;
        }

        client_t *cs = malloc(sizeof(client_t));
        cs->addrstr = malloc(128);
        cs->fd = cfd;
        sa_addr_str(&sa, cs->addrstr, 128);

        /* TLS accept */
        if (tls_accept_socket(sctx, &cctx, cfd) != 0) {
            console_log(LOG_ERR, lfdstr, "Accepting TLS client: ",
                strerror(errno));
            return NULL;
        }

        cs->ctx = cctx;

        /*const char *tls_ver = tls_conn_version(cctx);*/
        const char *tls_cipher = tls_conn_cipher(cctx);

        console_log(LOG_DBG, cs->addrstr, "Accepted TLS client", tls_cipher);

        /* Create thread for every incoming connection */
        pthread_t recv_thread;
        pthread_create(&recv_thread, NULL, tls_receive_loop, cs);
        pthread_detach(recv_thread);
    }
}


int
tls_socket_listen_accept(struct addrinfo *ai, unsigned short port) {
    /* listen */
    int lfd = tls_socket_listen(ai, port);
    if (lfd < 0) return -1;

    /* push element */
    fd_thread_node_t* node = fd_thread_list_push(&tls_listen_socket_list, lfd, 0);

    /* run accept thread */
    pthread_t accept_thread;
    pthread_create(&accept_thread, NULL, tls_accept_loop, &node->fd);

    node->thread = accept_thread;

    return lfd;
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
