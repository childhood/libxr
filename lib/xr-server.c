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
#include <time.h>
#include <string.h>

#include "xr-server.h"
#include "xr-http.h"
#include "xr-utils.h"

/* server */

struct _xr_server
{
  GSocketService* service;
  gboolean secure;
  GTlsCertificate *cert;
  GSList* servlet_types;
  GHashTable* sessions;
  GStaticRWLock sessions_lock;
  GThread* sessions_cleaner;
  GMainLoop* loop;
  time_t current_time;
};

/* servlet API */

typedef struct _xr_server_conn xr_server_conn;
struct _xr_server_conn
{
  GSocketConnection* conn;
  GIOStream* tls_conn;
  xr_http* http;
  GPtrArray* servlets;
  gboolean running;
};

struct _xr_servlet
{
  void* priv;
  xr_servlet_def* def;
  xr_call* call;
  xr_server_conn* conn;
  time_t last_used;
  GMutex* call_mutex; /* held during call */
};

static void xr_servlet_free(xr_servlet* servlet)
{
  if (servlet == NULL)
    return;

  g_free(servlet->priv);
  g_mutex_free(servlet->call_mutex);
  memset(servlet, 0, sizeof(*servlet));
  g_free(servlet);
}

static void xr_servlet_free_fini(xr_servlet* servlet)
{
  if (servlet && servlet->def && servlet->def->fini)
    servlet->def->fini(servlet);

  xr_servlet_free(servlet);
}

static xr_servlet* xr_servlet_new(xr_servlet_def* def, xr_server_conn* conn)
{
  xr_servlet* s = g_new0(xr_servlet, 1);
  if (def->size > 0)
    s->priv = g_malloc0(def->size);
  s->def = def;
  s->conn = conn;
  s->call_mutex = g_mutex_new();

  if (s->def->init && !s->def->init(s))
  {
    xr_servlet_free(s);
    return NULL;
  }

  return s;
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
  g_return_val_if_fail(servlet != NULL, NULL);

  return servlet->priv;
}

xr_http* xr_servlet_get_http(xr_servlet* servlet)
{
  g_return_val_if_fail(servlet != NULL, NULL);
  g_return_val_if_fail(servlet->conn != NULL, NULL);
  g_return_val_if_fail(servlet->conn->http != NULL, NULL);

  return servlet->conn->http;
}

char* xr_servlet_get_client_ip(xr_servlet* servlet)
{
  g_return_val_if_fail(servlet != NULL, NULL);
  g_return_val_if_fail(servlet->conn != NULL, NULL);

  GSocketAddress* addr = g_socket_connection_get_remote_address(servlet->conn->conn, NULL);
  if (addr)
  {
    GInetAddress* inet_addr = g_inet_socket_address_get_address(G_INET_SOCKET_ADDRESS(addr));

    char* str = g_inet_address_to_string(inet_addr);

    g_object_unref(addr);

    return str;
  }

  return  NULL;
}

static xr_servlet_def* _find_servlet_def(xr_server* server, char* name)
{
  GSList* i;

  for (i = server->servlet_types; i; i = i->next)
  {
    xr_servlet_def* def = i->data;
    if (!g_ascii_strcasecmp(def->name, name))
      return def;
  }

  return NULL;
}

static xr_servlet_method_def* _find_servlet_method_def(xr_servlet* servlet, const char* name)
{
  int i;

  g_return_val_if_fail(servlet != NULL, NULL);
  g_return_val_if_fail(servlet->def != NULL, NULL);

  for (i = 0; i < servlet->def->methods_count; i++)
    if (!strcmp(servlet->def->methods[i].name, name))
      return servlet->def->methods + i;

  return NULL;
}

static gboolean _xr_servlet_do_call(xr_servlet* servlet, xr_call* call)
{
  xr_servlet_method_def* method;
  gboolean retval = FALSE;

  servlet->call = call;

  /* find method and perform a call */
  method = _find_servlet_method_def(servlet, xr_call_get_method(call));
  if (method)
  {
    if (servlet->def->pre_call)
    {
      if (!servlet->def->pre_call(servlet, call))
      {
        // FALSE returned
        if (xr_call_get_retval(call) == NULL && !xr_call_get_error_code(call))
          xr_call_set_error(call, -1, "Pre-call did not returned value or set error.");
        goto out;
      }
    }

    retval = method->cb(servlet, call);

    if (servlet->def->post_call)
      servlet->def->post_call(servlet, call);
  }
  else if (servlet->def->fallback)
  {
    if (servlet->def->fallback(servlet, call))
    {
      // call should be handled
      if (xr_call_get_retval(call) == NULL && !xr_call_get_error_code(call))
        xr_call_set_error(call, -1, "Fallback did not returned value or set error.");
    }
    else
      xr_call_set_error(call, -1, "Method %s not found in %s servlet.", xr_call_get_method(call), servlet->def->name);
  }
  else
    xr_call_set_error(call, -1, "Method %s not found in %s servlet.", xr_call_get_method(call), servlet->def->name);

out:
  servlet->call = NULL;
  return retval;
}

