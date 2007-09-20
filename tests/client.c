#include <stdio.h>
#include <signal.h>

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

//  xr_debug_enabled = XR_DEBUG_ALL;

  if (!g_thread_supported())
    g_thread_init(NULL);

  /* create object for performing client connections */
  xr_client_conn* conn = xr_client_new(&err);
  if (_check_err(err))
    return 1;

  /* connect to the servlet on the server specified by uri */
  xr_client_open(conn, uri, &err);
  if (_check_err(err))
  {
    xr_client_free(conn);
    return 1;
  }

  GSList* arr = TTest1_getBigArray(conn, &err);
  _check_err(err);
  err = NULL;
  Array_string_free(arr);

  int i;
  arr = NULL;
  for (i=0; i<5000; i++)
    arr = g_slist_append(arr, g_strdup_printf("user.bob%d@zonio.net", i));
  TTest1_putBigArray(conn, arr, &err);
  _check_err(err);
  err = NULL;
  Array_string_free(arr);

  /* call some servlet methods */
  TAllTypes* t = TTest1_getAll(conn, &err);
  _check_err(err);
  err = NULL;
  TAllTypes_free(t);

  TTest2_auth(conn, &err);
  _check_err(err);
  err = NULL;

  t = TTest1_getAll(conn, &err);
  _check_err(err);
  err = NULL;
  TAllTypes_free(t);
  
  /* free connections object */
  xr_client_free(conn);

  xr_fini();

  return 0;
}
