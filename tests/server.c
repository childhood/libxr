#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "TTest1.xrs.h"

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
    g_print("** ERROR **: %s\n", err->message);
    return -1;
  }
  return 0;
}

int main(int ac, char* av[])
{
  GError* err = NULL;

  if (!g_thread_supported())
    g_thread_init(NULL);

  /* hookup signals to stop server, see above */
#ifndef WIN32
  struct sigaction act;
  act.sa_handler = _sig_stop;
  act.sa_flags = SA_RESTART;
  sigemptyset(&act.sa_mask);
  if (sigaction(SIGINT, &act, NULL) < 0
   || sigaction(SIGHUP, &act, NULL) < 0
   || sigaction(SIGTERM, &act, NULL) < 0)
    goto err;
#endif

  xr_debug_enabled = XR_DEBUG_ALL;

  /* create new server and bind it to the port 444 */
  server = xr_server_new("server.pem", 10, &err);
  if (_check_err(err))
    return 1;
  xr_server_bind(server, "*:4444", &err);
  if (_check_err(err))
    goto err;

  xr_server_register_servlet(server, __TTest1Servlet_def());

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
