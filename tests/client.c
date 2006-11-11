#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <glib.h>

#include "xr-client.h"
#include "EEEClient.h"

#include "bench.h"

static int print_error(xr_client_conn* conn)
{
  if (xr_client_get_error_code(conn))
  {
    fprintf(stderr, "error[%d]: %s\n", xr_client_get_error_code(conn), xr_client_get_error_message(conn));
    xr_client_reset_error(conn);
    return 1;
  }
  return 0;
}

/* main() */

static char* opt_uri = "http://localhost:80";

static GOptionEntry entries[] = 
{
  { "uri", 'u', 0, G_OPTION_ARG_STRING, &opt_uri, "URI of the resource.", "URI" },
  { NULL }
};

int main(int ac, char* av[])
{
  GOptionContext* ctx = g_option_context_new("- XML-RPC SSL Spike Client.");
  g_option_context_add_main_entries(ctx, entries, NULL);
  g_option_context_parse(ctx, &ac, &av, NULL);
  g_option_context_set_help_enabled(ctx, TRUE);

  signal(SIGPIPE, SIG_IGN);

  reset_timers();
  start_timer(9);
  xr_client_init();
  stop_timer(9);

  // connect
  start_timer(0);
  xr_client_conn* conn = xr_client_new();
  if (xr_client_open(conn, opt_uri))
  {
    printf("Can't open connection.\n");
    xr_client_free(conn);
    exit(1);
  }

  // test
  continue_timer(1);
  EEEDateTime* t = EEEClient_getTime(conn);
  if (!print_error(conn))
  {
  }
  EEEUser* u = EEEClient_getUserData(conn, "bob");
  if (!print_error(conn))
  {
    EEEClient_setUserData(conn, u);
  }
  //EEEUser_free(u);
  stop_timer(1);

  // disconnect
  xr_client_close(conn);
  xr_client_free(conn);
  stop_timer(0);

  g_option_context_free(ctx);

  // print results
  print_timer(0, "connect(), test(), close()");
  print_timer(1, "test()");
  print_timer(9, "init()");
  return 0;
}
