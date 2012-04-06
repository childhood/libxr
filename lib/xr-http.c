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

#include "xr-http.h"
#include "xr-lib.h"
#include "xr-utils.h"

enum state
{
  STATE_INIT = 0,        /* header should be read or written */
  STATE_HEADER_READ,     /* header should be processed and body should be read */
  STATE_READING_BODY,    /* body should be read */
  STATE_HEADER_WRITTEN,  /* body should be written now */
  STATE_WRITING_BODY,    /* body should be written */
  STATE_ERROR            /* fatal error, connection should be closed */
};

struct _xr_http
{
  GDataInputStream* in;
  GOutputStream* out;

  gsize bytes_read;
  int state;

  int msg_type;
  char* req_method;
  char* req_resource;
  char* req_version;
  int res_code;
  char* res_reason;
  GHashTable* headers;
  gssize content_length;
};

/* private methods */

static GRegex* regex_res = NULL;
static GRegex* regex_req = NULL;

void xr_http_init()
{
  if (G_UNLIKELY(regex_res == NULL))
    regex_res = g_regex_new("^HTTP/([0-9]+\\.[0-9]+) ([0-9]+) (.+)$", 0, 0, NULL);
  if (G_UNLIKELY(regex_req == NULL))
    regex_req = g_regex_new("^([A-Z]+) ([^ ]+) HTTP/([0-9]+\\.[0-9]+)$", 0, 0, NULL);
}

static gboolean _xr_http_header_parse_first_line(xr_http* http, const char* line)
{
  GMatchInfo *match_info = NULL;

  xr_http_init();
  
  if (g_regex_match(regex_res, line, 0, &match_info))
  {
    char* code;

    code = g_match_info_fetch(match_info, 2);
    http->res_code = atoi(code);
    g_free(code);
    g_free(http->res_reason);
    http->res_reason = g_match_info_fetch(match_info, 3);
    http->msg_type = XR_HTTP_RESPONSE;

    g_match_info_free(match_info);
    return TRUE;
  }

  g_match_info_free(match_info);
  
  if (g_regex_match(regex_req, line, 0, &match_info))
  {
    g_free(http->req_method);
    g_free(http->req_resource);
    g_free(http->req_version);
    http->req_method = g_match_info_fetch(match_info, 1);
    http->req_resource = g_match_info_fetch(match_info, 2);
    http->req_version = g_match_info_fetch(match_info, 3);
    http->msg_type = XR_HTTP_REQUEST;

    g_match_info_free(match_info);
    return TRUE;
  }

  g_match_info_free(match_info);

  return FALSE;
}

/* public methods */

xr_http* xr_http_new(GIOStream* stream)
{
  g_return_val_if_fail(stream != NULL, NULL);

  xr_http* http = g_new0(xr_http, 1);
  http->in = g_data_input_stream_new(g_io_stream_get_input_stream(stream));
  g_data_input_stream_set_newline_type(http->in, G_DATA_STREAM_NEWLINE_TYPE_ANY);
  http->out = g_buffered_output_stream_new_sized(g_io_stream_get_output_stream(stream), 16*1024);
  http->headers = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

  xr_trace(XR_DEBUG_HTTP_TRACE, "(http=%p)", http);

  return http;
}

void xr_http_free(xr_http* http)
{
  xr_trace(XR_DEBUG_HTTP_TRACE, "(http=%p)", http);

  if (http == NULL)
    return;

  g_object_unref(http->in);
  g_object_unref(http->out);
  g_hash_table_destroy(http->headers);
  g_free(http->req_method);
  g_free(http->req_resource);
  g_free(http->req_version);
  g_free(http->res_reason);
  memset(http, 0, sizeof(*http));
  g_free(http);
}

