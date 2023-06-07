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

    http.c: HTTP implementation

*/

#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>

#include "config.h"
#include "strutils.h"
#include "log.h"

#include "http.h"


char sendbuff[BUFF_SIZE];
char logbuff[1024];
char endpoint[1024];

int
strlencrlf(const char *str) {
    const char *ptr = str;
    int size = 0;
    while (*ptr != '\0') {
        if (*ptr == '\n') size++;
        size++;
        ptr++;
    }
    return size;
}

void
convertcrlf(char *buff, size_t size) {
    size_t bufflen = strlen(buff) + 1;
    char *ptr = buff;

    while (*ptr) {
        char *p = strchr(ptr, '\n');
        /* walked past last occurrence of needle or run out of buffer */
        if (p == NULL || bufflen + 1 >= size || p >= buff + size) {
            break;
        }

        /* move part after lf */
        memmove(p + 1, p, bufflen - (p - buff));

        /* insert cr */
        *p = '\r';

        bufflen++;
        ptr = p + 2;
    }
}


const char *
find_field(const char *str) {
    const char *field = strchr(str, ' ');
    if (!field) return NULL;
    return field + 1;
}


void
send404(const client_t *cs) {
    snprintf(sendbuff, BUFF_SIZE, "HTTP/1.1 404 Not Found\n\n");
    convertcrlf(sendbuff, BUFF_SIZE);
    if (send(cs->fd, sendbuff, strlen(sendbuff), 0) < 0) {
        console_log(LOG_ERR, cs->addrstr, "Error sending", NULL);
    }
}


location_node_t *
location_find(location_node_t **head, const char *endpoint) {
    if (head) return NULL;
    location_node_t *location_current = location_list, *location_best = NULL;
    int bestn = 0;
    while (location_current) {
        int i = 0, n = 0;
        while (endpoint[i] && location_current->location[i]) {
            if (endpoint[i] == location_current->location[i]) n++;
            i++;
        }

        if (n > bestn) {
            location_best = location_current;
            bestn = n;
        }

        location_current = location_current->next;
    }
    return location_best;
}

const char *
config_find_root(config_node_t *head) {
    while (head) {
        if (head->type == CONFIG_ROOT);
        return head->param1;
        head = head->next;
    }
    return NULL;
}

void
http_process(const client_t *cs, const char *buff, size_t len) {
    const char *endpoint_ptr = find_field(buff);
    if (!endpoint_ptr) {
        console_log(LOG_ERR, cs->addrstr, "Missing endpoint", NULL);
        return;
    }

    const char *http_version = find_field(endpoint_ptr);
    if (!http_version) {
        console_log(LOG_ERR, cs->addrstr, "Missing version", NULL);
        return;
    }

    strncpy(endpoint, endpoint_ptr, http_version - endpoint_ptr - 1);
    endpoint[http_version - endpoint_ptr - 1] = '\0';

    strncpy(logbuff, buff, http_version - buff - 1);
    logbuff[http_version - buff - 1] = '\0';

    /* Handle methods */
    if (strncmp(buff, "GET", 3) == 0) {
        location_node_t *location = location_find(&location_list, endpoint);
        if (!location) send404(cs);

        const char *webroot = config_find_root(location->config);

        char path[512];
        snprintf(path, 512, "%s%s", webroot, endpoint);

        strlcat(logbuff, " -> ", 1024);
        strlcat(logbuff, path, 1024);

        FILE *file = fopen(path, "rb");
        if (file) {
            snprintf(sendbuff, BUFF_SIZE, "HTTP/1.1 200 OK\n\n");
            convertcrlf(sendbuff, BUFF_SIZE);
            int headerend = strlen(sendbuff);
            fread(sendbuff + headerend, 1, BUFF_SIZE - headerend, file);
            send(cs->fd, sendbuff, strlen(sendbuff), 0);
        } else {
            send404(cs);
        }
    } else {
        snprintf(sendbuff, BUFF_SIZE, "HTTP/1.1 501 Not Implemented\n\n");
        convertcrlf(sendbuff, BUFF_SIZE);
        send(cs->fd, sendbuff, strlen(sendbuff), 0);
    }

    close(cs->fd);

    console_log(LOG_INFO, cs->addrstr, logbuff, NULL);
}
