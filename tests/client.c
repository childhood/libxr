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

int main(int ac, char* av[])
{
  GError* err = NULL;
  char* uri = ac == 2 ? av[1] : "https://localhost:4444/RPC2";

  xr_debug_enabled = XR_DEBUG_CALL | XR_DEBUG_HTTP | XR_DEBUG_ALL;

  if (!g_thread_supported())
    g_thread_init(NULL);

  /* create object for performing client connections */
  xr_client_conn* conn = xr_client_new(&err);
  if (_check_err(err))
    return 1;

  xr_client_basic_auth(conn, "user", "pass");

  /* connect to the servlet on the server specified by uri */
  xr_client_open(conn, uri, &err);
  if (_check_err(err))
  {
    xr_client_free(conn);
    return 1;
  }

#ifdef XR_JSON_ENABLED
  /* use json transport for big arrays */
  xr_client_set_transport(conn, XR_CALL_JSON_RPC);
#endif 

  GArray* arr = TTest1_getBigArray(conn, &err);
  _check_err(err);
  err = NULL;
  Array_string_free(arr);

  int i;
  arr = Array_string_new();
  for (i=0; i<5000; i++)
    Array_string_add(arr, g_strdup_printf("user.bob%d@zonio.net", i));
  TTest1_putBigArray(conn, arr, &err);
  _check_err(err);
  err = NULL;
  Array_string_free(arr);

  xr_client_set_transport(conn, XR_CALL_XML_RPC);
  
  /* call some servlet methods */
  TAllTypes* t1 = TTest1_getAll(conn, &err);
  _check_err(err);
  err = NULL;
  TAllTypes_free(t1);

  TAllArrays* t2 = TTest1_getAllArrays(conn, &err);
  _check_err(err);
  err = NULL;
  TAllArrays_free(t2);

  TTest2_auth(conn, "name", "pass", &err);
  _check_err(err);
  err = NULL;

  t1 = TTest1_getAll(conn, &err);
  _check_err(err);
  err = NULL;
  TAllTypes_free(t1);
  
  /* call undefined servlet methods */
  
  xr_call* call = xr_call_new("TTest1.unDefined");
  xr_call_add_param(call, xr_value_build("{s:i,*}", "test", 100));
  xr_client_call(conn, call, &err);
  _check_err(err);
  err = NULL;
  
  /* free connections object */
  xr_client_free(conn);

  xr_fini();

  return 0;
}
