#include <stdio.h>
#include <signal.h>

#include "TTest1.xrc.h"

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
  char* uri = ac == 2 ? av[1] : "https://localhost:4444/TTest1";
  xr_debug_enabled = XR_DEBUG_ALL;

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

  /* call some servlet methods */
  TAllTypes* t = TTest1_getAll(conn, &err);
  _check_err(err);
  err = NULL;
  TAllTypes_free(t);
  
  /* disconnect */
  xr_client_close(conn);
  
  /* free connections object */
  xr_client_free(conn);

  return 0;
}
