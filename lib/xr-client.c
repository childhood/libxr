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

#include <config.h>
#include <stdlib.h>
#include <string.h>

#include "xr-client.h"
#include "xr-http.h"
#include "xr-utils.h"

struct _xr_client_conn
{
  GSocketClient* client;
  GSocketConnection* conn;
  xr_http* http;

  char* resource;
  char* host;
  char* session_id;
  gboolean secure;

  gboolean is_open;
  GHashTable* headers;
  xr_call_transport transport;
};

xr_client_conn* xr_client_new(GError** err)
{
  g_return_val_if_fail(err == NULL || *err == NULL, NULL);

  xr_trace(XR_DEBUG_CLIENT_TRACE, "(err=%p)", err);

  xr_init();

  xr_client_conn* conn = g_new0(xr_client_conn, 1);
  conn->client = g_socket_client_new();

  conn->headers = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
  conn->transport = XR_CALL_XML_RPC;

  return conn;
}

G_LOCK_DEFINE_STATIC(regex);

static gboolean _parse_uri(const char* uri, gboolean* secure, char** host, char** resource)
{
  static GRegex* regex = NULL;
  GMatchInfo *match_info = NULL;

  g_return_val_if_fail(uri != NULL, FALSE);
  g_return_val_if_fail(secure != NULL, FALSE);
  g_return_val_if_fail(host != NULL, FALSE);
  g_return_val_if_fail(resource != NULL, FALSE);

  // precompile regexp
  G_LOCK(regex);
  if (regex == NULL)
    regex = g_regex_new("^([a-z]+)://([a-z0-9.-]+(:([0-9]+))?)(/.+)?$", G_REGEX_CASELESS, 0, NULL);
  G_UNLOCK(regex);

  if (!g_regex_match(regex, uri, 0, &match_info))
    return FALSE;
  
  // check schema
  char* schema = g_match_info_fetch(match_info, 1);
  if (!g_ascii_strcasecmp("http", schema))
    *secure = 0;
  else if (!g_ascii_strcasecmp("https", schema))
    *secure = 1;
  else
  {
    g_free(schema);
    g_match_info_free(match_info);
    return FALSE;
  }
  g_free(schema);
  
  *host = g_match_info_fetch(match_info, 2);
  *resource = g_match_info_fetch(match_info, 5);
  if (*resource == NULL)
    *resource = g_strdup("/RPC2");

  g_match_info_free(match_info);
  return TRUE;
}

gboolean xr_client_open(xr_client_conn* conn, const char* uri, GError** err)
{
  GError* local_err = NULL;

  g_return_val_if_fail(conn != NULL, FALSE);
  g_return_val_if_fail(uri != NULL, FALSE);
  g_return_val_if_fail(!conn->is_open, FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);

  xr_trace(XR_DEBUG_CLIENT_TRACE, "(conn=%p, uri=%s)", conn, uri);

  // parse URI format: http://host:8080/RES
  g_free(conn->host);
  g_free(conn->resource);
  conn->host = NULL;
  conn->resource = NULL;
  if (!_parse_uri(uri, &conn->secure, &conn->host, &conn->resource))
  {
    g_set_error(err, XR_CLIENT_ERROR, XR_CLIENT_ERROR_FAILED, "invalid URI format: %s", uri);
    return FALSE;
  }

  // enable/disable TLS
  if (conn->secure)
  {
    g_socket_client_set_tls(conn->client, TRUE);
    g_socket_client_set_tls_validation_flags(conn->client, G_TLS_CERTIFICATE_VALIDATE_ALL & ~G_TLS_CERTIFICATE_UNKNOWN_CA & ~G_TLS_CERTIFICATE_BAD_IDENTITY);
  }
  else
  {
    g_socket_client_set_tls(conn->client, FALSE);
  }

  conn->conn = g_socket_client_connect_to_host(conn->client, conn->host, 80, NULL, &local_err);
  if (local_err)
  {
    g_propagate_prefixed_error(err, local_err, "Connection failed: ");
    return FALSE;
  }

  xr_set_nodelay(g_socket_connection_get_socket(conn->conn));

  conn->http = xr_http_new(G_IO_STREAM(conn->conn));
  g_free(conn->session_id);
  conn->session_id = g_strdup_printf("%08x%08x%08x%08x", g_random_int(), g_random_int(), g_random_int(), g_random_int());
  conn->is_open = 1;

  xr_client_set_http_header(conn, "X-SESSION-ID", conn->session_id);

  return TRUE;
}

void xr_client_set_http_header(xr_client_conn* conn, const char* name, const char* value)
{
  g_return_if_fail(conn != NULL);
  g_return_if_fail(name != NULL);

  if (value == NULL)
    g_hash_table_remove(conn->headers, name);
  else
    g_hash_table_replace(conn->headers, g_strdup(name), g_strdup(value));
}

