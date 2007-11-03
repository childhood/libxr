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

#include <config.h>
#include <stdlib.h>
#ifndef HAVE_GLIB_REGEXP
#include <regex.h>
#endif

#include "xr-client.h"
#include "xr-http.h"
#include "xr-utils.h"

struct _xr_client_conn
{
  SSL_CTX* ctx;
  BIO* bio;
  xr_http* http;

  char* resource;
  char* host;
  int secure;

  int is_open;
};

xr_client_conn* xr_client_new(GError** err)
{
  g_return_val_if_fail(err == NULL || *err == NULL, NULL);

  xr_trace(XR_DEBUG_CLIENT_TRACE, "(err=%p)", err);

  xr_init();

  xr_client_conn* conn = g_new0(xr_client_conn, 1);
  conn->ctx = SSL_CTX_new(SSLv3_client_method());
  //XXX: setup certificates?
  if (conn->ctx == NULL)
  {
    g_free(conn);
    g_set_error(err, XR_CLIENT_ERROR, XR_CLIENT_ERROR_FAILED, "ssl context creation failed: %s", ERR_reason_error_string(ERR_get_error()));
    return NULL;
  }
  return conn;
}

#ifndef HAVE_GLIB_REGEXP

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

#else

G_LOCK_DEFINE_STATIC(regex);

static int _parse_uri(const char* uri, int* secure, char** host, char** resource)
{
  static GRegex* regex = NULL;
  GMatchInfo *match_info = NULL;

  g_assert(uri != NULL);
  g_assert(secure != NULL);
  g_assert(host != NULL);
  g_assert(resource != NULL);

  // precompile regexp
  G_LOCK(regex);
  if (regex == NULL)
  {
    regex = g_regex_new("^([a-z]+)://([a-z0-9.-]+(:([0-9]+))?)(/.+)?$", G_REGEX_CASELESS, 0, NULL);
    g_assert(regex != NULL);
  }
  G_UNLOCK(regex);

  if (!g_regex_match(regex, uri, 0, &match_info))
    return -1;
  
  // check schema
  char* schema = g_match_info_fetch(match_info, 1);
  if (!g_ascii_strcasecmp("eees", schema))
    *secure = 1;
  else if (!g_ascii_strcasecmp("eee", schema))
    *secure = 0;
  else if (!g_ascii_strcasecmp("http", schema))
    *secure = 0;
  else if (!g_ascii_strcasecmp("https", schema))
    *secure = 1;
  else
  {
    g_free(schema);
    g_match_info_free(match_info);
    return -1;
  }
  g_free(schema);
  
  *host = g_match_info_fetch(match_info, 2);
  *resource = g_match_info_fetch(match_info, 5);
  if (*resource == NULL)
    *resource = g_strdup("/RPC2");

  g_match_info_free(match_info);
  return 0;
}

#endif

int xr_client_open(xr_client_conn* conn, const char* uri, GError** err)
{
  g_assert(conn != NULL);
  g_assert(uri != NULL);
  g_assert(!conn->is_open);

  g_return_val_if_fail(err == NULL || *err == NULL, -1);

  xr_trace(XR_DEBUG_CLIENT_TRACE, "(conn=%p, uri=%s)", conn, uri);

  // parse URI format: http://host:8080/RES
  g_free(conn->host);
  g_free(conn->resource);
  conn->host = NULL;
  conn->resource = NULL;
  if (_parse_uri(uri, &conn->secure, &conn->host, &conn->resource))
  {
    g_set_error(err, XR_CLIENT_ERROR, XR_CLIENT_ERROR_FAILED, "invalid URI format: %s", uri);
    return -1;
  }

  if (conn->secure)
  {
    SSL* ssl;

    conn->bio = BIO_new_buffer_ssl_connect(conn->ctx);
    BIO_get_ssl(conn->bio, &ssl);
    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
    BIO_set_conn_hostname(conn->bio, conn->host);
    BIO_set_buffer_size(conn->bio, 2048);
  }
  else
  {
    conn->bio = BIO_new(BIO_f_buffer());
    BIO_push(conn->bio, BIO_new_connect(conn->host));
    BIO_set_buffer_size(conn->bio, 2048);
  }

  if (BIO_do_connect(conn->bio) <= 0)
  {
    g_set_error(err, XR_CLIENT_ERROR, XR_CLIENT_ERROR_FAILED, "BIO_do_connect failed: %s", xr_get_bio_error_string());
    BIO_free_all(conn->bio);
    return -1;
  }

  xr_set_nodelay(conn->bio);

  if (conn->secure)
  {
    if (BIO_do_handshake(conn->bio) <= 0)
    {
      g_set_error(err, XR_CLIENT_ERROR, XR_CLIENT_ERROR_FAILED, "BIO_do_handshake failed: %s", xr_get_bio_error_string());
      BIO_free_all(conn->bio);
      return -1;
    }
  }

  conn->http = xr_http_new(conn->bio);
  conn->is_open = 1;
  return 0;
}

