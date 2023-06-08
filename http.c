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

#include <linux/limits.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#include "config.h"
#include "strutils.h"
#include "log.h"

#include "http.h"


char sendbuff[BUFF_SIZE];
char logbuff[1024];
char endpoint[1024];


#define AUTOINDEX_INTRO \
"<!DOCTYPE html>\n" \
"<html>\n" \
"  <head>\n" \
"  </head>\n" \
"  <body>\n" \
"    <h1>Index Of %s</h1>\n" \
"    <hr>\n" \
"    <table>\n" \
"      <tr>\n" \
"        <th>Name</th>\n" \
"        <th>Size</th>\n" \
"        <th>Type</th>\n" \
"        <th>Date</th>\n" \
"      </tr>\n"

#define AUTOINDEX_OUTRO \
"    </table>\n" \
"  </body>\n" \
"</html>\n"



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


/* Status */
void
send404(const client_t *cs) {
    snprintf(sendbuff, BUFF_SIZE, "HTTP/1.1 404 Not Found\n\n");
    convertcrlf(sendbuff, BUFF_SIZE);
    if (send(cs->fd, sendbuff, strlen(sendbuff), 0) < 0) {
        console_log(LOG_ERR, cs->addrstr, "Error sending: ", strerror(errno));
    }
}

void
send403(const client_t *cs) {
    snprintf(sendbuff, BUFF_SIZE, "HTTP/1.1 403 Forbidden\n\n");
    convertcrlf(sendbuff, BUFF_SIZE);
    if (send(cs->fd, sendbuff, strlen(sendbuff), 0) < 0) {
        console_log(LOG_ERR, cs->addrstr, "Error sending: ", strerror(errno));
    }
}

void
send501(const client_t *cs) {
    snprintf(sendbuff, BUFF_SIZE, "HTTP/1.1 501 Not Implemented\n\n");
    convertcrlf(sendbuff, BUFF_SIZE);
    if (send(cs->fd, sendbuff, strlen(sendbuff), 0) < 0) {
        console_log(LOG_ERR, cs->addrstr, "Error sending: ", strerror(errno));
    }
}

void
send503(const client_t *cs) {
    snprintf(sendbuff, BUFF_SIZE, "HTTP/1.1 503 Service Unavailable\n\n");
    convertcrlf(sendbuff, BUFF_SIZE);
    if (send(cs->fd, sendbuff, strlen(sendbuff), 0) < 0) {
        console_log(LOG_ERR, cs->addrstr, "Error sending: ", strerror(errno));
    }
}

void
send200(const client_t *cs, const char *headers) {
    snprintf(sendbuff, BUFF_SIZE, "HTTP/1.1 200 OK\n\n");
    convertcrlf(sendbuff, BUFF_SIZE);
    if (send(cs->fd, sendbuff, strlen(sendbuff), 0) < 0) {
        console_log(LOG_ERR, cs->addrstr, "Error sending: ", strerror(errno));
    }
}

void
sendfile(const client_t *cs, FILE *file, size_t size) {
    size_t nread = 0;
    while (size) {
        nread = fread(sendbuff, 1, BUFF_SIZE, file);

        if (send(cs->fd, sendbuff, BUFF_SIZE, 0) < 0) {
            console_log(LOG_ERR, cs->addrstr, "Error sending: ",
                strerror(errno));
        }

        size -= nread;
    }
}

void
sendautoindex(const client_t *cs, DIR *dir, const char *path,
    const char *endpoint)
{
    snprintf(sendbuff, BUFF_SIZE, AUTOINDEX_INTRO, endpoint);
    struct dirent *direntry;
    struct stat statbuf;
    char tempbuff[PATH_MAX];

    while ((direntry = readdir(dir)) != NULL) {
        if (strcmp(direntry->d_name, ".") == 0) continue;
        strlcat(sendbuff, "<tr>\n<td>", BUFF_SIZE);
        strlcat(sendbuff, direntry->d_name, BUFF_SIZE);
        strlcat(sendbuff, "</td>\n<td>", BUFF_SIZE);

        snprintf(tempbuff, PATH_MAX, "%s%s", path, direntry->d_name);
        if (stat(tempbuff, &statbuf) < 0) {
            console_log(LOG_DBG, tempbuff, "Error stating: ",
                strerror(errno));
            tempbuff[0] = '\0';
        } else {
            human_size(statbuf.st_size, tempbuff, PATH_MAX);
        }
        strlcat(sendbuff, tempbuff, BUFF_SIZE);
        strlcat(sendbuff, "</td>\n<td>", BUFF_SIZE);
        strlcat(sendbuff, "</td>\n<td>", BUFF_SIZE);
        snprintf(tempbuff, PATH_MAX, "%d", statbuf.st_mtime);
        strlcat(sendbuff, tempbuff, BUFF_SIZE);
        strlcat(sendbuff, "</td>\n</tr>", BUFF_SIZE);
    }
    strlcat(sendbuff, AUTOINDEX_OUTRO, BUFF_SIZE);

    if (send(cs->fd, sendbuff, strlen(sendbuff), 0) < 0) {
        console_log(LOG_ERR, cs->addrstr, "Error sending: ",
            strerror(errno));
    }
}


