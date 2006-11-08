#ifndef __XMLRPC_SERVER_H__
#define __XMLRPC_SERVER_H__

#include <glib.h>

/** @file xmlrpc_server Header
 */

#include "xmlrpc-call.h"

typedef struct _xr_server xr_server;
typedef struct _xr_servlet xr_servlet;

xr_server* xr_server_new();
int xr_server_run(xr_server* server);
int xr_server_stop(xr_server* server);
int xr_server_register_servlet(xr_server* server, xr_servlet* servlet);
void xr_server_free(xr_server* server);

void xr_server_init();

#endif
