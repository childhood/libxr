#include <stdlib.h>
#include <regex.h>
#include <sys/select.h>

#include "xr-server.h"
#include "xr-http.h"
#include "xr-utils.h"

//#define DEBUG

/* server */

struct _xr_servlet
{
  void* priv;
  BIO* bio;
  int running;
  char* name;
  char* resource;
  xr_servlet_def* def;
  xr_call* call;
};

struct _xr_server
{
  SSL_CTX* ctx;
  BIO* bio_in;
  int sock;
  BIO* bio_ssl;
  GThreadPool* pool;
  int secure;
  int running;
  GSList* servlet_types;
};

void* xr_servlet_get_priv(xr_servlet* servlet)
{
  g_assert(servlet != NULL);
  return servlet->priv;
}

xr_servlet_def* _find_servlet_def(xr_server* server, char* name)
{
  GSList* i;
  for (i=server->servlet_types; i; i=i->next)
  {
    xr_servlet_def* sd = i->data;
    if (!strcasecmp(sd->name, name))
      return sd;
  }
  return NULL;
}

xr_servlet_method_def* _find_servlet_method_def(xr_servlet* servlet, char* name)
{
  g_assert(servlet != NULL);
  g_assert(servlet->def != NULL);

  int i;
  for (i = 0; i < servlet->def->methods_count; i++)
  {
    if (!strcmp(servlet->def->methods[i].name, name))
      return servlet->def->methods + i;
  }
  return NULL;
}

static int _xr_server_servlet_method_call(xr_server* server, xr_servlet* servlet, xr_call* call)
{
  g_assert(server != NULL);
  g_assert(servlet != NULL);
  g_assert(call != NULL);

  int retval = -1;
  // check resource for this call
  if (servlet->resource == NULL)
  {
    xr_call_set_error(call, 100, "Unknown resource!");
    goto done;
  }
  servlet->call = call;
  // initialize servlet if necessary
  if (servlet->name == NULL)
  {
    servlet->def = _find_servlet_def(server, servlet->resource+1);
    if (servlet->def == NULL)
    {
      xr_call_set_error(call, 100, "Unknown servlet.");
      goto done;
    }
    servlet->name = g_strdup(servlet->resource);
    if (servlet->def->size > 0)
      servlet->priv = g_malloc0(servlet->def->size);
    if (servlet->def->init(servlet) < 0)
    {
      xr_call_set_error(call, 100, "Servlet initialization failed.");
      g_free(servlet->name);
      g_free(servlet->priv);
      servlet->name = NULL;
      servlet->priv = NULL;
      goto done;
    }
  }

  // find method and perform a call
  xr_servlet_method_def* method = _find_servlet_method_def(servlet, xr_call_get_method(call));
  if (method == NULL)
  {
    char* msg = g_strdup_printf("Method %s not found in %s servlet.", xr_call_get_method(call), servlet->name);
    xr_call_set_error(call, 100, msg);
    g_free(msg);
    goto done;
  }

#ifdef DEBUG
  printf("Pre method call:\n");
  xr_call_dump(call, 0);
#endif

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
#ifdef DEBUG
  printf("Post method call:\n");
  xr_call_dump(call, 0);
#endif
  servlet->call = NULL;
  return retval;
}

static int _xr_server_servlet_call(xr_server* server, xr_servlet* servlet)
{
  g_assert(server != NULL);
  g_assert(servlet != NULL);

  char* buffer;
  int length, rs, retval = -1;

  xr_http* http = xr_http_new(servlet->bio);
  if (xr_http_receive(http, XR_HTTP_REQUEST, &buffer, &length) < 0)
    goto err;

  g_free(servlet->resource);
  servlet->resource = xr_http_get_resource(http);

  xr_call* call = xr_call_new(NULL);
  rs = xr_call_unserialize_request(call, buffer, length);
  g_free(buffer);
  if (rs)
    xr_call_set_error(call, 100, "Unserialize request failure.");
  else
    _xr_server_servlet_method_call(server, servlet, call);
  xr_call_serialize_response(call, &buffer, &length);
  xr_call_free(call);

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

static int _xr_server_servlet_run(xr_servlet* servlet, xr_server* server)
{
  g_assert(server != NULL);
  g_assert(servlet != NULL);

  if (server->secure)
  {
    if (BIO_do_handshake(servlet->bio) <= 0)
      goto done;
  }

  servlet->running = 1;
  while (servlet->running)
  {
    if (_xr_server_servlet_call(server, servlet) < 0)
      goto done;
  }

 done:
  if (servlet->name != NULL)
  {
    g_assert(servlet->def != NULL);
    servlet->def->fini(servlet);
    g_free(servlet->name);
    g_free(servlet->priv);
    servlet->name = NULL;
    servlet->priv = NULL;
  }
  BIO_free_all(servlet->bio);
  g_free(servlet);
  return 0;
}

void xr_server_stop(xr_server* server)
{
  g_assert(server != NULL);
  server->running = 0;
}

static int _xr_server_accept_connection(xr_server* server, GError** err)
{
  GError* local_err = NULL;

  g_assert(server != NULL);
  g_return_val_if_fail(err == NULL || *err == NULL, -1);

  if (BIO_do_accept(server->bio_in) <= 0)
  {
    g_set_error(err, XR_SERVER_ERROR, XR_SERVER_ERROR_FAILED, "accept failed: %s", xr_get_bio_error_string());
    return -1;
  }

  // new connection accepted
  xr_servlet* servlet = g_new0(xr_servlet, 1);
  servlet->bio = BIO_pop(server->bio_in);

  g_thread_pool_push(server->pool, servlet, &local_err);
  if (local_err)
  {
    // cleanup
    BIO_free_all(servlet->bio);
    g_free(servlet);
    if (!g_error_matches(local_err, G_THREAD_ERROR, G_THREAD_ERROR_AGAIN))
    {
      g_set_error(err, XR_SERVER_ERROR, XR_SERVER_ERROR_FAILED, "thread push failed: %s", local_err->message);
      g_clear_error(&local_err);
      return -1;
    }
    // reject connection if thread pool is full
    g_clear_error(&local_err);
  }
  return 0;
}

int xr_server_run(xr_server* server, GError** err)
{
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
  
  server->running = 1;
  while (server->running)
  {
    set = setcopy;
    tv = tvcopy;
    int rs = select(maxfd, &set, NULL, NULL, &tv);
    if (rs < 0)
    {
      if (errno == EINTR)
        return 0;
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
  g_assert(server != NULL);
  g_assert(servlet != NULL);
  if (!_find_servlet_def(server, servlet->name))
    server->servlet_types = g_slist_append(server->servlet_types, servlet);
  return 0;
}

xr_server* xr_server_new(const char* cert, int threads, GError** err)
{
  GError* local_err = NULL;

  g_assert(threads > 0 && threads < 1000);
  g_return_val_if_fail (err == NULL || *err == NULL, NULL);

  xr_ssl_init();

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

  server->pool = g_thread_pool_new((GFunc)_xr_server_servlet_run, server, threads, TRUE, &local_err);
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
  return source;
}
