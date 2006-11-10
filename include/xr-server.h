#ifndef __XR_SERVER_H__
#define __XR_SERVER_H__

#include <glib.h>

/** @file xmlrpc_server Header
 */

#include "xr-call.h"

void xr_server_init();

/** Server routines.
 */

typedef struct _xr_server xr_server;

xr_server* xr_server_new(const char* certfile, const char* port);
int xr_server_run(xr_server* server);
void xr_server_stop(xr_server* server);
void xr_server_free(xr_server* server);

/** Servlet registrations routines.
 */

typedef int (*servlet_method_t)(void* obj, xr_call* call);
typedef int (*servlet_construct_t)(void* obj);
typedef void (*servlet_destruct_t)(void* obj);
typedef struct _xr_servlet_method_desc xr_servlet_method_desc;
typedef struct _xr_servlet_desc xr_servlet_desc;

int xr_server_register_servlet(xr_server* server, xr_servlet_desc* servlet);

#endif
