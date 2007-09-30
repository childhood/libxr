/* 
 * Copyright 2006, 2007 Ondrej Jirman <ondrej.jirman@zonio.net>
 * 
 * This file is part of libxr.
 *
 * Libxr is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
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

#ifdef WIN32
  #include <winsock2.h>
#else
  #include <sys/select.h>
  #include <signal.h>
#endif
#include <stdlib.h>

#include "xr-server.h"
#include "xr-http.h"
#include "xr-utils.h"

/* server */

struct _xr_server
{
  SSL_CTX* ctx;
  BIO* bio_in;
  int sock;
  BIO* bio_ssl;
  GThreadPool* pool;
  gboolean secure;
  gboolean running;
  GSList* servlet_types;
};

/* servlet API */

struct _xr_servlet
{
  void* priv;
  xr_servlet_def* def;
  xr_call* call;
};

typedef struct _xr_server_conn xr_server_conn;
struct _xr_server_conn
{
  BIO* bio;
  GPtrArray* servlets;
  gboolean running;
};

static xr_servlet* xr_servlet_new(xr_servlet_def* def)
{
  xr_servlet* s = g_new0(xr_servlet, 1);
  if (def->size > 0)
    s->priv = g_malloc0(def->size);
  s->def = def;
  return s;
}

static void xr_servlet_free(xr_servlet* servlet, gboolean fini)
{
  if (servlet == NULL)
    return;
  if (fini && servlet->def && servlet->def->fini)
    servlet->def->fini(servlet);
  g_free(servlet->priv);
  g_free(servlet);
}

static xr_server_conn* xr_server_conn_new(BIO* bio)
{
  xr_server_conn* c = g_new0(xr_server_conn, 1);
  c->servlets = g_ptr_array_sized_new(3);
  c->bio = bio;
  c->running = TRUE;
  return c;
}

static void xr_server_conn_free(xr_server_conn* conn)
{
  if (conn == NULL)
    return;
  BIO_free_all(conn->bio);
  g_ptr_array_foreach(conn->servlets, (GFunc)xr_servlet_free, (gpointer)TRUE);
  g_ptr_array_free(conn->servlets, TRUE);
  g_free(conn);
}

static xr_servlet* xr_server_conn_find_servlet(xr_server_conn* conn, const char* name)
{
  int i;

  for (i = 0; i < conn->servlets->len; i++)
  {
    xr_servlet* servlet = conn->servlets->pdata[i];
    if (!strcmp(servlet->def->name, name))
      return servlet;
  }

  return NULL;
}

void* xr_servlet_get_priv(xr_servlet* servlet)
{
  g_assert(servlet != NULL);
  return servlet->priv;
}

static xr_servlet_def* _find_servlet_def(xr_server* server, char* name)
{
  GSList* i;

  for (i = server->servlet_types; i; i = i->next)
  {
    xr_servlet_def* def = i->data;
    if (!strcasecmp(def->name, name))
      return def;
  }

  return NULL;
}

static xr_servlet_method_def* _find_servlet_method_def(xr_servlet* servlet, const char* name)
{
  int i;

  g_assert(servlet != NULL);
  g_assert(servlet->def != NULL);

  for (i = 0; i < servlet->def->methods_count; i++)
    if (!strcmp(servlet->def->methods[i].name, name))
      return servlet->def->methods + i;

  return NULL;
}

static int _xr_server_servlet_method_call(xr_server* server, xr_server_conn* conn, xr_call* call, xr_http* http)
{
  int retval = -1;
  xr_servlet* servlet = NULL;
  xr_servlet_method_def* method;
  char *servlet_name, *tmp;

  g_assert(server != NULL);
  g_assert(conn != NULL);
  g_assert(call != NULL);

  /* identify servlet name */
  servlet_name = g_strdup(xr_call_get_method_full(call));
  if (servlet_name && (tmp = strchr(servlet_name, '.')) && tmp != servlet_name)
    *tmp = '\0';
  else
  {
    g_free(servlet_name);
    servlet_name = xr_http_get_resource(http);
    if (servlet_name)
      memmove(servlet_name, servlet_name + 1, strlen(servlet_name));
  }

  /* get xr_servlet object for current connection and given servlet name */
  servlet = xr_server_conn_find_servlet(conn, servlet_name);
  if (servlet == NULL)
  {
    xr_servlet_def* def = _find_servlet_def(server, servlet_name);
    if (def == NULL)
    {
      xr_call_set_error(call, 100, "Unknown servlet.");
      goto done;
    }

    servlet = xr_servlet_new(def);
    if (servlet->def->init(servlet) < 0)
    {
      xr_call_set_error(call, 100, "Servlet initialization failed.");
      xr_servlet_free(servlet, FALSE);
      servlet = NULL;
      goto done;
    }

    g_ptr_array_add(conn->servlets, servlet);
  }
  
  servlet->call = call;

  /* find method and perform a call */
  method = _find_servlet_method_def(servlet, xr_call_get_method(call));
  if (method == NULL)
  {
    char* msg = g_strdup_printf("Method %s not found in %s servlet.", xr_call_get_method(call), servlet->def->name);
    xr_call_set_error(call, 100, msg);
    g_free(msg);
    goto done;
  }

  if (servlet->def->pre_call)
  {
    if (!servlet->def->pre_call(servlet, call))
    {
      // FALSE returned
      if (xr_call_get_retval(call) == NULL && !xr_call_get_error_code(call))
        xr_call_set_error(call, -1, "Pre-call did not returned value or set error.");
      goto done;
    }
  }

  retval = method->cb(servlet, call);

  if (servlet->def->post_call)
    servlet->def->post_call(servlet, call);

 done:
  g_free(servlet_name);
  if (servlet)
    servlet->call = NULL;
  return retval;
}

