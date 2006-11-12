#include <stdio.h>
#include <signal.h>

#include "EEEClient.xrs.h"
#include "EEEServer.xrs.h"

static xr_server* server = NULL;

/* stop server on signal */
static void _sig_stop(int signum)
{
  xr_server_stop(server);
}

int main(int ac, char* av[])
{
  /* initialize xr server library */
  xr_server_init();
  
  /* hookup signals to stop server, see above */
  signal(SIGINT, _sig_stop);
  signal(SIGHUP, _sig_stop);

  /* create new server and bind it to the port 444 */
  server = xr_server_new("server.pem", "*:444");
  if (server == NULL)
  {
    printf("error: can't initialize server\n");
    return 1;
  }

  xr_server_register_servlet(server, __EEEClientServlet_def());
  xr_server_register_servlet(server, __EEEServerServlet_def());

  /* run server */
  xr_server_run(server);

  /* free server after it is stopped */
  xr_server_free(server);

  return 0;
}