location_node_t *
location_find(location_node_t **head, const char *endpoint) {
    if (!head) return NULL;
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
        if (head->type == CONFIG_ROOT)
            return head->param1;
        head = head->next;
    }
    return NULL;
}

const char *
config_find_index(config_node_t *head) {
    while (head) {
        if (head->type == CONFIG_INDEX)
            return head->param1;
        head = head->next;
    }
    return NULL;
}


int
config_find_autoindex(config_node_t *head) {
    while (head) {
        if (head->type == CONFIG_AUTOINDEX)
            return 1;
        head = head->next;
    }
    return 0;
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
        if (!location) {
            strlcat(logbuff, " -> 404 (no location)", 1024);
            send404(cs);
            goto doclose;
        }

        const char *webroot = config_find_root(location->config);
        if (!webroot) {
            strlcat(logbuff, " -> 503 (no webroot)", 1024);
            send503(cs);
            goto doclose;
        }

        char path[PATH_MAX];
        snprintf(path, PATH_MAX, "%s%s", webroot, endpoint);

        strlcat(logbuff, " -> ", 1024);
        strlcat(logbuff, path, 1024);


        /* Checkout file */
        struct stat statbuf;
        if (stat(path, &statbuf) < 0) {
            if (errno == EACCES) {
                send403(cs);
            } else if (errno == ENOENT) {
                send404(cs);
            } else {
                send503(cs);
            }
            console_log(LOG_DBG, cs->addrstr, "Error stating: ",
                strerror(errno));
            goto doclose;
        }
        /* It exists and its readable */

        int sendisfile = 0;
        if (S_ISREG(statbuf.st_mode)) { /* Is regular file */
            sendisfile = 1;
        } else if (S_ISDIR(statbuf.st_mode)) { /* Is directory */
            const char *index = config_find_index(location->config);
            char temppath[PATH_MAX];
            snprintf(temppath, PATH_MAX, "%s%s", path, index);
            if (index) { /* If default index defined */
                /* Check it out */
                if (stat(temppath, &statbuf) < 0) {
                    console_log(LOG_DBG, cs->addrstr, "Error stating: ",
                        strerror(errno));
                    sendisfile = 0;
                }
                /* It exists and its readable */
                strlcat(path, index, PATH_MAX);
                strlcat(logbuff, index, 1024);
                sendisfile = 1;
            } else sendisfile = 0;
        }

        if (sendisfile) {
            /* Open file */
            FILE *file = fopen(path, "rb");
            if (file) {
                strlcat(logbuff, " 200 OK", 1024);
                send200(cs, NULL);
                sendfile(cs, file, statbuf.st_size);
                fclose(file);
            } else {
                console_log(LOG_ERR, cs->addrstr, "Error fopening: ",
                    strerror(errno));
                send503(cs);
                strlcat(logbuff, " 503 Service Unavailable", 1024);
            }
        } else if (config_find_autoindex(location->config)) {
            /* If dir and autoindex enabled */
            DIR *dir = opendir(path);
            if (dir) {
                strlcat(logbuff, " 200 OK", 1024);
                send200(cs, NULL);
                sendautoindex(cs, dir, path, endpoint);
                closedir(dir);
            } else {
                console_log(LOG_ERR, cs->addrstr, "Error opendiring: ",
                    strerror(errno));
                send503(cs);
                strlcat(logbuff, " 503 Service Unavailable", 1024);
            }
        } else {
            strlcat(logbuff, " 403 Forbidden", 1024);
            send403(cs);
        }

    } else {
        strlcat(logbuff, " 501 Not Implemented", 1024);
        send501(cs);
    }

    doclose:
    close(cs->fd);

    console_log(LOG_INFO, cs->addrstr, logbuff, NULL);
}
