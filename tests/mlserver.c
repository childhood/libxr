#include <stdio.h>
#include <signal.h>

#include "TTest1.xrs.h"

static int _check_err(GError* err)
{
  if (err)
  {
    g_print("** ERROR **: %s\n", err->message);
    return -1;
  }
  return 0;
}

/* stop server on signal */

GMainLoop* mainloop;

static void _sig_stop(int signum)
{
  if (g_main_loop_is_running(mainloop))
    g_main_loop_quit(mainloop);
}

int main(int ac, char* av[])
{
  GError* err = NULL;

  mainloop = g_main_loop_new(NULL, FALSE);

  /* hookup signals to stop server, see above */
#ifndef WIN32
  struct sigaction act;
  act.sa_handler = _sig_stop;
  act.sa_flags = SA_RESTART;
  sigemptyset(&act.sa_mask);
  if (sigaction(SIGINT, &act, NULL) < 0
   || sigaction(SIGHUP, &act, NULL) < 0
   || sigaction(SIGTERM, &act, NULL) < 0)
  {
    g_main_loop_unref(mainloop);
    return 1;
  }
#endif

  xr_debug_enabled = XR_DEBUG_ALL;

  /* create new server and bind it to the port 444 */
  xr_server* server = NULL;
  server = xr_server_new("server.pem", 20, &err);
  if (_check_err(err))
    goto err1;
  xr_server_bind(server, "*:4444", &err);
  if (_check_err(err))
    goto err1;

  xr_server_register_servlet(server, __TTest1Servlet_def());

  GSource* source = xr_server_source(server);
  g_source_attach(source, NULL);
  g_source_unref(source);

  /* run server */
  g_main_loop_run(mainloop);

  /* free server after it is stopped */
  xr_server_free(server);
  g_main_loop_unref(mainloop);
  return 0;

 err1:
  xr_server_free(server);
  g_main_loop_unref(mainloop);
  return 1;
}