static gboolean _maybe_remove_servlet(gpointer key, gpointer value, gpointer user_data)
{
  xr_servlet* servlet = value;
  xr_server* server = user_data;

  if (g_mutex_trylock(servlet->call_mutex))
  {
    g_mutex_unlock(servlet->call_mutex); /* this is ok, as nobody else is going to take this lock during remove */
    gboolean remove = (servlet->last_used + 60 < server->current_time || servlet->last_used > server->current_time);
    return remove;
  }

  /* servlet is locked, can't remove */
  return FALSE;
}

static gpointer sessions_cleaner_func(xr_server* server)
{
  while (g_socket_service_is_active(G_SOCKET_SERVICE(server->service)))
  {
    server->current_time = time(NULL);
    g_static_rw_lock_writer_lock(&server->sessions_lock);
    g_hash_table_foreach_remove(server->sessions, _maybe_remove_servlet, server);
    g_static_rw_lock_writer_unlock(&server->sessions_lock);

    g_usleep(1000000);
  }

  return NULL;
}

static gboolean _xr_server_servlet_method_call(xr_server* server, xr_server_conn* conn, xr_call* call)
{
  xr_servlet* servlet = NULL;
  xr_servlet* cur_servlet;
  char *servlet_name;

  g_return_val_if_fail(server != NULL, FALSE);
  g_return_val_if_fail(conn != NULL, FALSE);
  g_return_val_if_fail(call != NULL, FALSE);

  /* session mode */
  const char* session_id = xr_http_get_header(conn->http, "X-SESSION-ID");
  if (session_id && xr_http_get_header(conn->http, "X-SESSION-USE"))
  {
    /* lookup servlet in session and try to lock it for call, if call is in
       progress try again later (1ms) */
again:
    g_static_rw_lock_reader_lock(&server->sessions_lock);
    servlet = g_hash_table_lookup(server->sessions, session_id);
    if (servlet)
    {
      if (!g_mutex_trylock(servlet->call_mutex))
      {
        g_static_rw_lock_reader_unlock(&server->sessions_lock);
        g_usleep(10000);
        goto again;
      }
    }
    g_static_rw_lock_reader_unlock(&server->sessions_lock);

    /* if servlet does not exist */
    if (servlet == NULL)
    {
      xr_servlet_def* def;
      servlet_name = xr_call_get_servlet_name(call, xr_http_get_resource(conn->http) + 1);
      if (servlet_name == NULL)
      {
        xr_call_set_error(call, -1, "Undefined servlet name.");
        return FALSE;
      }

      def = _find_servlet_def(server, servlet_name);
      if (def == NULL)
      {
        xr_call_set_error(call, -1, "Unknown servlet %s.", servlet_name);
        g_free(servlet_name);
        return FALSE;
      }
      g_free(servlet_name);

      servlet = xr_servlet_new(def, conn);
      if (servlet == NULL)
      {
        xr_call_set_error(call, -1, "Servlet initialization failed.");
        return FALSE;
      }

      g_static_rw_lock_writer_lock(&server->sessions_lock);

      /* user might have used same session ID to create servlet in other thread, check for
         this situation */
      cur_servlet = g_hash_table_lookup(server->sessions, session_id); 
      if (cur_servlet)
      {
        xr_servlet_free_fini(servlet);
        servlet = cur_servlet;
      }
      else
      {
        g_hash_table_replace(server->sessions, g_strdup(session_id), servlet);
      }

      /* this will block sessions ht access until servlet call completes, if
         servlet was found in other thread, which should be rare occurrance */
      if (!g_mutex_trylock(servlet->call_mutex))
      {
        g_static_rw_lock_writer_unlock(&server->sessions_lock);
        g_usleep(10000);
        goto again;
      }

      g_static_rw_lock_writer_unlock(&server->sessions_lock);
    }

    servlet->conn = conn;
    servlet->last_used = time(NULL);
    gboolean rs = _xr_servlet_do_call(servlet, call);

    g_mutex_unlock(servlet->call_mutex);

    return rs;
  }

  /* persistent mode */

  /* get xr_servlet object for current connection and given servlet name */
  servlet_name = xr_call_get_servlet_name(call, xr_http_get_resource(conn->http) + 1);
  if (servlet_name == NULL)
  {
    xr_call_set_error(call, -1, "Undefined servlet name.");
    return FALSE;
  }

  servlet = xr_server_conn_find_servlet(conn, servlet_name);
  if (servlet == NULL)
  {
    xr_servlet_def* def = _find_servlet_def(server, servlet_name);
    if (def == NULL)
    {
      xr_call_set_error(call, -1, "Unknown servlet %s.", servlet_name);
      g_free(servlet_name);
      return FALSE;
    }

    servlet = xr_servlet_new(def, conn);
    if (servlet == NULL)
    {
      xr_call_set_error(call, -1, "Servlet initialization failed.");
      g_free(servlet_name);
      return FALSE;
    }

    g_ptr_array_add(conn->servlets, servlet);
  }

  g_free(servlet_name);
  
  return _xr_servlet_do_call(servlet, call);
}

