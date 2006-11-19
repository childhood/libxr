#include <stdlib.h>
#include <regex.h>

#ifndef WIN32
  #include <signal.h>
#endif

#include "xr-client.h"
#include "xr-http.h"
#include "xr-utils.h"

struct _xr_client_conn
{
  SSL_CTX* ctx;
  SSL* ssl;
  BIO* bio;

  char* resource;
  char* host;
  int secure;

  int is_open;

  int errcode;    /* this must be > 0 for errors */
  char* errmsg;   /* Non-NULL on error. */
};

static void _xr_client_set_error(xr_client_conn* conn, int code, char* msg)
{
  g_assert(conn != NULL);

  conn->errcode = code;
  g_free(conn->errmsg);
  conn->errmsg = g_strdup(msg);
}

xr_client_conn* xr_client_new()
{
  xr_ssl_init();
  xr_client_conn* conn = g_new0(xr_client_conn, 1);
  conn->ctx = SSL_CTX_new(SSLv3_client_method());
  //XXX: setup certificates
  if (conn->ctx == NULL)
  {
    g_free(conn);
    return NULL;
  }
  return conn;
}

int xr_client_get_error_code(xr_client_conn* conn)
{
  g_assert(conn != NULL);
  return conn->errcode;
}

char* xr_client_get_error_message(xr_client_conn* conn)
{
  g_assert(conn != NULL);
  return conn->errmsg;
}

void xr_client_reset_error(xr_client_conn* conn)
{
  _xr_client_set_error(conn, 0, NULL);
}

static int _parse_uri(const char* uri, int* secure, char** host, char** resource)
{
  g_assert(uri != NULL);
  g_assert(secure != NULL);
  g_assert(host != NULL);
  g_assert(resource != NULL);

  regex_t r;
  regmatch_t m[7];
  gint rs;
  if ((rs = regcomp(&r, "^([a-z]+)://([a-z0-9.-]+(:([0-9]+))?)(/.+)?$", REG_EXTENDED|REG_ICASE)))
    return -1;
  rs = regexec(&r, uri, 7, m, 0);
  regfree(&r);
  if (rs != 0)
    return -1;
  
  char* schema = g_strndup(uri+m[1].rm_so, m[1].rm_eo-m[1].rm_so);

  if (!strcasecmp("eees", schema))
    *secure = 1;
  else if (!strcasecmp("eee", schema))
    *secure = 0;
  else if (!strcasecmp("http", schema))
    *secure = 0;
  else if (!strcasecmp("https", schema))
    *secure = 1;
  else
  {
    g_free(schema);
    return -1;
  }
  g_free(schema);
  
  *host = g_strndup(uri+m[2].rm_so, m[2].rm_eo-m[2].rm_so);
  if (m[5].rm_eo-m[5].rm_so == 0)
    *resource = g_strdup("/RPC2");
  else
    *resource = g_strndup(uri+m[5].rm_so, m[5].rm_eo-m[5].rm_so);
  
  return 0;
}

int xr_client_open(xr_client_conn* conn, char* uri)
{
  g_assert(conn != NULL);
  g_assert(uri != NULL);
  g_assert(!conn->is_open);

  // parse URI format: http://host:8080/RES
  g_free(conn->host);
  g_free(conn->resource);
  conn->host = NULL;
  conn->resource = NULL;
  if (_parse_uri(uri, &conn->secure, &conn->host, &conn->resource))
  {
    fprintf(stderr, "Invalid URI format (%s).\n", uri);
    return -1;
  }

  if (conn->secure)
  {
    conn->bio = BIO_new_ssl_connect(conn->ctx);
    BIO_get_ssl(conn->bio, &conn->ssl);
    SSL_set_mode(conn->ssl, SSL_MODE_AUTO_RETRY);
    BIO_set_conn_hostname(conn->bio, conn->host);
  }
  else
  {
    conn->bio = BIO_new_connect(conn->host);
  }

  if (BIO_do_connect(conn->bio) <= 0)
  {
    fprintf(stderr, "Error connecting to server\n");
    ERR_print_errors_fp(stderr);
    BIO_free_all(conn->bio);
    return -1;
  }

  xr_set_nodelay(conn->bio);

  if (conn->secure)
  {
    if (BIO_do_handshake(conn->bio) <= 0)
    {
      fprintf(stderr, "Error establishing SSL connection\n");
      ERR_print_errors_fp(stderr);
      BIO_free_all(conn->bio);
      return -1;
    }
  }
  conn->is_open = 1;
  return 0;
}

void xr_client_close(xr_client_conn* conn)
{
  g_assert(conn != NULL);
  g_assert(conn->is_open);

  if (conn->secure)
    BIO_ssl_shutdown(conn->bio);
  BIO_free_all(conn->bio);
  conn->ssl = NULL;
  conn->bio = NULL;
  conn->is_open = 0;
}

int xr_client_call(xr_client_conn* conn, xr_call* call)
{
  g_assert(conn != NULL);
  g_assert(conn->is_open);
  g_assert(call != NULL);
  g_assert(conn->errcode == 0);

  char* buffer;
  int length, rs;

  xr_http* http = xr_http_new(conn->bio);
  xr_call_serialize_request(call, &buffer, &length);
  xr_http_setup_request(http, "POST", conn->resource, conn->host);
  rs = xr_http_send(http, XR_HTTP_REQUEST, buffer, length);
  xr_call_free_buffer(buffer);
  if (rs < 0)
    goto err;
  
  if (xr_http_receive(http, XR_HTTP_RESPONSE, &buffer, &length) < 0)
    goto err;
  rs = xr_call_unserialize_response(call, buffer, length);
  g_free(buffer);
  xr_http_free(http);

  if (rs)
  {
    _xr_client_set_error(conn, xr_call_get_error_code(call), xr_call_get_error_message(call));
    return 1;
  }
  xr_client_reset_error(conn);

  return 0;
 err:
  xr_http_free(http);
  return -1;
}

int xr_client_call_ex(xr_client_conn* conn, xr_call* call, xr_demarchalizer_t dem, void** retval)
{
  int rs = xr_client_call(conn, call);
  if (rs == 0)
  {
    if (dem(xr_call_get_retval(call), retval) < 0)
      _xr_client_set_error(conn, 100, "Retval demarchalziation failed!");
  }
  return rs;
}

void xr_client_free(xr_client_conn* conn)
{
  g_assert(conn != NULL);
  if (conn->is_open)
    xr_client_close(conn);
  g_free(conn->host);
  g_free(conn->resource);
  SSL_CTX_free(conn->ctx);
  g_free(conn);
}