static int _xr_server_servlet_call(xr_server* server, xr_server_conn* conn)
{
  char* buffer;
  int length, rs, retval = -1;
  xr_http* http;
  xr_call* call;

  xr_trace(XR_DEBUG_SERVER_TRACE, "(server=%p, conn=%p)", server, conn);

  g_assert(server != NULL);
  g_assert(conn != NULL);

  /* receive HTTP request */
  http = xr_http_new(conn->bio);
  if (xr_http_receive(http, XR_HTTP_REQUEST, &buffer, &length) < 0)
    goto err;

  /* parse request data into xr_call */
  call = xr_call_new(NULL);
  rs = xr_call_unserialize_request(call, buffer, length);
  g_free(buffer);

  /* run call */
  if (rs)
    xr_call_set_error(call, 100, "Unserialize request failure.");
  else
    _xr_server_servlet_method_call(server, conn, call, http);

  /* generate response data from xr_call */
  xr_call_serialize_response(call, &buffer, &length);
  if (xr_debug_enabled & XR_DEBUG_CALL)
    xr_call_dump(call, 0);
  xr_call_free(call);

  /* send HTTP response */
  xr_http_setup_response(http, 200);
  rs = xr_http_send(http, XR_HTTP_RESPONSE, buffer, length);
  xr_call_free_buffer(buffer);
  if (rs < 0)
    goto err;

  retval = 0;
 err:
  xr_http_free(http);
  return retval;
}

static int _xr_server_connection_thread(xr_server_conn* conn, xr_server* server)
{
  xr_trace(XR_DEBUG_SERVER_TRACE, "(conn=%p, server=%p)", conn, server);

  g_assert(conn != NULL);
  g_assert(server != NULL);

  if (server->secure)
  {
    if (BIO_do_handshake(conn->bio) <= 0)
      goto done;
  }

  while (conn->running)
    if (_xr_server_servlet_call(server, conn) < 0)
      break;

 done:
  xr_server_conn_free(conn);
  return 0;
}

void xr_server_stop(xr_server* server)
{
  xr_trace(XR_DEBUG_SERVER_TRACE, "(server=%p)", server);
  g_assert(server != NULL);
  server->running = FALSE;
}

/* wait for a connection and accept it, return -1 on fatal error, 0 on temprary
 * error or success */
static int _xr_server_accept_connection(xr_server* server, GError** err)
{
  GError* local_err = NULL;
  xr_server_conn* conn;

  xr_trace(XR_DEBUG_SERVER_TRACE, "(server=%p, err=%p)", server, err);

  g_assert(server != NULL);
  g_return_val_if_fail(err == NULL || *err == NULL, -1);

  if (BIO_do_accept(server->bio_in) <= 0)
  {
    g_set_error(err, XR_SERVER_ERROR, XR_SERVER_ERROR_FAILED, "accept failed: %s", xr_get_bio_error_string());
    return -1;
  }

  // new connection accepted
  conn = xr_server_conn_new(BIO_pop(server->bio_in));

  // if we have too many clients in the queue, pause accepting new ones
  // and leave some time to process existing ones, this ensures that
  // server does not get overloaded and improves latency for clients that are in
  // the queue or being processed right now
  if (g_thread_pool_unprocessed(server->pool) > 50)
    g_usleep(500000);

  // push client into the queue
  g_thread_pool_push(server->pool, conn, &local_err);
  if (local_err)
  {
    xr_server_conn_free(conn);

    // check if this is only temporary error
    if (!g_error_matches(local_err, G_THREAD_ERROR, G_THREAD_ERROR_AGAIN))
    {
      g_set_error(err, XR_SERVER_ERROR, XR_SERVER_ERROR_FAILED, "thread push failed: %s", local_err->message);
      g_clear_error(&local_err);
      return -1;
    }

    g_clear_error(&local_err);
  }

  return 0;
}

