#include "ZMServer.xrs.h"

int main(int ac, char* av[])
{
  GError* err = NULL;
//  xr_debug_enabled = XR_DEBUG_ALL;

  // start HTTPS server, with 5 threads in the pool, listening on port 4444,
  // and implementing ZMServer servlet.
  xr_server_simple("server.pem", 5, "*:1234", __ZMServerServlet_def(), &err);
  if (err)
    g_print("error: %s\n", err->message);

  return !!err;
}
