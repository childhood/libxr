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
