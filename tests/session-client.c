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

#include <stdio.h>
#include <string.h>

#include "TTest1.xrc.h"
#include "TTest2.xrc.h"

/* this function prints client error if any and resets error so that futher calls to client funcs work */

static char* uri;

static int _check_err(GError* err)
{
  if (err)
  {
    g_print("** ERROR **: %s\n", err->message);
    g_error_free(err);
    return 1;
  }
  return 0;
}

gboolean connect_call_disconnect(int id)
{
  GError* err = NULL;

  /* create object for performing client connections */
  xr_client_conn* conn = xr_client_new(&err);
  if (_check_err(err))
    return TRUE;

  /* connect to the servlet on the server specified by uri */
  xr_client_open(conn, uri, &err);
  if (_check_err(err))
  {
    xr_client_free(conn);
    return TRUE;
  }

  char* session_id = g_strdup_printf("%d", id);
  xr_client_set_http_header(conn, "X-Libxr-Session-ID", session_id);
  g_free(session_id);

#ifdef XR_JSON_ENABLED
  xr_client_set_transport(conn, XR_CALL_JSON_RPC);
#endif

  TTest2_auth(conn, "a", "b", &err);
  _check_err(err);
  err = NULL;
  TTest2_getUsername(conn, &err);
  _check_err(err);
  err = NULL;
  
  xr_client_free(conn);
  
  return FALSE;
}

int main(int ac, char* av[])
{
  GError* err = NULL;
  GThread* t[100];
  int i;
  
  uri = ac == 2 ? av[1] : "https://localhost:4444/RPC2";

  if (!g_thread_supported())
    g_thread_init(NULL);

  for (i = 0; i < 100; i++)
    t[i] = g_thread_create((GThreadFunc)connect_call_disconnect, i % 1, TRUE, NULL);

  for (i = 0; i < 100; i++)
    g_thread_join(t[i]);

  xr_fini();

  return 0;
}
