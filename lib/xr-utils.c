/* 
 * Copyright 2006-2008 Ondrej Jirman <ondrej.jirman@zonio.net>
 * 
 * This file is part of libxr.
 *
 * Libxr is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 2 of the License, or (at your option) any
 * later version.
 *
 * Libxr is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libxr.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef WIN32
  #include <winsock2.h>
#else
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/types.h>
  #include <netinet/tcp.h>
#endif

#include "xr-utils.h"

void xr_set_nodelay(BIO* bio)
{
  int flag = 1;
  int sock = -1;
  BIO_get_fd(bio, &sock);
  if (sock >= 0)
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
  else
    fprintf(stderr, "Error disabling Nagle.\n");
}

const char* xr_get_bio_error_string()
{
  const char* str;
  if ((str = ERR_reason_error_string(ERR_get_error())))
    return str;
#ifndef WIN32
  if ((str = g_strerror(errno)))
    return str;
#else
  if (WSAGetLastError())
    return "WSA ERROR.";
#endif
  return "Unknown or no error.";
}
