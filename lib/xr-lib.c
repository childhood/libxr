#include <stdlib.h>
#include <stdio.h>
#include "xr-lib.h"

int xr_debug_enabled = 0;

void _xr_debug(const char* loc, const char* fmt, ...)
{
  va_list ap;
  char* msg;

  if (fmt == NULL)
    return;
  
  va_start(ap, fmt);
  msg = g_strdup_vprintf(fmt, ap);
  va_end(ap);

  g_printerr("%s%s\n", loc ? loc : "", msg);
  g_free(msg);
}
