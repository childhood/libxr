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

#include <stdlib.h>
#include <stdio.h>

#include "xr-lib.h"
#include "xr-http.h"

int xr_debug_enabled = 0;

void _xr_debug(const char* loc, const char* fmt, ...)
{
  va_list ap;
  char* msg;

  if (fmt == NULL)
    return;
  
  va_start(ap, fmt);
  msg = g_strdup_vprintf(fmt, ap);
  va_end(ap);

  g_printerr("%s%s\n", loc ? loc : "", msg);
  g_free(msg);
}

G_LOCK_DEFINE_STATIC(init);

void xr_init()
{
  g_type_init();

  if (!g_thread_supported())
    g_error("xr_init(): You must call g_thread_init() to allow libxr to work correctly.");

  G_LOCK(init);

  xr_http_init();

  G_UNLOCK(init);
}

void xr_fini()
{
}
