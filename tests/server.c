#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "EEEClient.xrs.h"
#include "EEEServer.xrs.h"

static xr_server* server = NULL;

/* stop server on signal */

static void _sig_stop(int signum)
{
  xr_server_stop(server);
  xr_server_free(server);
  exit(0);
}

int _check_err(GError* err)
{
  if (err)
  {
    g_print("\n** ERROR **: %s\n\n", err->message);
    return -1;
  }
  return 0;
}

int main(int ac, char* av[])
{
  GError* err = NULL;

  /* hookup signals to stop server, see above */
#ifndef WIN32
  signal(SIGINT, _sig_stop);
  signal(SIGHUP, _sig_stop);
#endif

  /* create new server and bind it to the port 444 */
  server = xr_server_new("server.pem", 10, &err);
  if (_check_err(err))
    goto err;
  xr_server_bind(server, "*:4444", &err);
  if (_check_err(err))
    goto err;

  xr_server_register_servlet(server, __EEEClientServlet_def());
  xr_server_register_servlet(server, __EEEServerServlet_def());

  /* run server */
  xr_server_run(server, &err);
  if (_check_err(err))
    goto err;

  /* free server after it is stopped */
  xr_server_free(server);
  return 0;

 err:
  xr_server_free(server);
  return 1;
}