static gboolean _xr_server_serve_download(xr_server* server, xr_server_conn* conn)
{
  guint i;
  GSList* iter;

  /* for each available servlet type, check if it has download hook */
  for (iter = server->servlet_types; iter; iter = iter->next)
  {
    xr_servlet_def* def = iter->data;

    if (def->download)
    {
      xr_servlet* servlet = NULL;

      /* check if servlet is instantiated */
      for (i = 0; i < conn->servlets->len; i++)
      {
        xr_servlet* existing_servlet = g_ptr_array_index(conn->servlets, i);
        if (existing_servlet->def == def)
          servlet = existing_servlet;
      }

      /* it's not, instantiate it now */
      if (servlet == NULL)
      {
        servlet = xr_servlet_new(def, conn);
        g_ptr_array_add(conn->servlets, servlet);
      }

      if (def->download(servlet))
        return xr_http_is_ready(conn->http);
    }
  }

  xr_http_setup_response(conn->http, 501);
  xr_http_set_header(conn->http, "Content-Type", "text/plain");
  if (!xr_http_write_all(conn->http, "Download hook is not implemented.", -1, NULL))
    return FALSE;

  return TRUE;
}

static gboolean _xr_server_serve_upload(xr_server* server, xr_server_conn* conn)
{
  guint i;
  GSList* iter;

  /* for each available servlet type, check if it has upload hook */
  for (iter = server->servlet_types; iter; iter = iter->next)
  {
    xr_servlet_def* def = iter->data;

    if (def->upload)
    {
      xr_servlet* servlet = NULL;

      /* check if servlet is instantiated */
      for (i = 0; i < conn->servlets->len; i++)
      {
        xr_servlet* existing_servlet = g_ptr_array_index(conn->servlets, i);
        if (existing_servlet->def == def)
          servlet = existing_servlet;
      }

      /* it's not, instantiate it now */
      if (servlet == NULL)
      {
        servlet = xr_servlet_new(def, conn);
        g_ptr_array_add(conn->servlets, servlet);
      }

      if (def->upload(servlet))
        return xr_http_is_ready(conn->http);
    }
  }

  char buf[4096];
  while (xr_http_read(conn->http, buf, sizeof(buf), NULL) > 0);
  xr_http_setup_response(conn->http, 501);
  xr_http_set_header(conn->http, "Content-Type", "text/plain");
  if (!xr_http_write_all(conn->http, "Upload hook is not implemented.", -1, NULL))
    return FALSE;

  return TRUE;
}

static int _ctype_to_transport(const char* ctype)
{
  if (ctype == NULL)
    return -1;
  if (!g_ascii_strncasecmp(ctype, "text/xml", 8))
    return XR_CALL_XML_RPC;
#ifdef XR_JSON_ENABLED
  if (!g_ascii_strncasecmp(ctype, "text/json", 9))
    return XR_CALL_JSON_RPC;
#endif
  return -1;
}

