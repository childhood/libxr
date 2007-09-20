#include <stdlib.h>
#include <stdio.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#ifndef WIN32
  #include <signal.h>
#endif

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

static GMutex** _ssl_mutexes = NULL;
static int _ssl_initialized = 0;

static void _ssl_locking_callback(int mode, int type, const char *file, int line)
{
  if (mode & CRYPTO_LOCK)
    g_mutex_lock(_ssl_mutexes[type]);
  else
    g_mutex_unlock(_ssl_mutexes[type]);
}

static unsigned long _ssl_thread_id_callback()
{
  unsigned long ret;
  ret = (unsigned long)g_thread_self();
  return ret;
}

void xr_init()
{
  int i;

  if (_ssl_initialized)
    return;
  _ssl_initialized = 1;

  if (!g_thread_supported())
    g_thread_init(NULL);

#ifndef WIN32
  signal(SIGPIPE, SIG_IGN);
#endif

  SSL_library_init();
  ERR_load_crypto_strings();
  SSL_load_error_strings();
  ERR_load_SSL_strings();

  _ssl_mutexes = g_new(GMutex*, CRYPTO_num_locks());
  for (i=0; i<CRYPTO_num_locks(); i++)
    _ssl_mutexes[i] = g_mutex_new();
  CRYPTO_set_id_callback(_ssl_thread_id_callback);
  CRYPTO_set_locking_callback(_ssl_locking_callback);
}

void xr_fini()
{
  if (!_ssl_initialized)
    return;

#ifndef WIN32
  signal(SIGPIPE, SIG_DFL);
#endif

  if (_ssl_mutexes)
  {
    CRYPTO_set_id_callback(NULL);
    CRYPTO_set_locking_callback(NULL);
    int i;
    for (i=0; i<CRYPTO_num_locks(); i++)
      g_mutex_free(_ssl_mutexes[i]);
    g_free(_ssl_mutexes);
    _ssl_mutexes = NULL;
  }

  _ssl_initialized = 0;
}
