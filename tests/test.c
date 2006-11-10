#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "xr-call.h"

#include "bench.h"

// call impl
int impl_testMethod(char* str, int val)
{
  return 4;
}

// call proxy
int xr_testMethod(xr_call* call)
{
  char* str;
  int val;
  
  int retval = impl_testMethod(str, val);
  xr_call_set_retval(call, xr_value_int_new(retval));
}

int xr_call_server(char* buf_req, int len_req, char** buf_res, int* len_res)
{
  xr_call* call = xr_call_new(NULL);
  xr_call_unserialize_request(call, buf_req, len_req);
  if (!strcmp(call->method, "testMethod"))
    xr_testMethod(call);
  else
    xr_call_set_errval(call, 100, "Unknown method!");
  xr_call_serialize_response(call, buf_res, len_res);
  xr_call_free(call);
}

int xr_call_client(xr_call* call)
{
  char* buf_req;
  int len_req;
  char* buf_res;
  int len_res;

  xr_call_serialize_request(call, &buf_req, &len_req);
  printf("---- REQ ----\n%s", buf_req);
  xr_call_server(buf_req, len_req, &buf_res, &len_res); //XXX: send/receive
  printf("---- RES ----\n%s", buf_res);
  xr_call_unserialize_response(call, buf_req, len_req);
}

int testMethod(char* str, int val)
{
  start_timer(3);
  xr_call* call = xr_call_new(__func__);
  xr_call_add_param(call, xr_value_blob_new(str, strlen(str)));
  xr_call_add_param(call, xr_value_string_new(str));
  xr_call_add_param(call, xr_value_int_new(val));
  stop_timer(3);
  xr_call_client(call);
  xr_call_free(call);
}

static GOptionEntry entries[] = 
{
//  { "", '\0', 0, G_OPTION_ARG_STRING, &, "", "" },
//  { "", '\0', 0, G_OPTION_ARG_NONE, &, "", 0 },
  { NULL }
};

int main(int ac, char* av[])
{
  GOptionContext* ctx = g_option_context_new("- XML-RPC Spike Server.");
  g_option_context_add_main_entries(ctx, entries, NULL);
  g_option_context_parse(ctx, &ac, &av, NULL);
  g_option_context_set_help_enabled(ctx, TRUE);

  reset_timers();
  start_timer(0);
  testMethod("test_p1", 6);
  stop_timer(0);
  start_timer(1);
  sleep(1);
  stop_timer(1);
  print_timer(0, "testMethod");
  print_timer(1, "1s");
  print_timer(3, "testMethod(setup)");
  return 0;
}
