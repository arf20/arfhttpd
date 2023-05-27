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
#include "log.h"

#include "http.h"


char sendbuff[BUFF_SIZE];
char logbuff[1024];

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
http_process(const client_t *cs, const char *buff, size_t len) {
    fwrite(buff, len, 1, stdout);

    const char *http_endpoint = find_field(buff);
    if (!http_endpoint) {
        console_log(LOG_ERR, cs->addrstr,"Missing endpoint", NULL);
        return;
    }

    const char *http_version = find_field(http_endpoint);
    if (!http_version) {
        console_log(LOG_ERR, cs->addrstr, "Missing version", NULL);
        return;
    }

    strncpy(logbuff, buff, http_version - buff);
    console_log(LOG_INFO, cs->addrstr, logbuff, NULL);

    /* Handle methods */
    if (strncmp(buff, "GET", 3) == 0) {
        
    } else {
        snprintf(sendbuff, BUFF_SIZE, "HTTP/1.1 501 Not Implemented\n");
        convertcrlf(sendbuff, BUFF_SIZE);
        send(cs->fd, sendbuff, strlen(sendbuff), 0);
    }
}
