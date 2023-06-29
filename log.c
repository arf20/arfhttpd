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

    log.c: logging

*/

#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#include "strutils.h"

#include "log.h"

#define LOG_BUFFER_SIZE 1024

static char logbuff[LOG_BUFFER_SIZE];
struct timeval tv;
struct tm *tm_local;

void console_log(int severity, const char *client, const char *msg,
    const char *str)
{
    logbuff[0] = '[';

    gettimeofday(&tv, NULL);
    tm_local = localtime(&tv.tv_sec);
    float sec = (float)tm_local->tm_sec + ((float)tv.tv_usec / 1000000.0f);

    int aftertime = strftime(logbuff + 1, LOG_BUFFER_SIZE - 1,
        "%d/%m/%Y:%H:%M:", tm_local);
    snprintf(logbuff + 1 + aftertime, LOG_BUFFER_SIZE - 1 - aftertime, 
        "%06.5f] ", sec);

    switch (severity) {
        case LOG_ERR:  strlcat(logbuff, "ERROR:",   LOG_BUFFER_SIZE); break;
        case LOG_WARN: strlcat(logbuff, "Warning:", LOG_BUFFER_SIZE); break;
        case LOG_INFO: strlcat(logbuff, "Info:",    LOG_BUFFER_SIZE); break;
        case LOG_DBG:  strlcat(logbuff, "Debug:",   LOG_BUFFER_SIZE); break;
    }

    strlcat(logbuff, "\t", LOG_BUFFER_SIZE);

    if (client)
        strlcat(logbuff, client, LOG_BUFFER_SIZE);

    strlcat(logbuff, "\t", LOG_BUFFER_SIZE);

    strlcat(logbuff, msg, LOG_BUFFER_SIZE);

    if (str)
        strlcat(logbuff, str, LOG_BUFFER_SIZE);

    puts(logbuff);
}
