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

void xr_set_nodelay(GSocket* sock)
{
  int flag = 1;
  int fd = g_socket_get_fd(sock);
  if (fd >= 0)
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
}
