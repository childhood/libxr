#include <stdio.h>
#include <signal.h>

#include "TTest1.xrc.h"

static int _check_err(GError* err)
{
  if (err)
  {
    g_print("** ERROR **: %s\n", err->message);
    return 1;
  }
  return 0;
}

static gpointer _thread_func(gpointer data)
{
  GError *err = NULL;
  char* uri = data;

  xr_client_conn* conn = xr_client_new(&err);
  if (_check_err(err))
    return NULL;

  xr_client_open(conn, uri, &err);
  if (_check_err(err))
  {
    xr_client_free(conn);
    return NULL;
  }

  int stop = 0;
  for (int i=0; i<100 && !stop; i++)
  {
    /* call some servlet methods */
    TAllTypes* t = TTest1_getAll(conn, &err);
    if (_check_err(err))
    {
      if (err->domain == XR_CLIENT_ERROR)
        stop = 1;
      g_clear_error(&err);
    }
    TAllTypes_free(t);
  }
  
  xr_client_close(conn);
  xr_client_free(conn);
  return NULL;
}

int main(int ac, char* av[])
{
  GError* err = NULL;
  GThread* t[1024];
  int count = 100, i;
  char* uri = ac == 2 ? av[1] : "https://localhost:4444/TTest1";

  if (!g_thread_supported())
    g_thread_init(NULL);

  for (i=0; i<count; i++)
  {
    t[i] = g_thread_create(_thread_func, uri, TRUE, &err);
    g_assert(err == NULL);
  }

  for (i=0; i<count; i++)
    g_thread_join(t[i]);

  return 0;
}
