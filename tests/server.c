#include "TTest1.xrs.h"

int main(int ac, char* av[])
{
  GError* err = NULL;

  xr_debug_enabled = XR_DEBUG_ALL;

  xr_server_simple("server.pem", 5, "*:4444", __TTest1Servlet_def(), &err);
  if (err)
    g_print("error: %s\n", err->message);

  return !!err;
}
