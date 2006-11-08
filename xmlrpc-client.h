#ifndef __XMLRPC_CLIENT_H__
#define __XMLRPC_CLIENT_H__

#include <glib.h>

#include "xmlrpc-call.h"

/** @file xmlrpc_client Header
 */

typedef struct _xr_client_conn xr_client_conn;

xr_client_conn* xr_client_new();
int xr_client_open(xr_client_conn* conn, char* uri);
int xr_client_close(xr_client_conn* conn);
int xr_client_call(xr_client_conn* conn, xr_call* call);
void xr_client_free(xr_client_conn* conn);

void xr_client_init();

#endif