int xr_server_run(xr_server* server, GError** err)
{
  xr_trace(XR_DEBUG_SERVER_TRACE, "(server=%p, err=%p)", server, err);
  GError* local_err = NULL;
  fd_set set, setcopy;
  struct timeval tv, tvcopy;
  int maxfd;

  g_assert(server != NULL);
  g_return_val_if_fail(err == NULL || *err == NULL, -1);

  FD_ZERO(&setcopy);
  FD_SET(server->sock, &setcopy);
  maxfd = server->sock + 1;
  tvcopy.tv_sec = 0;
  tvcopy.tv_usec = 100000;
  
  server->running = TRUE;
  while (server->running)
  {
    set = setcopy;
    tv = tvcopy;
    int rs = select(maxfd, &set, NULL, NULL, &tv);
    if (rs < 0)
    {
#ifdef WIN32
      if (WSAGetLastError() == WSAEINTR)
        continue;
#else
      if (errno == EINTR)
        return 0;
#endif
      g_set_error(err, XR_SERVER_ERROR, XR_SERVER_ERROR_FAILED, "select failed: %s", g_strerror(errno));
      return -1;
    }
    if (rs == 0)
      continue;
    if (_xr_server_accept_connection(server, &local_err) < 0)
    {
      g_propagate_error(err, local_err);
      return -1;
    }
  }

  return 0;
}

int xr_server_register_servlet(xr_server* server, xr_servlet_def* servlet)
{
  xr_trace(XR_DEBUG_SERVER_TRACE, "(server=%p, servlet=%p)", server, servlet);
  g_assert(server != NULL);
  g_assert(servlet != NULL);
  if (!_find_servlet_def(server, servlet->name))
    server->servlet_types = g_slist_append(server->servlet_types, servlet);
  return 0;
}

xr_server* xr_server_new(const char* cert, int threads, GError** err)
{
  xr_trace(XR_DEBUG_SERVER_TRACE, "(cert=%s, threads=%d, err=%p)", cert, threads, err);
  GError* local_err = NULL;

  g_assert(threads > 0 && threads < 1000);
  g_return_val_if_fail (err == NULL || *err == NULL, NULL);

  xr_init();

  xr_server* server = g_new0(xr_server, 1);
  server->secure = !!cert;

  server->ctx = SSL_CTX_new(SSLv23_server_method());
  if (server->ctx == NULL)
  {
    g_set_error(err, XR_SERVER_ERROR, XR_SERVER_ERROR_FAILED, "ssl context creation failed: %s", ERR_reason_error_string(ERR_get_error()));
    goto err1;
  }

  if (server->secure)
  {
    if (!SSL_CTX_use_certificate_file(server->ctx, cert, SSL_FILETYPE_PEM) ||
        !SSL_CTX_use_PrivateKey_file(server->ctx, cert, SSL_FILETYPE_PEM) ||
        !SSL_CTX_check_private_key(server->ctx))
    {
      g_set_error(err, XR_SERVER_ERROR, XR_SERVER_ERROR_FAILED, "ssl cert load failed: %s", ERR_reason_error_string(ERR_get_error()));
      goto err2;
    }
  }

  server->pool = g_thread_pool_new((GFunc)_xr_server_connection_thread, server, threads, TRUE, &local_err);
  if (local_err)
  {
    g_set_error(err, XR_SERVER_ERROR, XR_SERVER_ERROR_FAILED, "thread pool creation failed: %s", local_err->message);
    g_clear_error(&local_err);
    goto err2;
  }

  return server;

 err2:
  SSL_CTX_free(server->ctx);
  server->ctx = NULL;
 err1:
  g_free(server);
  return NULL;
}