void xr_client_close(xr_client_conn* conn)
{
  g_assert(conn != NULL);

  if (!conn->is_open)
    return;

  xr_trace(XR_DEBUG_CLIENT_TRACE, "(conn=%p)", conn);

  if (conn->secure)
    BIO_ssl_shutdown(conn->bio);

  xr_http_free(conn->http);
  conn->http = NULL;
  BIO_free_all(conn->bio);
  conn->bio = NULL;
  conn->is_open = FALSE;
}

int xr_client_call(xr_client_conn* conn, xr_call* call, GError** err)
{
  char* buffer;
  int length;
  gsize rs;
  gboolean write_success;
  GString* response;

  g_assert(conn != NULL);
  g_assert(call != NULL);

  g_return_val_if_fail(err == NULL || *err == NULL, -1);
  xr_trace(XR_DEBUG_CLIENT_TRACE, "(conn=%p, call=%p)", conn, call);

  if (!conn->is_open)
  {
    g_set_error(err, XR_CLIENT_ERROR, XR_CLIENT_ERROR_CLOSED, "Can't perform RPC on closed connection.");
    return -1;
  }

  /* serialize nad send XML-RPC request */
  xr_call_serialize_request(call, &buffer, &length);
  xr_http_setup_request(conn->http, "POST", conn->resource, conn->host);
  xr_http_set_header(conn->http, "Content-Type", "text/xml");
  xr_http_set_message_length(conn->http, length);
  write_success = xr_http_write_all(conn->http, buffer, length, err);
  xr_call_free_buffer(buffer);
  if (!write_success)
  {
    xr_client_close(conn);
    return -1;
  }

  /* receive HTTP response header */
  if (!xr_http_read_header(conn->http, err))
    return -1;

  /* check if some dumb bunny sent us wrong message type */
  if (xr_http_get_message_type(conn->http) != XR_HTTP_RESPONSE)
    return -1;

  response = xr_http_read_all(conn->http, err);
  if (response == NULL)
  {
    g_set_error(err, XR_CLIENT_ERROR, XR_CLIENT_ERROR_IO, "HTTP receive failed.");
    xr_client_close(conn);
    return -1;
  }

  rs = xr_call_unserialize_response(call, response->str, response->len);
  g_string_free(response, TRUE);
  if (rs)
  {
    g_set_error(err, 0, xr_call_get_error_code(call), "%s", xr_call_get_error_message(call));
    return -1;
  }

  if (xr_debug_enabled & XR_DEBUG_CALL)
    xr_call_dump(call, 0);

  return 0;
}

void xr_client_free(xr_client_conn* conn)
{
  xr_trace(XR_DEBUG_CLIENT_TRACE, "(conn=%p)", conn);

  g_assert(conn != NULL);
  xr_client_close(conn);
  g_free(conn->host);
  g_free(conn->resource);
  SSL_CTX_free(conn->ctx);
  g_free(conn);
}

GQuark xr_client_error_quark()
{
  static GQuark quark;
  return quark ? quark : (quark = g_quark_from_static_string("xr_client_error"));
}
