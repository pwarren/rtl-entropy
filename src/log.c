/* log.c - simple logging support, from snd-egd
 *
 * (C) 2005-2010 Nicholas J. Kain <njkain at gmail dot com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#include <stdio.h>
#include <strings.h>
#include <stdarg.h>
#include <stdlib.h>

#include "defines.h"
#include "log.h"

/* global logging flags from defins.h */
char gflags_detach = GFLAGS_DETACH;
char gflags_debug = GFLAGS_DEBUG;

void log_line(int logtype, char *format, ...) {
  va_list argp;
  
  if (format == NULL)
    return;
  if (logtype == LOG_DEBUG && !gflags_debug)
    return;
  
  if (gflags_detach) {
    openlog("rtl-entropy", LOG_PID, LOG_DAEMON);
    va_start(argp, format);
    vsyslog(logtype | LOG_DAEMON, format, argp);
    va_end(argp);
    closelog();
  } else {
    va_start(argp, format);
    vfprintf(stderr, format, argp);
    fprintf(stderr, "\n");
    va_end(argp);
  }
  closelog();
}

void suicide(char *format, ...) {
  va_list argp;
  
  if (format == NULL)
    goto out;
  
  if (gflags_detach) {
    openlog("rtl-entropy", LOG_PID, LOG_DAEMON);
    va_start(argp, format);
    vsyslog(LOG_ERR | LOG_DAEMON, format, argp);
    va_end(argp);
    closelog();
  } else {
    va_start(argp, format);
    vfprintf(stderr, format, argp);
    fprintf(stderr, "\n");
    va_end(argp);
    perror(NULL);
  }
  closelog();
 out:
  exit(EXIT_FAILURE);
}