int xr_server_bind(xr_server* server, const char* port, GError** err)
{
  xr_trace(XR_DEBUG_SERVER_TRACE, "(server=%p, port=%s, err=%p)", server, port, err);
  g_assert(server != NULL);
  g_assert(port != NULL);
  g_return_val_if_fail (err == NULL || *err == NULL, -1);

  server->bio_in = BIO_new_accept((char*)port);
  if (server->bio_in == NULL)
  {
    g_set_error(err, XR_SERVER_ERROR, XR_SERVER_ERROR_FAILED, "accept bio creation failed: %s", xr_get_bio_error_string());
    goto err1;
  }

  BIO_set_bind_mode(server->bio_in, BIO_BIND_REUSEADDR);

  if (server->secure)
  {
    SSL* ssl;
    server->bio_ssl = BIO_new_ssl(server->ctx, 0);
    BIO_get_ssl(server->bio_ssl, &ssl);
    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
    BIO_set_accept_bios(server->bio_in, server->bio_ssl);
  }

  if (BIO_do_accept(server->bio_in) <= 0)
  {
    g_set_error(err, XR_SERVER_ERROR, XR_SERVER_ERROR_FAILED, "%s", xr_get_bio_error_string());
    goto err2;
  }

  server->sock = -1;
  BIO_get_fd(server->bio_in, &server->sock);
  xr_set_nodelay(server->bio_in);

  return 0;
 err2:
  BIO_free_all(server->bio_in);
  server->bio_in = NULL;
 err1:
  return -1;
}

void xr_server_free(xr_server* server)
{
  xr_trace(XR_DEBUG_SERVER_TRACE, "(server=%p)", server);
  g_assert(server != NULL);
  g_thread_pool_free(server->pool, TRUE, TRUE);
  BIO_free_all(server->bio_in);
  SSL_CTX_free(server->ctx);
  g_slist_free(server->servlet_types);
  g_free(server);
}

GQuark xr_server_error_quark()
{
  static GQuark quark;
  return quark ? quark : (quark = g_quark_from_static_string("xr_server_error"));
}

/* simple server setup function */

static xr_server* server = NULL;
static void _sh(int signum)
{
  xr_server_stop(server);
}

gboolean xr_server_simple(const char* cert, int threads, const char* bind, xr_servlet_def** servlets, GError** err)
{
  if (!g_thread_supported())
    g_thread_init(NULL);

  g_assert(server == NULL);

#ifndef WIN32
  struct sigaction act;
  act.sa_handler = _sh;
  act.sa_flags = SA_RESTART;
  sigemptyset(&act.sa_mask);
  if (sigaction(SIGINT, &act, NULL) < 0
   || sigaction(SIGHUP, &act, NULL) < 0
   || sigaction(SIGTERM, &act, NULL) < 0)
    return FALSE;
#endif

  server = xr_server_new(cert, threads, err);
  if (server == NULL)
    return FALSE;

  if (xr_server_bind(server, bind, err) < 0)
  {
    xr_server_free(server);
    return FALSE;
  }

  if (servlets)
  {
    while (*servlets)
    {
      xr_server_register_servlet(server, *servlets);
      servlets++;
    }
  }

  if (xr_server_run(server, err) < 0)
  {
    xr_server_free(server);
    return FALSE;
  }

  xr_server_free(server);
  return TRUE;
}

/* GSource stuff for integration with glib mainloop */

typedef struct _xr_server_watch xr_server_watch;
struct _xr_server_watch
{
  GSource       source;
  GPollFD       pollfd;
  xr_server*    server;
};

static gboolean _xr_swatch_prepare(GSource *source, gint *timeout)
{
  xr_server_watch *watch = (xr_server_watch*)source;
  *timeout = -1;
  return FALSE;
}

static gboolean _xr_swatch_check(GSource *source)
{
  xr_server_watch *watch = (xr_server_watch*)source;
  return watch->pollfd.revents & G_IO_IN;
}

static gboolean _xr_swatch_dispatch(GSource *source, GSourceFunc callback, gpointer user_data)
{
  xr_server_watch *watch = (xr_server_watch*)source;
  _xr_server_accept_connection(watch->server, NULL);
  return TRUE;
}

static void _xr_swatch_finalize(GSource *source)
{
  xr_server_watch *watch = (xr_server_watch*)source;
}

static GSourceFuncs _xr_server_watch_funcs = {
  .prepare = _xr_swatch_prepare,
  .check = _xr_swatch_check,
  .dispatch = _xr_swatch_dispatch,
  .finalize = _xr_swatch_finalize
};

GSource* xr_server_source(xr_server* server)
{
  GSource *source;
  xr_server_watch *watch;

  source = g_source_new(&_xr_server_watch_funcs, sizeof(xr_server_watch));
  watch = (xr_server_watch*)source;

  watch->server = server;
  watch->pollfd.fd = server->sock;
  watch->pollfd.events = G_IO_IN;

  g_source_add_poll(source, &watch->pollfd);
  xr_trace(XR_DEBUG_SERVER_TRACE, "(server=%p) = %p", server, source);
  return source;
}