void xr_client_reset_http_headers(xr_client_conn* conn)
{
  g_return_if_fail(conn != NULL);

  g_hash_table_remove_all(conn->headers);
}

void xr_client_basic_auth(xr_client_conn* conn, const char* username, const char* password)
{
  g_return_if_fail(conn != NULL);
  g_return_if_fail(username != NULL);
  g_return_if_fail(password != NULL);

  char* auth_str = g_strdup_printf("%s:%s", username, password);
  char* enc_auth_str = g_base64_encode(auth_str, strlen(auth_str));
  char* auth_value = g_strdup_printf("Basic %s", enc_auth_str);
  xr_client_set_http_header(conn, "Authorization", auth_value);
  g_free(auth_str);
  g_free(enc_auth_str);
  g_free(auth_value);
}

xr_http* xr_client_get_http(xr_client_conn* conn)
{
  g_return_val_if_fail(conn != NULL, NULL);

  return conn->http;
}

void xr_client_close(xr_client_conn* conn)
{
  xr_trace(XR_DEBUG_CLIENT_TRACE, "(conn=%p)", conn);

  g_return_if_fail(conn != NULL);

  if (!conn->is_open)
    return;

  xr_http_free(conn->http);
  conn->http = NULL;
  if (conn->client)
    g_object_unref(conn->client);
  if (conn->conn)
    g_object_unref(conn->conn);
  conn->client = NULL;
  conn->conn = NULL;
  conn->is_open = FALSE;
}

static void _add_http_header(const char* name, const char* value, xr_http* http)
{
  xr_http_set_header(http, name, value);
}

gboolean xr_client_set_transport(xr_client_conn* conn, xr_call_transport transport)
{
  g_return_val_if_fail(conn != NULL, FALSE);
  g_return_val_if_fail(transport < XR_CALL_TRANSPORT_COUNT, FALSE);

  conn->transport = transport;

  return TRUE;
}

gboolean xr_client_call(xr_client_conn* conn, xr_call* call, GError** err)
{
  char* buffer;
  int length;
  gboolean rs;
  gboolean write_success;
  GString* response;

  xr_trace(XR_DEBUG_CLIENT_TRACE, "(conn=%p, call=%p)", conn, call);

  g_return_val_if_fail(conn != NULL, FALSE);
  g_return_val_if_fail(call != NULL, FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);

  if (!conn->is_open)
  {
    g_set_error(err, XR_CLIENT_ERROR, XR_CLIENT_ERROR_CLOSED, "Can't perform RPC on closed connection.");
    return FALSE;
  }

  /* serialize nad send XML-RPC request */
  xr_call_set_transport(call, conn->transport);
  xr_call_serialize_request(call, &buffer, &length);
  xr_http_setup_request(conn->http, "POST", conn->resource, conn->host);
  g_hash_table_foreach(conn->headers, (GHFunc)_add_http_header, conn->http);
  if (conn->transport == XR_CALL_XML_RPC)
    xr_http_set_header(conn->http, "Content-Type", "text/xml");
#ifdef XR_JSON_ENABLED
  else if (conn->transport == XR_CALL_JSON_RPC)
    xr_http_set_header(conn->http, "Content-Type", "text/json");
#endif
  xr_http_set_message_length(conn->http, length);
  write_success = xr_http_write_all(conn->http, buffer, length, err);
  xr_call_free_buffer(call, buffer);
  if (!write_success)
  {
    xr_client_close(conn);
    return FALSE;
  }

  /* receive HTTP response header */
  if (!xr_http_read_header(conn->http, err))
    return FALSE;

  /* check if some dumb bunny sent us wrong message type */
  if (xr_http_get_message_type(conn->http) != XR_HTTP_RESPONSE)
    return FALSE;

  response = xr_http_read_all(conn->http, err);
  if (response == NULL)
  {
    xr_client_close(conn);
    return FALSE;
  }

  rs = xr_call_unserialize_response(call, response->str, response->len);
  g_string_free(response, TRUE);
  if (!rs)
  {
    g_set_error(err, 0, xr_call_get_error_code(call), "%s", xr_call_get_error_message(call));

    if (xr_debug_enabled & XR_DEBUG_CALL)
      xr_call_dump(call, 0);

    return FALSE;
  }

  if (xr_debug_enabled & XR_DEBUG_CALL)
    xr_call_dump(call, 0);

  return TRUE;
}

void xr_client_free(xr_client_conn* conn)
{
  xr_trace(XR_DEBUG_CLIENT_TRACE, "(conn=%p)", conn);

  if (conn == NULL)
    return;

  xr_client_close(conn);
  g_free(conn->host);
  g_free(conn->resource);
  g_free(conn->session_id);
  g_hash_table_destroy(conn->headers);
  g_free(conn);
}

GQuark xr_client_error_quark()
{
  static GQuark quark;
  return quark ? quark : (quark = g_quark_from_static_string("xr_client_error"));
}
