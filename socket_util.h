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

#ifndef _SOCKET_UTIL_H
#define _SOCKET_UTIL_H

#include <netdb.h>

#include "config.h"

fd_thread_node_t *fd_thread_list_push(fd_thread_node_t **head, int fd, pthread_t thread);
int host_resolve(const char *host, struct addrinfo **addrs);
int ai_addr_str(const struct addrinfo *addr, char *str, size_t strlen, int flags);
int sa_addr_str(const struct sockaddr *addr, char *str, size_t strlen);

#endif
