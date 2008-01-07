/* 
 * Copyright 2006-2008 Ondrej Jirman <ondrej.jirman@zonio.net>
 * 
 * This file is part of libxr.
 *
 * Libxr is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 2 of the License, or (at your option) any
 * later version.
 *
 * Libxr is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libxr.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#ifndef WIN32
  #include <signal.h>
#endif

#include "xr-lib.h"
#include "xr-http.h"

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

G_LOCK_DEFINE_STATIC(init);

void xr_init()
{
  int i;

  if (!g_thread_supported())
    g_error("xr_init(): You must call g_thread_init() to allow libxr to work correctly.");

  G_LOCK(init);

  if (_ssl_initialized)
  {
    G_UNLOCK(init);
    return;
  }

  xr_http_init();

#ifndef WIN32
  signal(SIGPIPE, SIG_IGN);
#endif

  SSL_library_init();
  SSL_load_error_strings();
  OpenSSL_add_all_algorithms();

  /* add zlib SSL compresssion */
  COMP_METHOD *cm = COMP_zlib();
  if (cm->type != NID_undef)
    SSL_COMP_add_compression_method(1, cm); /* 1 is DEFALTE according to the http://www.ietf.org/rfc/rfc3749.txt */

  _ssl_mutexes = g_new(GMutex*, CRYPTO_num_locks());
  for (i=0; i<CRYPTO_num_locks(); i++)
    _ssl_mutexes[i] = g_mutex_new();
  CRYPTO_set_id_callback(_ssl_thread_id_callback);
  CRYPTO_set_locking_callback(_ssl_locking_callback);

  _ssl_initialized = 1;

  G_UNLOCK(init);
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
