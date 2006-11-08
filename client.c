#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "xmlrpc-client.h"

#include "bench.h"

/* XML-RPC methods proxy implementation */

int testMethod(xr_client_conn* conn, char* str, int val)
{
  int retval = -1;
  xr_call* call = xr_call_new(__func__);
//  xr_call_add_param(call, xr_value_blob_new(str, strlen(str)));
  xr_call_add_param(call, xr_value_string_new(str));
  xr_call_add_param(call, xr_value_int_new(val));
  xr_client_call(conn, call);
  if (call->retval)
    retval = call->retval->int_val;
  xr_call_free(call);
  return retval;
}

/* test(conn) */

void test(xr_client_conn* conn)
{
  testMethod(conn, "test_p1", 6);
}

/* main() */

static char* opt_uri = "127.0.0.1:444";
//static char* opt_uri = "127.0.0.1:80";

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
  for (int i=0; i<100; i++)
  {
    continue_timer(1);
    test(conn);
    stop_timer(1);
  }

  // disconnect
  xr_client_close(conn);
  xr_client_free(conn);
  stop_timer(0);

  // print results
  print_timer(0, "connect(), test(), close()");
  print_timer(1, "test()");
  print_timer(9, "init()");
  return 0;
}
