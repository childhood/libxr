#include <stdio.h>
#include <signal.h>

#include "EEEClient.xrs.h"
#include "EEEServer.xrs.h"

static int _check_err(GError* err)
{
  if (err)
  {
    g_print("\n** ERROR **: %s\n\n", err->message);
    return -1;
  }
  return 0;
}

/* stop server on signal */

GMainLoop* mainloop;

static void _sig_stop(int signum)
{
  g_main_loop_quit(mainloop);
}

int main(int ac, char* av[])
{
  GError* err = NULL;

  mainloop = g_main_loop_new(NULL, FALSE);

  /* hookup signals to stop server, see above */
#ifndef WIN32
  signal(SIGINT, _sig_stop);
  signal(SIGHUP, _sig_stop);
#endif

  /* create new server and bind it to the port 444 */
  xr_server* server = NULL;
  server = xr_server_new("server.pem", 10, &err);
  if (_check_err(err))
    goto err;
  xr_server_bind(server, "*:444", &err);
  if (_check_err(err))
    goto err;

  xr_server_register_servlet(server, __EEEClientServlet_def());
  xr_server_register_servlet(server, __EEEServerServlet_def());

  GSource* source = xr_server_source(server);
  g_source_attach(source, NULL);
  g_source_unref(source);

  /* run server */
  g_main_loop_run(mainloop);

  /* free server after it is stopped */
  xr_server_free(server);
  return 0;

 err:
  xr_server_free(server);
  return 1;
}
