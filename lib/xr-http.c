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
#ifndef HAVE_GLIB_REGEXP
#include <regex.h>
#endif

#include "xr-http.h"
#include "xr-lib.h"
#include "xr-utils.h"

#define MAX_HEADER_SIZE 2048

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
  BIO* bio;

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

#ifndef HAVE_GLIB_REGEXP

static gboolean http_initialized;
static regex_t regex_res;
static regex_t regex_req;

void xr_http_init()
{
  if (!http_initialized)
  {
    http_initialized = TRUE;
    regcomp(&regex_res, "^HTTP/([0-9]+\\.[0-9]+) ([0-9]+) (.+)$", REG_EXTENDED);
    regcomp(&regex_req, "^([A-Z]+) ([^ ]+) HTTP/([0-9]+\\.[0-9]+)$", REG_EXTENDED);
  }
}

static gboolean _xr_http_header_parse_first_line(xr_http* http, const char* line)
{
  regmatch_t m[4];
  
  if (regexec(&regex_req, line, 4, m, 0) == 0)
  {
    g_free(http->req_method);
    http->req_method = g_strndup(line+m[1].rm_so, m[1].rm_eo-m[1].rm_so);
    g_free(http->req_resource);
    http->req_resource = g_strndup(line+m[2].rm_so, m[2].rm_eo-m[2].rm_so);
    g_free(http->req_version);
    http->req_version = g_strndup(line+m[3].rm_so, m[3].rm_eo-m[3].rm_so);
    http->msg_type = XR_HTTP_REQUEST;
    return TRUE;
  }
  else if (regexec(&regex_res, line, 4, m, 0) == 0)
  {
    gchar* code = g_strndup(line+m[2].rm_so, m[2].rm_eo-m[2].rm_so);
    http->res_code = atoi(code);
    g_free(code);
    g_free(http->res_reason);
    http->res_reason = g_strndup(line+m[3].rm_so, m[3].rm_eo-m[3].rm_so);
    http->msg_type = XR_HTTP_RESPONSE;
    return TRUE;
  }

  return FALSE;
}

#else

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
  gboolean retval = TRUE;

  xr_http_init();
  
  if (g_regex_match(regex_res, line, 0, &match_info))
  {
    char* code;

    code = g_match_info_fetch(match_info, 1);
    http->res_code = atoi(code);
    g_free(code);
    g_free(http->res_reason);
    http->res_reason = g_match_info_fetch(match_info, 2);
    http->msg_type = XR_HTTP_RESPONSE;
  }
  else if (g_regex_match(regex_req, line, 0, &match_info))
  {
    g_free(http->req_method);
    g_free(http->req_resource);
    g_free(http->req_version);
    http->req_method = g_match_info_fetch(match_info, 1);
    http->req_resource = g_match_info_fetch(match_info, 2);
    http->req_version = g_match_info_fetch(match_info, 3);
    http->msg_type = XR_HTTP_REQUEST;
  }
  else
    retval = FALSE;

  g_match_info_free(match_info);

  return retval;
}

#endif

static int BIO_xread(BIO* bio, void* buf, int len)
{
  int bytes_read = 0;
  while (bytes_read < len)
  {
    int rs = BIO_read(bio, buf + bytes_read, len - bytes_read);
    if (rs < 0)
      return -1;
    if (rs == 0)
      return bytes_read;
    bytes_read += rs;
  }
  return len;
}

/* return -1 on error, 0 on success, 1 if line was longer than buffer */
static int BIO_xgets(BIO* bio, char* buffer, int length)
{
  char *cp, tmp;

  memset(buffer, 0, length);
  switch (BIO_gets(bio, buffer, length - 1)) 
  {
    case -2:
      return -1;

    case 0:
    case -1:
      return 1;

    default:
      for (cp = buffer; *cp; cp++)
      {
        if (*cp == '\n' || *cp == '\r') 
        {
          *cp = '\0';
          return 0;
        }
      }
      /* skip rest of "line" */
      tmp = '\0';
      while (tmp != '\n')
        if (BIO_read(bio, &tmp, 1) != 1)
          return 1;
      break;
  }

  return 0;
}

/* public methods */

xr_http* xr_http_new(BIO* bio)
{
  g_return_val_if_fail(bio != NULL, NULL);

  xr_http* http = g_new0(xr_http, 1);
  http->bio = bio;
  http->headers = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

  xr_trace(XR_DEBUG_HTTP_TRACE, "(http=%p)", http);

  return http;
}

void xr_http_free(xr_http* http)
{
  xr_trace(XR_DEBUG_HTTP_TRACE, "(http=%p)", http);

  if (http == NULL)
    return;

  g_hash_table_destroy(http->headers);
  g_free(http->req_method);
  g_free(http->req_resource);
  g_free(http->res_reason);
  memset(http, 0, sizeof(*http));
  g_free(http);
}

