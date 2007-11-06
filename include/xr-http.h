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

#ifndef __XR_HTTP_H__
#define __XR_HTTP_H__

#include <glib.h>
#include <openssl/bio.h>

/** @file xr_utils Header
 *
 * xr_http object exists as long as HTTP connection exists. It is used to server
 * multiple requests/responses.
 *
 * xr_http object is created using xr_http_new(bio) and then user is supposed to
 * call xr_http_read_header() that will receive and parse HTTP header. After
 * that user can call one of xr_http_read() functions to read body of the HTTP
 * request or response.
 *
 * When request is received, user must generate response using
 * xr_http_write_header() and optionally body using xr_http_write(). Header
 * values can be set using xr_http_setup_request() or xr_http_setup_response()
 * and xr_http_set_header() functions.
 */

typedef struct _xr_http xr_http;

typedef enum {
  XR_HTTP_NONE,
  XR_HTTP_REQUEST,
  XR_HTTP_RESPONSE
} xr_http_message_type;

#define XR_HTTP_ERROR xr_http_error_quark()

typedef enum
{
  XR_HTTP_ERROR_FAILED
} XRHttpError;

G_BEGIN_DECLS

void xr_http_init();

/** Create new HTTP transport object.
 * 
 * @param bio OpenSSL BIO object used as underlaying transport.
 * 
 * @return New HTTP transport object.
 */
xr_http* xr_http_new(BIO* bio);

/** Destroy HTTP transport object.
 * 
 * @param http HTTP transport object.
 */
void xr_http_free(xr_http* http);

/* receive API */

/** Read HTTP message header.
 * 
 * @param http HTTP transport object.
 * @param err Error object.
 * 
 * @return TRUE on success, FALSE on error.
 */
gboolean xr_http_read_header(xr_http* http, GError** err);

/** Get specific HTTP header from incomming messages by its name.
 * 
 * @param http HTTP transport object.
 * @param name Case insensitive header name (e.g. Content-Type).
 * 
 * @return Header value.
 */
const char* xr_http_get_header(xr_http* http, const char* name);

/** Get HTTP method.
 * 
 * @param http HTTP transport object.
 * 
 * @return Method name (GET/POST/...).
 */
const char* xr_http_get_method(xr_http* http);

/** Get resource (only if xr_http_get_message_type() == XR_HTTP_REQUEST).
 * 
 * @param http HTTP transport object.
 * 
 * @return Resource string (/blah/blah?test).
 */
const char* xr_http_get_resource(xr_http* http);

/** Get message type.
 * 
 * @param http HTTP transport object.
 * 
 * @return XR_HTTP_REQUEST, XR_HTTP_RESPONSE or XR_HTTP_NONE if header was not
 *   read yet.
 */
xr_http_message_type xr_http_get_message_type(xr_http* http);

/** Get length of the message body (Content-Length header value).
 *
 * This function may return -1 if Content-Length was not specified.
 * 
 * @param http HTTP transport object.
 * 
 * @return Message body length.
 */
gssize xr_http_get_message_length(xr_http* http);

/** Read HTTP message body.
 * 
 * @param http HTTP transport object.
 * @param buffer Target buffer.
 * @param length Target buffer length.
 * @param err Error object.
 * 
 * @return -1 on error, actual length that was read.
 */
gssize xr_http_read(xr_http* http, char* buffer, gsize length, GError** err);

/** Read whole message body into a GString object.
 * 
 * @param http HTTP transport object.
 * @param err Error object.
 * 
 * @return GString object with returned data or NULL on error.
 */
GString* xr_http_read_all(xr_http* http, GError** err);

/* transmit API */

/** Set HTTP header for outgoing message.
 * 
 * @param http HTTP transport object.
 * @param name Case insensitive header name (e.g. Content-Type).
 * @param value Header value.
 */
void xr_http_set_header(xr_http* http, const char* name, const char* value);

/** Set outgoing message type.
 * 
 * @param http HTTP transport object.
 * @param type XR_HTTP_REQUEST or XR_HTTP_RESPONSE.
 */
void xr_http_set_message_type(xr_http* http, xr_http_message_type type);

/** Set Content-Length header for outgoing message.
 * 
 * @param http HTTP transport object.
 * @param length Mesasge body length.
 */
void xr_http_set_message_length(xr_http* http, gsize length);

/** Setup outgoing request.
 * 
 * @param http HTTP transport object.
 * @param method HTTP method name (GET/POST).
 * @param resource HTTP resource value.
 * @param host Host header value.
 */
void xr_http_setup_request(xr_http* http, const char* method, const char* resource, const char* host);

/** Setup outgoing response.
 * 
 * @param http HTTP transport object.
 * @param code HTTP response code (200, 404, etc.).
 */
void xr_http_setup_response(xr_http* http, int code);

/** Write outgoing message header.
 *
 * You should call xr_http_setup_*() and xr_http_set_message_length() functions
 * before calling this.
 * 
 * @param http HTTP transport object.
 * @param err Error object.
 * 
 * @return TRUE on success, FALSE on error.
 */
gboolean xr_http_write_header(xr_http* http, GError** err);

/** Write response body.
 *
 * This function will automatically call xr_http_write_header() if it was not
 * called before.
 *
 * This method may be called multiple times to write response iteratively.
 * 
 * @param http HTTP transport object.
 * @param buffer Source buffer.
 * @param length Data length.
 * @param err Error object.
 * 
 * @return TRUE on success, FALSE on error.
 */
gboolean xr_http_write(xr_http* http, const char* buffer, gsize length, GError** err);

/** Complete message.
 *
 * Must be called after body is written by possibly multiple xr_http_write()
 * calls or directly after xr_http_write_header().
 * 
 * @param http HTTP transport object.
 * @param err Error object.
 * 
 * @return TRUE on success, FALSE on error.
 */
gboolean xr_http_write_complete(xr_http* http, GError** err);

/** Write whole message body at once from the GString.
 * 
 * @param http HTTP transport object. 
 * @param buffer Source buffer.
 * @param length Data length.
 * @param err Error object.
 * 
 * @return TRUE on success, FALSE on error.
 */
gboolean xr_http_write_all(xr_http* http, const char* buffer, gsize length, GError** err);

/** Check if object is ready to receive or send message.
 * 
 * @param http HTTP transport object. 
 * 
 * @return TRUE if ready.
 */
gboolean xr_http_is_ready(xr_http* http);

GQuark xr_http_error_quark();

G_END_DECLS

#endif