gboolean xr_http_read_header(xr_http* http, GError** err)
{
  GError* local_err = NULL;
  char* header;
  gboolean first_line = TRUE;
  const char* clen;

  g_return_val_if_fail(http != NULL, FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
  g_return_val_if_fail(http->state == STATE_INIT, FALSE);

  xr_trace(XR_DEBUG_HTTP_TRACE, "(http=%p)", http);

  g_hash_table_remove_all(http->headers);

  if (xr_debug_enabled & XR_DEBUG_HTTP)
    g_print("<<<<< HTTP RECEIVE START <<<<<\n");

  /* read first line */
  header = g_data_input_stream_read_line(http->in, NULL, NULL, &local_err);
  if (local_err)
  {
    g_propagate_prefixed_error(err, local_err, "HTTP read_line failed: ");
    goto err;
  }

  if (header == NULL)
    return FALSE;

  if (xr_debug_enabled & XR_DEBUG_HTTP)
    g_print("%s\n", header);

  if (!_xr_http_header_parse_first_line(http, header))
  {
    g_set_error(err, XR_HTTP_ERROR, XR_HTTP_ERROR_FAILED, "Invalid HTTP first line.");
    goto err;
  }

  g_free(header);

  while (TRUE)
  {
    header = g_data_input_stream_read_line(http->in, NULL, NULL, &local_err);
    if (local_err)
    {
      g_propagate_prefixed_error(err, local_err, "HTTP read_line failed: ");
      goto err;
    }

    if (xr_debug_enabled & XR_DEBUG_HTTP)
      g_print("%s\n", header);

    /* end of header */
    if (!header || *header == '\0')
    {
      g_free(header);

      clen = g_hash_table_lookup(http->headers, "content-length");
      http->content_length = clen ? atoi(clen) : -1;

      if (http->msg_type == XR_HTTP_REQUEST && !strcmp(http->req_method, "GET"))
      {
        http->state = STATE_INIT;
        if (xr_debug_enabled & XR_DEBUG_HTTP)
          g_print(">>>>> HTTP RECEIVE END >>>>>>>\n");
      }
      else
        http->state = STATE_HEADER_READ;

      return TRUE;
    }

    char* colon = strchr(header, ':');

    if (colon)
    {
      *colon = '\0';
      char* key = g_ascii_strdown(g_strstrip(header), -1);
      char* value = g_strdup(g_strstrip(colon + 1));
      g_hash_table_replace(http->headers, key, value);
    }

    g_free(header);
  }

err:
  if (xr_debug_enabled & XR_DEBUG_HTTP)
    g_print(">>>>> HTTP RECEIVE ERROR >>>>>\n");

  http->state = STATE_ERROR;
  return FALSE;
}

const char* xr_http_get_header(xr_http* http, const char* name)
{
  const char* value;
  char* _name;

  g_return_val_if_fail(http != NULL, NULL);
  g_return_val_if_fail(name != NULL, NULL);

  xr_trace(XR_DEBUG_HTTP_TRACE, "(http=%p)", http);

  _name = g_ascii_strdown(name, -1);
  value = g_hash_table_lookup(http->headers, _name);
  g_free(_name);

  return value;
}

void xr_http_set_basic_auth(xr_http* http, const char* username, const char* password)
{
  g_return_if_fail(http != NULL);
  g_return_if_fail(username != NULL);
  g_return_if_fail(password != NULL);

  char* auth_str = g_strdup_printf("%s:%s", username, password);
  char* enc_auth_str = g_base64_encode(auth_str, strlen(auth_str));
  char* auth_value = g_strdup_printf("Basic %s", enc_auth_str);
  xr_http_set_header(http, "Authorization", auth_value);
  g_free(auth_str);
  g_free(enc_auth_str);
  g_free(auth_value);
}

gboolean xr_http_get_basic_auth(xr_http* http, char** username, char** password)
{
  gsize auth_str_len, i;

  g_return_val_if_fail(http != NULL, FALSE);
  g_return_val_if_fail(username != NULL, FALSE);
  g_return_val_if_fail(password != NULL, FALSE);

  *username = NULL;
  *password = NULL;

  const char* auth_header = xr_http_get_header(http, "Authorization");
  if (auth_header == NULL || !g_str_has_prefix(auth_header, "Basic "))
    return FALSE;

  char* enc_auth_str = g_strstrip(g_strdup(auth_header + 6));
  guchar* auth_str = g_base64_decode(enc_auth_str, &auth_str_len);
  g_free(enc_auth_str);

  if (auth_str == NULL || auth_str_len < 1)
    return FALSE;

  for (i = 0; i < auth_str_len; i++)
    if (auth_str[i] == ':')
      break;

  if (i == auth_str_len)
  {
    g_free(auth_str);
    return FALSE;
  }

  *username = g_strndup(auth_str, i);
  *password = g_strndup(auth_str + i + 1, auth_str_len - i - 1);

  return TRUE;
}

const char* xr_http_get_resource(xr_http* http)
{
  g_return_val_if_fail(http != NULL, NULL);

  xr_trace(XR_DEBUG_HTTP_TRACE, "(http=%p)", http);

  return http->req_resource;
}

const char* xr_http_get_method(xr_http* http)
{
  g_return_val_if_fail(http != NULL, NULL);

  xr_trace(XR_DEBUG_HTTP_TRACE, "(http=%p)", http);

  return http->req_method;
}

int xr_http_get_version(xr_http* http)
{
  g_return_val_if_fail(http != NULL, 0);

  xr_trace(XR_DEBUG_HTTP_TRACE, "(http=%p)", http);

  if (http->req_version == NULL)
    return 0;
  if (!strcmp(http->req_version, "1.1"))
    return 1;
  return 0;
}

int xr_http_get_code(xr_http* http)
{
  g_return_val_if_fail(http != NULL, -1);

  xr_trace(XR_DEBUG_HTTP_TRACE, "(http=%p)", http);

  return http->res_code;
}

xr_http_message_type xr_http_get_message_type(xr_http* http)
{
  g_return_val_if_fail(http != NULL, XR_HTTP_NONE);

  xr_trace(XR_DEBUG_HTTP_TRACE, "(http=%p)", http);

  return http->msg_type;
}

gssize xr_http_get_message_length(xr_http* http)
{
  g_return_val_if_fail(http != NULL, -1);

  xr_trace(XR_DEBUG_HTTP_TRACE, "(http=%p)", http);

  return http->content_length;
}

gssize xr_http_read(xr_http* http, char* buffer, gsize length, GError** err)
{
  GError* local_err = NULL;
  gsize bytes_read;
  gssize bytes_remaining;

  g_return_val_if_fail(http != NULL, -1);
  g_return_val_if_fail(err == NULL || *err == NULL, -1);
  g_return_val_if_fail(buffer != NULL, -1);
  g_return_val_if_fail(length > 0, -1);
  g_return_val_if_fail(http->state == STATE_HEADER_READ || http->state == STATE_READING_BODY || http->state == STATE_INIT, -1);

  xr_trace(XR_DEBUG_HTTP_TRACE, "(http=%p)", http);

  if (http->state == STATE_INIT)
    return 0;

  if (http->state == STATE_HEADER_READ)
  {
    http->state = STATE_READING_BODY;
    http->bytes_read = 0;
  }

  bytes_remaining = http->content_length - http->bytes_read;
  g_input_stream_read_all(G_INPUT_STREAM(http->in), buffer, MIN(length, bytes_remaining), &bytes_read, NULL, &local_err);
  if (local_err)
  {
    g_propagate_prefixed_error(err, local_err, "HTTP read failed: ");
    http->state = STATE_ERROR;
    return -1;
  }

  if (xr_debug_enabled & XR_DEBUG_HTTP)
    g_print("%.*s", (int)bytes_read, buffer);

  http->bytes_read += bytes_read;

  /* check if we are done XXX: is this right? */
  if ((http->content_length < 0 && bytes_read == 0) ||
      (http->content_length >= 0 && http->bytes_read >= http->content_length))
  {
    http->state = STATE_INIT;
    if (xr_debug_enabled & XR_DEBUG_HTTP)
      g_print(">>>>> HTTP RECEIVE END >>>>>>>\n");
  }

  return bytes_read;
}

GString* xr_http_read_all(xr_http* http, GError** err)
{
  GString* str;
  gssize bytes_read;

  g_return_val_if_fail(http != NULL, NULL);
  g_return_val_if_fail(err == NULL || *err == NULL, NULL);
  g_return_val_if_fail(http->state == STATE_HEADER_READ, NULL);
  g_return_val_if_fail(http->content_length > 0, NULL);

  xr_trace(XR_DEBUG_HTTP_TRACE, "(http=%p)", http);

  str = g_string_sized_new(http->content_length);
  g_string_set_size(str, http->content_length);

  bytes_read = xr_http_read(http, str->str, http->content_length, err);
  if (bytes_read < 0 || bytes_read < http->content_length)
  {
    g_string_free(str, TRUE);
    http->state = STATE_ERROR;
    if (bytes_read > 0)
      g_set_error(err, XR_HTTP_ERROR, XR_HTTP_ERROR_FAILED, "HTTP read failed: incomplete message.");
    return NULL;
  }

  http->state = STATE_INIT;

  return str;
}

void xr_http_set_header(xr_http* http, const char* name, const char* value)
{
  g_return_if_fail(http != NULL);
  g_return_if_fail(name != NULL);
  g_return_if_fail(value != NULL);
  g_return_if_fail(http->state == STATE_INIT);

  xr_trace(XR_DEBUG_HTTP_TRACE, "(http=%p)", http);

  g_hash_table_replace(http->headers, g_strdup(name), g_strdup(value));
}

void xr_http_set_message_type(xr_http* http, xr_http_message_type type)
{
  g_return_if_fail(http != NULL);
  g_return_if_fail(type == XR_HTTP_REQUEST || type == XR_HTTP_RESPONSE);
  g_return_if_fail(http->state == STATE_INIT);

  xr_trace(XR_DEBUG_HTTP_TRACE, "(http=%p)", http);

  http->msg_type = type;
}

void xr_http_set_message_length(xr_http* http, gsize length)
{
  g_return_if_fail(http != NULL);
  g_return_if_fail(http->state == STATE_INIT);

  xr_trace(XR_DEBUG_HTTP_TRACE, "(http=%p)", http);

  http->content_length = length;
  g_hash_table_replace(http->headers, g_strdup("Content-Length"), g_strdup_printf("%" G_GSIZE_FORMAT, length));
}

void xr_http_setup_request(xr_http* http, const char* method, const char* resource, const char* host)
{
  g_return_if_fail(http != NULL);
  g_return_if_fail(method != NULL);
  g_return_if_fail(resource != NULL);
  g_return_if_fail(host != NULL);
  g_return_if_fail(http->state == STATE_INIT);

  xr_trace(XR_DEBUG_HTTP_TRACE, "(http=%p)", http);

  http->msg_type = XR_HTTP_REQUEST;

  g_hash_table_remove_all(http->headers);
  xr_http_set_header(http, "Host", host);
  xr_http_set_header(http, "Connection", "keep-alive");

  g_free(http->req_method);
  g_free(http->req_resource);
  http->req_method = g_strdup(method);
  http->req_resource = g_strdup(resource);
}

void xr_http_setup_response(xr_http* http, int code)
{
  g_return_if_fail(http != NULL);
  g_return_if_fail(http->state == STATE_INIT);

  xr_trace(XR_DEBUG_HTTP_TRACE, "(http=%p)", http);

  http->msg_type = XR_HTTP_RESPONSE;

  g_hash_table_remove_all(http->headers);
  if (xr_http_get_version(http) == 1)
    xr_http_set_header(http, "Connection", "keep-alive");
  xr_http_set_header(http, "Content-Type", "text/xml");

  http->res_code = code;

  g_free(http->res_reason);
  http->res_reason = NULL;

  switch (code)
  {
    // HTTP 1.1 Status Codes
    case 100: http->res_reason = g_strdup("Continue"); break;
    case 101: http->res_reason = g_strdup("Switching Protocols"); break;
    case 200: http->res_reason = g_strdup("OK"); break;
    case 201: http->res_reason = g_strdup("Created"); break;
    case 202: http->res_reason = g_strdup("Accepted"); break;
    case 203: http->res_reason = g_strdup("Non-Authoritative Information"); break;
    case 204: http->res_reason = g_strdup("No Content"); break;
    case 205: http->res_reason = g_strdup("Reset Content"); break;
    case 206: http->res_reason = g_strdup("Partial Content"); break;
    case 300: http->res_reason = g_strdup("Multiple Choices"); break;
    case 301: http->res_reason = g_strdup("Moved Permanently"); break;
    case 302: http->res_reason = g_strdup("Found"); break;
    case 303: http->res_reason = g_strdup("See Other"); break;
    case 304: http->res_reason = g_strdup("Not Modified"); break;
    case 305: http->res_reason = g_strdup("Use Proxy"); break;
    case 306: http->res_reason = g_strdup("(Unused)"); break;
    case 307: http->res_reason = g_strdup("Temporary Redirect"); break;
    case 400: http->res_reason = g_strdup("Bad Request"); break;
    case 401: http->res_reason = g_strdup("Unauthorized"); break;
    case 402: http->res_reason = g_strdup("Payment Required"); break;
    case 403: http->res_reason = g_strdup("Forbidden"); break;
    case 404: http->res_reason = g_strdup("Not Found"); break;
    case 405: http->res_reason = g_strdup("Method Not Allowed"); break;
    case 406: http->res_reason = g_strdup("Not Acceptable"); break;
    case 407: http->res_reason = g_strdup("Proxy Authentication Required"); break;
    case 408: http->res_reason = g_strdup("Request Timeout"); break;
    case 409: http->res_reason = g_strdup("Conflict"); break;
    case 410: http->res_reason = g_strdup("Gone"); break;
    case 411: http->res_reason = g_strdup("Length Required"); break;
    case 412: http->res_reason = g_strdup("Precondition Failed"); break;
    case 413: http->res_reason = g_strdup("Request Entity Too Large"); break;
    case 414: http->res_reason = g_strdup("Request-URI Too Long"); break;
    case 415: http->res_reason = g_strdup("Unsupported Media Type"); break;
    case 416: http->res_reason = g_strdup("Requested Range Not Satisfiable"); break;
    case 417: http->res_reason = g_strdup("Expectation Failed"); break;
    case 500: http->res_reason = g_strdup("Internal Server Error"); break;
    case 501: http->res_reason = g_strdup("Not Implemented"); break;
    case 502: http->res_reason = g_strdup("Bad Gateway"); break;
    case 503: http->res_reason = g_strdup("Service Unavailable"); break;
    case 504: http->res_reason = g_strdup("Gateway Timeout"); break;
    case 505: http->res_reason = g_strdup("HTTP Version Not Supported"); break;
    default:  http->res_reason = g_strdup("Unknown Status"); break;
  }
}

static void add_header(const char* key, const char* value, GString* header)
{
  g_string_append_printf(header, "%s: %s\r\n", key, value);
}

gboolean xr_http_write_header(xr_http* http, GError** err)
{
  GError* local_err = NULL;
  GString* header;

  g_return_val_if_fail(http != NULL, FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
  g_return_val_if_fail(http->state == STATE_INIT, FALSE);

  xr_trace(XR_DEBUG_HTTP_TRACE, "(http=%p)", http);

  header = g_string_sized_new(256);
  if (http->msg_type == XR_HTTP_REQUEST)
    g_string_append_printf(header, "%s %s HTTP/1.1\r\n", http->req_method, http->req_resource);
  else if (http->msg_type == XR_HTTP_RESPONSE)
    g_string_append_printf(header, "HTTP/1.%d %d %s\r\n", xr_http_get_version(http), http->res_code, http->res_reason);
  else
  {
    g_set_error(err, XR_HTTP_ERROR, XR_HTTP_ERROR_FAILED, "Undefined message type: %d.", http->msg_type);
    g_string_free(header, TRUE);
    http->state = STATE_ERROR;
    return FALSE;
  }

  g_hash_table_foreach(http->headers, (GHFunc)add_header, header);
  g_string_append(header, "\r\n");

  if (xr_debug_enabled & XR_DEBUG_HTTP)
  {
    g_print("<<<<< HTTP SEND START <<<<<\n");
    g_print("%s", header->str);
  }

  if (!g_output_stream_write_all(http->out, header->str, header->len, NULL, NULL, &local_err))
  {
    g_propagate_prefixed_error(err, local_err, "HTTP write failed: ");
    http->state = STATE_ERROR;
    g_string_free(header, TRUE);
    return FALSE;
  }

  http->state = STATE_HEADER_WRITTEN;

  g_string_free(header, TRUE);
  return TRUE;
}

gboolean xr_http_write(xr_http* http, const char* buffer, gsize length, GError** err)
{
  GError* local_err = NULL;

  g_return_val_if_fail(http != NULL, FALSE);
  g_return_val_if_fail(buffer != NULL, FALSE);
  g_return_val_if_fail(length > 0, FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
  g_return_val_if_fail(http->state == STATE_WRITING_BODY || http->state == STATE_HEADER_WRITTEN, FALSE);

  xr_trace(XR_DEBUG_HTTP_TRACE, "(http=%p)", http);

  http->state = STATE_WRITING_BODY;

  if (!g_output_stream_write_all(http->out, buffer, length, NULL, NULL, &local_err))
  {
    g_propagate_prefixed_error(err, local_err, "HTTP write failed: ");
    http->state = STATE_ERROR;
    return FALSE;
  }

  if (xr_debug_enabled & XR_DEBUG_HTTP)
    g_print("%.*s", (int)length, buffer);

  return TRUE;
}

gboolean xr_http_write_complete(xr_http* http, GError** err)
{
  GError* local_err = NULL;

  g_return_val_if_fail(http != NULL, FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
  g_return_val_if_fail(http->state == STATE_WRITING_BODY || http->state == STATE_HEADER_WRITTEN, FALSE);

  xr_trace(XR_DEBUG_HTTP_TRACE, "(http=%p)", http);

  if (!g_output_stream_flush(http->out, NULL, &local_err))
  {
    g_propagate_prefixed_error(err, local_err, "HTTP flush failed: ");
    http->state = STATE_ERROR;
    return FALSE;
  }

  if (xr_debug_enabled & XR_DEBUG_HTTP)
    g_print("<<<<< HTTP SEND END <<<<<<<\n");

  http->state = STATE_INIT;

  return TRUE;
}

gboolean xr_http_write_all(xr_http* http, const char* buffer, gssize length, GError** err)
{
  g_return_val_if_fail(http != NULL, FALSE);
  g_return_val_if_fail(buffer != NULL, FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
  g_return_val_if_fail(http->state == STATE_INIT, FALSE);

  xr_trace(XR_DEBUG_HTTP_TRACE, "(http=%p)", http);

  if (length < 0)
    length = strlen(buffer);

  xr_http_set_message_length(http, length);

  if (!xr_http_write_header(http, err))
    return FALSE;

  if (!xr_http_write(http, buffer, length, err))
    return FALSE;

  if (!xr_http_write_complete(http, err))
    return FALSE;

  return TRUE;
}

gboolean xr_http_is_ready(xr_http* http)
{
  g_return_val_if_fail(http != NULL, FALSE);

  return http->state == STATE_INIT;
}

GQuark xr_http_error_quark()
{
  static GQuark quark = 0;
  return quark ? quark : (quark = g_quark_from_static_string("xr_http_error"));
}