static gboolean _xr_server_serve_request(xr_server* server, xr_server_conn* conn)
{
  const char* method;
  int version;

  xr_trace(XR_DEBUG_SERVER_TRACE, "(server=%p, conn=%p)", server, conn);

  g_return_val_if_fail(server != NULL, FALSE);
  g_return_val_if_fail(conn != NULL, FALSE);

  /* receive HTTP request */
  if (!xr_http_read_header(conn->http, NULL))
    return FALSE;

  /* check if some dumb bunny sent us wrong message type */
  if (xr_http_get_message_type(conn->http) != XR_HTTP_REQUEST)
    return FALSE;

  method = xr_http_get_method(conn->http);
  if (method == NULL)
    return FALSE;

  version = xr_http_get_version(conn->http);

  if (!strcmp(method, "GET"))
    return _xr_server_serve_download(server, conn) && (version == 1);
  else if (!strcmp(method, "POST"))
  {
    int transport = _ctype_to_transport(xr_http_get_header(conn->http, "Content-Type"));

    if (transport >= 0)
    {
      xr_call* call;
      GString* request;
      char* buffer;
      int length;
      gboolean rs;

      request = xr_http_read_all(conn->http, NULL);
      if (request == NULL)
        return FALSE;

      /* parse request data into xr_call */
      call = xr_call_new(NULL);
      xr_call_set_transport(call, transport);

      rs = xr_call_unserialize_request(call, request->str, request->len);
      g_string_free(request, TRUE);

      /* run call */
      if (!rs)
        xr_call_set_error(call, -1, "Unserialize request failure.");
      else
        _xr_server_servlet_method_call(server, conn, call);

      /* generate response data from xr_call */
      xr_call_serialize_response(call, &buffer, &length);
      if (xr_debug_enabled & XR_DEBUG_CALL)
        xr_call_dump(call, 0);

      /* send HTTP response */
      xr_http_setup_response(conn->http, 200);
      xr_http_set_message_length(conn->http, length);
      rs = xr_http_write_all(conn->http, buffer, length, NULL);
      xr_call_free_buffer(call, buffer);
      xr_call_free(call);

      return rs && (version == 1);
    }
    else
      return _xr_server_serve_upload(server, conn) && (version == 1);
  }
  else
    return FALSE;

  return TRUE;
}

void xr_server_stop(xr_server* server)
{
  xr_trace(XR_DEBUG_SERVER_TRACE, "(server=%p)", server);

  g_return_if_fail(server != NULL);

  g_socket_service_stop(G_SOCKET_SERVICE(server->service));
  g_main_loop_quit(server->loop);
}

gboolean _xr_server_service_run(GThreadedSocketService *service, GSocketConnection *connection, GObject *source_object, gpointer user_data)
{
  GError* local_err = NULL;
  xr_server* server = user_data;
  xr_server_conn* conn;

  //xr_trace(XR_DEBUG_SERVER_TRACE, "(conn=%p, server=%p)", conn, server);

  xr_set_nodelay(g_socket_connection_get_socket(connection));

  // new connection accepted
  conn = g_new0(xr_server_conn, 1);
  conn->servlets = g_ptr_array_sized_new(3);
  conn->running = TRUE;

  // setup TLS
  if (server->secure)
  {
    conn->tls_conn = g_tls_server_connection_new(G_IO_STREAM(connection), server->cert, &local_err);
    if (local_err)
    {
      g_error_free(local_err);
      goto out;
    }

    //g_object_set(conn->conn, "authentication-mode", test->auth_mode, NULL);
    //g_signal_connect(conn->conn, "accept-certificate", G_CALLBACK(on_accept_certificate), server);

    conn->http = xr_http_new(conn->tls_conn);
  }
  else
  {
    conn->http = xr_http_new(G_IO_STREAM(connection));
  }

  conn->conn = connection;

  while (conn->running)
  {
    if (!_xr_server_serve_request(server, conn))
      break;
  }

out:
  xr_http_free(conn->http);
  if (conn->tls_conn)
    g_object_unref(conn->tls_conn);
  g_ptr_array_foreach(conn->servlets, (GFunc)xr_servlet_free_fini, NULL);
  g_ptr_array_free(conn->servlets, TRUE);
  memset(conn, 0, sizeof(*conn));
  g_free(conn);

  return FALSE;
}

gboolean xr_server_run(xr_server* server, GError** err)
{
  GError* local_err = NULL;

  xr_trace(XR_DEBUG_SERVER_TRACE, "(server=%p, err=%p)", server, err);

  g_return_val_if_fail(server != NULL, FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);

  g_socket_service_start(G_SOCKET_SERVICE(server->service));

  server->loop = g_main_loop_new(NULL, TRUE);
  g_main_loop_run(server->loop);

  return TRUE;
}

gboolean xr_server_register_servlet(xr_server* server, xr_servlet_def* servlet)
{
  xr_trace(XR_DEBUG_SERVER_TRACE, "(server=%p, servlet=%p)", server, servlet);

  g_return_val_if_fail(server != NULL, FALSE);
  g_return_val_if_fail(servlet != NULL, FALSE);

  if (_find_servlet_def(server, servlet->name))
    return FALSE;

  server->servlet_types = g_slist_append(server->servlet_types, servlet);
  return TRUE;
}

