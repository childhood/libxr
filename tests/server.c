#include "TTest1.xrs.h"
#include "TTest2.xrs.h"

int main(int ac, char* av[])
{
  GError* err = NULL;
  xr_servlet_def* servlets[3] = {
    __TTest1Servlet_def(),
    __TTest2Servlet_def(),
    NULL
  };

//  xr_debug_enabled = XR_DEBUG_ALL;

  xr_server_simple("server.pem", 5, "*:4444", servlets, &err);
  if (err)
    g_print("error: %s\n", err->message);

  xr_fini();

  return !!err;
}
