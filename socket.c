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

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>

#include <pthread.h>

#include "config.h"
#include "strutils.h"
#include "log.h"
#include "http.h"

#include "socket.h"

/* Vars */
fd_thread_node_t *listen_socket_list = NULL;

fd_thread_node_t *
fd_thread_list_push(fd_thread_node_t **head, int fd, pthread_t thread) {
    if (!head) return NULL;
    fd_thread_node_t *end = NULL;
    fd_thread_node_t *new = malloc(sizeof(fd_thread_node_t));
    if (*head) {
        end = *head;
        while (end->next) end = end->next;
        end->next = new;
        new->prev = *head;
    } else {
        *head = new;
    }
    new->next = NULL;
    new->fd = fd;
    new->thread = thread;
    return new;
}

int
host_resolve(const char *host, struct addrinfo **addrs) {
    struct addrinfo hints = { 0 };

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_CANONNAME;

    return getaddrinfo(host, NULL, &hints, addrs);

    return 0;
}

int
ai_addr_str(const struct addrinfo *addr, char *str, size_t strlen,
    int flags) 
{
    void *ptr;
    if (addr->ai_family == AF_INET)
        ptr = &((struct sockaddr_in*)addr->ai_addr)->sin_addr;
    else if (addr->ai_family == AF_INET6)
        ptr = &((struct sockaddr_in6*)addr->ai_addr)->sin6_addr;
    
    int r = (inet_ntop(addr->ai_family, ptr, str, strlen) != NULL)
        ? 0 : -1;
    if (flags && (r != -1) && addr->ai_canonname) {
        strlcat(str, " (", strlen);
        strlcat(str, addr->ai_canonname, strlen);
        strlcat(str, ")", strlen);
    }

    return r;
}

int
sa_addr_str(const struct sockaddr *addr, char *str, size_t strlen) {
    void *ptr;
    if (addr->sa_family == AF_INET)
        ptr = &((struct sockaddr_in*)addr)->sin_addr;
    else if (addr->sa_family == AF_INET6)
        ptr = &((struct sockaddr_in6*)addr)->sin6_addr;
    else return -1;
    
    int r = (inet_ntop(addr->sa_family, ptr, str, strlen) != NULL)
        ? 0 : -1;

    return r;
}

int
socket_listen(struct addrinfo *addr, unsigned short port) {
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
receive_loop(void *ptr) {
    client_t *cs = (client_t*)ptr;
    int cfd = cs->fd;
    const char *addrstr = cs->addrstr;
    char recvbuff[BUFF_SIZE];

    while (1) {
        int recvlen = read(cfd, recvbuff, BUFF_SIZE);
        if (recvlen < 0) {
            console_log(LOG_ERR, addrstr, "Error reading client: ",
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
accept_loop(void *ptr) {
    int lfd = *(int*)ptr;
    int cfd = -1;
    struct sockaddr sa;
    socklen_t salen = sizeof(struct sockaddr);
    char lfdstr[16];
    snprintf(lfdstr, 16, "%d", lfd);

    while (1) {
        cfd = accept(lfd, &sa, &salen);
        if (cfd < 0) {
            console_log(LOG_ERR, lfdstr, "Accepting client: ", strerror(errno));
            return NULL;
        }

        char *addrstr = malloc(128);
        client_t *cs = malloc(sizeof(client_t));
        cs->addrstr = addrstr;
        cs->fd = cfd;

        sa_addr_str(&sa, addrstr, 128);
        console_log(LOG_DBG, addrstr, "Accepted client", NULL);

        /* Create thread for every incoming connection */
        pthread_t recv_thread;
        pthread_create(&recv_thread, NULL, receive_loop, cs);
        pthread_detach(recv_thread);
    }
}

int
socket_listen_accept(struct addrinfo *ai, unsigned short port) {
    /* listen */
    int lfd = socket_listen(ai, port);
    if (lfd < 0) return -1;

    /* push element */
    fd_thread_node_t* node = fd_thread_list_push(&listen_socket_list, lfd, 0);

    /* run accept thread */
    pthread_t accept_thread;
    pthread_create(&accept_thread, NULL, accept_loop, &node->fd);

    node->thread = accept_thread;

    return lfd;
}

int
server_start(string_node_t *listen_list) {
    string_node_t *listen_current = listen_list;
    char addrstr[128];
    char portstr[8];
    struct addrinfo *ai;
    int port = 0;

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

            int lfd = socket_listen_accept(ai, port);
            printf("listening %s:%d %d\n", addrstr, port, lfd);
        } else { /* assume port */
            port = atoi(listen_current->str);

            if (port == 0) printf("Error invalid port or wrong separator (/)\n");

            host_resolve("0.0.0.0", &ai);
            ai_addr_str(ai, addrstr, 256, 1);
            int lfd = socket_listen_accept(ai, port);

            printf("listening %s:%d %d\n", addrstr, port, lfd);
        }

        listen_current = listen_current->next;
    }
}