xr_server* xr_server_new(const char* cert, int threads, GError** err)
{
  xr_trace(XR_DEBUG_SERVER_TRACE, "(cert=%s, threads=%d, err=%p)", cert, threads, err);
  GError* local_err = NULL;

  g_return_val_if_fail(threads > 0 && threads < 1000, NULL);
  g_return_val_if_fail (err == NULL || *err == NULL, NULL);

  xr_init();

  xr_server* server = g_new0(xr_server, 1);
  server->secure = !!cert;
  server->service = g_threaded_socket_service_new(threads);
  g_signal_connect(server->service, "run", (GCallback)_xr_server_service_run, server);

  if (cert)
  {
    server->cert = g_tls_certificate_new_from_file(cert, &local_err);
    if (local_err)
    {
      g_propagate_prefixed_error(err, local_err, "Certificate load failed: ");
      goto err0;
    }
  }

  server->sessions = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)xr_servlet_free_fini);
  g_static_rw_lock_init(&server->sessions_lock);
  server->sessions_cleaner = g_thread_create((GThreadFunc)sessions_cleaner_func, server, TRUE, NULL);
  if (server->sessions_cleaner == NULL)
    goto err1;

  return server;

err1:
  g_hash_table_destroy(server->sessions);
  g_static_rw_lock_free(&server->sessions_lock);
  if (server->cert)
    g_object_unref(server->cert);
err0:
  g_object_unref(server->service);
  g_free(server);
  return NULL;
}

static gboolean _parse_addr(const char* str, char** addr, int* port)
{
  gboolean retval = FALSE;
  GMatchInfo *match_info = NULL;
  GRegex* re = g_regex_new("^((?:\\*)|(?:\\d+\\.\\d+\\.\\d+\\.\\d+)):(\\d+)$", 0, 0, NULL);

  if (g_regex_match(re, str, 0, &match_info))
  {
    *addr = g_match_info_fetch(match_info, 1);
    char* port_str = g_match_info_fetch(match_info, 2);
    *port = atoi(port_str);
    g_free(port_str);
    retval = TRUE;
  }

  if (match_info)
    g_match_info_free(match_info);

  g_regex_unref(re);

  return retval;
}

gboolean xr_server_bind(xr_server* server, const char* bind_addr, GError** err)
{
  GError* local_err = NULL;
  char* addr = NULL;
  int port = 0;

  xr_trace(XR_DEBUG_SERVER_TRACE, "(server=%p, bind_addr=%s, err=%p)", server, bind_addr, err);

  g_return_val_if_fail(server != NULL, FALSE);
  g_return_val_if_fail(bind_addr != NULL, FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
  g_return_val_if_fail(_parse_addr(bind_addr, &addr, &port), FALSE);

  if (addr[0] == '*')
  {
    g_socket_listener_add_inet_port(G_SOCKET_LISTENER(server->service), (guint16)port, NULL, &local_err);
  }
  else
  {
    // build address
    GInetAddress* iaddr = g_inet_address_new_from_string(addr);
    if (!iaddr)
    {
      g_error_new(XR_SERVER_ERROR, XR_SERVER_ERROR_FAILED, "Invalid address: %s", bind_addr);
      g_free(addr);
      return FALSE;
    }
      
    GSocketAddress* isaddr = g_inet_socket_address_new(iaddr, (guint16)port);
    g_socket_listener_add_address(G_SOCKET_LISTENER(server->service), isaddr, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_TCP, NULL, NULL, &local_err);
  }

  g_free(addr);

  if (local_err)
  {
    g_propagate_prefixed_error(err, local_err, "Port listen failed: ");
    return FALSE;
  }

  return TRUE;
}

void xr_server_free(xr_server* server)
{
  xr_trace(XR_DEBUG_SERVER_TRACE, "(server=%p)", server);

  if (server == NULL)
    return;

  if (server->cert)
    g_object_unref(server->cert);
  g_object_unref(server->service);
  g_slist_free(server->servlet_types);
  g_thread_join(server->sessions_cleaner);
  g_hash_table_destroy(server->sessions);
  g_static_rw_lock_free(&server->sessions_lock);
  g_main_loop_unref(server->loop);
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

  g_return_val_if_fail(server == NULL, FALSE);
  g_return_val_if_fail(threads > 0, FALSE);
  g_return_val_if_fail(bind != NULL, FALSE);
  g_return_val_if_fail(servlets != NULL, FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);

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

  if (!xr_server_bind(server, bind, err))
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

  if (!xr_server_run(server, err))
  {
    xr_server_free(server);
    return FALSE;
  }

  xr_server_free(server);
  return TRUE;
}
