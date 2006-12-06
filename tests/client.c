#include <stdio.h>
#include <signal.h>

#include "EEEClient.xrc.h"
#include "EEEServer.xrc.h"

/* this function prints client error if any and resets error so that futher calls to client funcs work */

static int _check_err(GError* err)
{
  if (err)
  {
    g_print("\n** ERROR **: %s\n\n", err->message);
    g_error_free(err);
    return 1;
  }
  return 0;
}

int main(int ac, char* av[])
{
  GError* err = NULL;
  char* uri = ac == 2 ? av[1] : "https://localhost:4444/EEEClient";

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
  EEEDateTime* t = EEEClient_getTime(conn, &err);
  _check_err(err);
  err = NULL;
  EEEDateTime_free(t);
  
  /* call some more methods */
  EEEUser* u = EEEClient_getUserData(conn, "bob", &err);
  if (!_check_err(err))
  {
    EEEClient_setUserData(conn, u, &err);
    _check_err(err);
  }
  EEEUser_free(u);

  /* disconnect */
  xr_client_close(conn);
  
  /* free connections object */
  xr_client_free(conn);

  return 0;
}
