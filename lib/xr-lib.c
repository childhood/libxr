#include <stdlib.h>
#include <stdio.h>
#include "xr-lib.h"

int xr_debug_enabled = 0;

static GStaticMutex mutex = G_STATIC_MUTEX_INIT;

void _xr_debug(const char* loc, const char* fmt, ...)
{
  if (fmt == NULL)
    return;
  if (!g_thread_supported())
    g_thread_init(NULL);
  g_static_mutex_lock(&mutex);
  if (loc != NULL)
    fprintf(stderr, "%s", loc);
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fprintf(stderr, "\n");
  g_static_mutex_unlock(&mutex);
}
