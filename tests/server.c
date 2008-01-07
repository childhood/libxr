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

#include "TTest1.xrs.h"
#include "TTest2.xrs.h"

int main(int ac, char* av[])
{
  GError* err = NULL;
  xr_servlet_def* servlets[3] = {
    __TTest1Servlet_def(),
    __TTest2Servlet_def(),
    NULL
  };

  if (!g_thread_supported())
    g_thread_init(NULL);

  xr_debug_enabled = XR_DEBUG_CALL | XR_DEBUG_HTTP;

  xr_server_simple("server.pem", 5, "*:4444", servlets, &err);
  if (err)
    g_print("error: %s\n", err->message);

  xr_fini();

  return !!err;
}
