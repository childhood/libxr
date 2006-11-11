#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <glib.h>

#include "xr-server.h"
#include "EEEClient.xrs.c"

/* servlet implementation stubs */

#if 0
typedef struct _testServlet testServlet;

struct _testServlet
{
  int val;
};

int testServlet_testMethod(testServlet* obj, char* str, int val)
{
  return obj->val;
}

int testServlet_construct(testServlet* obj)
{
  obj->val = 6;
}

void testServlet_destruct(testServlet* obj)
{
}

/* behind the scenes generated stuff */

testServlet* testServlet_new()
{
  testServlet* obj = g_new0(testServlet, 1);
  if (!testServlet_construct(obj))
  {
    g_free(obj);
    return NULL;
  }
  return obj;
}

void testServlet_free(testServlet* obj)
{
  testServlet_destruct(obj);
  g_free(obj);
}

static int testServlet_testMethod_xr(testServlet* obj, xr_call* call)
{
  char* str;
  int val;
  
  int retval = testServlet_testMethod(obj, str, val);
  xr_call_set_retval(call, xr_value_int_new(retval));
}

int xr_server_servlet_call(xr_server* server, xr_servlet* servlet, xr_call* call)
{
  // check if servlet was called and init new servlet (based on resource)
  // call servlet method
  // 
}

int xr_server_servlet_init(xr_server* server, xr_servlet* servlet)
{
}

int xr_server_servlet_free(xr_server* server, xr_servlet* servlet)
{
}
#endif

/* generic stuff */


// conn == object


/* main() */

static GOptionEntry entries[] = 
{
//  { "", '\0', 0, G_OPTION_ARG_STRING, &, "", "" },
//  { "", '\0', 0, G_OPTION_ARG_NONE, &, "", 0 },
  { NULL }
};

static xr_server* server = NULL;

static void _stop(int signum)
{
  xr_server_stop(server);
}

int main(int ac, char* av[])
{
  GOptionContext* ctx = g_option_context_new("- XML-RPC SSL Spike Server.");
  g_option_context_add_main_entries(ctx, entries, NULL);
  g_option_context_parse(ctx, &ac, &av, NULL);
  g_option_context_set_help_enabled(ctx, TRUE);

  if (!g_thread_supported())
    g_thread_init(NULL);

  signal(SIGINT, _stop);

  xr_server_init();
  
  server = xr_server_new("server.pem", "*:444");
  if (server == NULL)
  {
    printf("error: can't initialize server\n");
    exit(1);
  }

  xr_server_run(server);

  xr_server_free(server);

  return 1;
}
