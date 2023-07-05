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

#include <stdlib.h>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>

#include "strutils.h"

#include "socket_util.h"

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
