/** @file xr-server.h
 *
 * XML-RPC Server API
 *
 * This API can be used to implement multithreaded XML-RPC server.
 */

#ifndef __XR_SERVER_H__
#define __XR_SERVER_H__

#include "xr-call.h"

void xr_server_init();

/** Server routines.
 */

typedef struct _xr_server xr_server;
typedef struct _xr_servlet xr_servlet;

xr_server* xr_server_new(const char* certfile, const char* port);
int xr_server_run(xr_server* server);
void xr_server_stop(xr_server* server);
void xr_server_free(xr_server* server);

void* xr_servlet_get_priv(xr_servlet* servlet);
void xr_servlet_return_error(xr_servlet* servlet, int code, char* msg);

/** Servlet registrations routines.
 */

typedef int (*servlet_method_t)(xr_servlet* servlet, xr_call* call);
typedef int (*servlet_construct_t)(void* obj);
typedef void (*servlet_destruct_t)(void* obj);

typedef struct _xr_servlet_method_def xr_servlet_method_def;
typedef struct _xr_servlet_def xr_servlet_def;

struct _xr_servlet_method_def
{
  char* name;
  servlet_method_t cb;
};

struct _xr_servlet_def
{
  char* name;                 /* servlet name (/Name resource for client) */
  int size;                   /* size of the servlet struct */
  servlet_construct_t init;   /* servlet constructor */
  servlet_destruct_t fini;
  int methods_count;
  xr_servlet_method_def* methods;
};

int xr_server_register_servlet(xr_server* server, xr_servlet_def* servlet);

#endif
