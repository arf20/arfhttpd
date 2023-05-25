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

#include "config.h"

#include "socket.h"

size_t /* from BSD */
strlcat(char *restrict dst, const char *restrict src, size_t dstsize) {
    int d_len, s_len, offset, src_index;

    /* obtain initial sizes */
    d_len = strlen(dst);
    s_len = strlen(src);

    /* get the end of dst */
    offset = d_len;

    /* append src */
    src_index = 0;
    while(*(src+src_index) != '\0')
    {
        *(dst + offset) = *(src + src_index);
        offset++;
        src_index++;
        /* don't copy more than dstsize characters
           minus one */
        if(offset == dstsize - 1)
            break;
    }
    /* always cap the string! */
    *(dst + offset) = '\0';

    return d_len + s_len;
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

void
strsub(char *dest, size_t destsize, const char *src, size_t n) {
    int i = 0;
    for (i = 0; i < n && i < destsize && src[i]; i++)
        dest[i] = src[i];
    dest[i] = '\0';
}

int
server_start(string_node_t *listen_list) {
    string_node_t *listen_list_current = listen_list;
    char addrstr[128];
    char portstr[8];
    struct addrinfo *ai;
    int port = 0;

    while (listen_list_current) {
        char *colon = strchr(listen_list_current->str, '/');
        if (colon) {
            /* get address */
            strsub(addrstr, 128, listen_list_current->str,
                colon - listen_list_current->str);
            /* get port */
            strsub(portstr, 8, colon + 1, 8);
            port = atoi(portstr);

            if (host_resolve(addrstr, &ai) < 0) {
                printf("Error resolving listen address %s: %s\n",
                    addrstr, strerror(errno));
            }

            ai_addr_str(ai, addrstr, 256, 1);

            printf("listening %s:%d\n", addrstr, port);
            socket_listen(ai, port);
        } else { /* assume port */
            port = atoi(listen_list_current->str);

            host_resolve("0.0.0.0", &ai);
            ai_addr_str(ai, addrstr, 256, 1);
            printf("listening %s:%d\n", addrstr, port);
            socket_listen(ai, port);

            host_resolve("::", &ai);
            ai_addr_str(ai, addrstr, 256, 1);
            printf("listening %s:%d\n", addrstr, port);
            socket_listen(ai, port);
        }
        

        listen_list_current = listen_list_current->next;
    }
}
