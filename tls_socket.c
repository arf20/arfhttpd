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
tls_server_start(string_node_t *tls_listen_list) {
    
}
