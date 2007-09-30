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
 */

enum
{
  XR_HTTP_REQUEST,
  XR_HTTP_RESPONSE
};

typedef struct _xr_http xr_http;

xr_http* xr_http_new();
void xr_http_free(xr_http* http);

gchar* xr_http_get_header(xr_http* http, const gchar* name);

int xr_http_receive(xr_http* http, int message_type, gchar** buffer, gint* length);
int xr_http_send(xr_http* http, int message_type, gchar* buffer, gint length);

void xr_http_setup_request(xr_http* http, gchar* method, gchar* resource, gchar* host);
void xr_http_setup_response(xr_http* http, gint code);

char* xr_http_get_resource(xr_http* http);

#endif
