#ifndef __XR_CLIENT_H__
#define __XR_CLIENT_H__

#include <glib.h>

#include "xr-call.h"

/** @file xmlrpc_client Header
 */

typedef struct _xr_client_conn xr_client_conn;
typedef int (*xr_demarchalizer_t)(xr_value* v, void** val);

xr_client_conn* xr_client_new();
void xr_client_free(xr_client_conn* conn);

int xr_client_open(xr_client_conn* conn, char* uri);
int xr_client_close(xr_client_conn* conn);

int xr_client_call(xr_client_conn* conn, xr_call* call);
int xr_client_call_ex(xr_client_conn* conn, xr_call* call, xr_demarchalizer_t dem, void** retval);

int xr_client_get_error_code(xr_client_conn* conn);
char* xr_client_get_error_message(xr_client_conn* conn);
void xr_client_reset_error(xr_client_conn* conn);

void xr_client_init();

#endif