gboolean xr_http_read_header(xr_http* http, GError** err)
{
  char header[MAX_HEADER_SIZE + 1];
  gboolean first_line = TRUE;
  const char* clen;

  g_return_val_if_fail(http != NULL, FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
  g_return_val_if_fail(http->state == STATE_INIT, FALSE);

  xr_trace(XR_DEBUG_HTTP_TRACE, "(http=%p)", http);

  g_hash_table_remove_all(http->headers);

  if (xr_debug_enabled & XR_DEBUG_HTTP)
    g_print("<<<<< HTTP RECEIVE START <<<<<\n");

  while (1)
  {
    int rs = BIO_xgets(http->bio, header, MAX_HEADER_SIZE);
    switch (rs)
    {
      case -1:
        g_set_error(err, XR_HTTP_ERROR, XR_HTTP_ERROR_FAILED, "HTTP xgets failed: %s.", xr_get_bio_error_string());
        goto err;

      case 1:
        g_set_error(err, XR_HTTP_ERROR, XR_HTTP_ERROR_FAILED, "HTTP header too long, limit is %d bytes.", MAX_HEADER_SIZE);
        goto err;

      default:
        if (xr_debug_enabled & XR_DEBUG_HTTP)
          g_print("%s\n", header);

        if (first_line)
        {
          if (!_xr_http_header_parse_first_line(http, header))
          {
            g_set_error(err, XR_HTTP_ERROR, XR_HTTP_ERROR_FAILED, "Invalid HTTP message.");
            goto err;
          }
          first_line = FALSE;
        }
        else
        {
          if (*header == '\0')
            goto done;

          char* colon = strchr(header, ':');

          if (colon)
          {
            *colon = '\0';
            char* key = g_ascii_strdown(g_strstrip(header), -1);
            char* value = g_strdup(g_strstrip(colon + 1));
            g_hash_table_replace(http->headers, key, value);
          }
        }
    }
  }

done:
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
  int bytes_read;
  int bytes_remaining;

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
  bytes_read = BIO_xread(http->bio, buffer, MIN(length, bytes_remaining));
  if (bytes_read < 0)
  {
    g_set_error(err, XR_HTTP_ERROR, XR_HTTP_ERROR_FAILED, "HTTP read failed: %s.", xr_get_bio_error_string());
    http->state = STATE_ERROR;
    return -1;
  }

  if (xr_debug_enabled & XR_DEBUG_HTTP)
    g_print("%.*s", bytes_read, buffer);

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
  g_hash_table_replace(http->headers, g_strdup("Content-Length"), g_strdup_printf("%u", length));
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
  g_return_if_fail(code >= 200 && code <= 500);
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

void add_header(const char* key, const char* value, GString* header)
{
  g_string_append_printf(header, "%s: %s\r\n", key, value);
}

gboolean xr_http_write_header(xr_http* http, GError** err)
{
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

  if (BIO_write(http->bio, header->str, header->len) != header->len)
  {
    g_set_error(err, XR_HTTP_ERROR, XR_HTTP_ERROR_FAILED, "HTTP write failed: %s.", xr_get_bio_error_string());
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
  g_return_val_if_fail(http != NULL, FALSE);
  g_return_val_if_fail(buffer != NULL, FALSE);
  g_return_val_if_fail(length > 0, FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
  g_return_val_if_fail(http->state == STATE_WRITING_BODY || http->state == STATE_HEADER_WRITTEN, FALSE);

  xr_trace(XR_DEBUG_HTTP_TRACE, "(http=%p)", http);

  http->state = STATE_WRITING_BODY;

  if (BIO_write(http->bio, buffer, length) != length)
  {
    g_set_error(err, XR_HTTP_ERROR, XR_HTTP_ERROR_FAILED, "HTTP write failed: %s.", xr_get_bio_error_string());
    http->state = STATE_ERROR;
    return FALSE;
  }

  if (xr_debug_enabled & XR_DEBUG_HTTP)
    g_print("%.*s", length, buffer);

  return TRUE;
}

gboolean xr_http_write_complete(xr_http* http, GError** err)
{
  g_return_val_if_fail(http != NULL, FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
  g_return_val_if_fail(http->state == STATE_WRITING_BODY || http->state == STATE_HEADER_WRITTEN, FALSE);

  xr_trace(XR_DEBUG_HTTP_TRACE, "(http=%p)", http);

  if (BIO_flush(http->bio) != 1)
  {
    g_set_error(err, XR_HTTP_ERROR, XR_HTTP_ERROR_FAILED, "HTTP flush failed: %s.", xr_get_bio_error_string());
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
  static GQuark quark;
  return quark ? quark : (quark = g_quark_from_static_string("xr_http_error"));
}
