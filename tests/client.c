#include <stdio.h>
#include <signal.h>

#include "EEEClient.xrc.h"
#include "EEEServer.xrc.h"

/* this function prints client error if any and resets error so that futher calls to client funcs work */

static int _print_error(xr_client_conn* conn)
{
  if (xr_client_get_error_code(conn))
  {
    fprintf(stderr, "error[%d]: %s\n", xr_client_get_error_code(conn), xr_client_get_error_message(conn));
    xr_client_reset_error(conn);
    return 1;
  }
  return 0;
}

static int _check_err(GError* err)
{
  if (err)
  {
    g_print("\n** ERROR **: %s\n\n", err->message);
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
  EEEDateTime* t = EEEClient_getTime(conn);
  _print_error(conn);
  EEEDateTime_free(t);
  
  /* call some more methods */
  EEEUser* u = EEEClient_getUserData(conn, "bob");
  if (!_print_error(conn))
    EEEClient_setUserData(conn, u);
  EEEUser_free(u);

  /* disconnect */
  xr_client_close(conn);
  
  /* free connections object */
  xr_client_free(conn);

  return 0;
}
