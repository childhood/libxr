/** @file xr-server.h
 *
 * XML-RPC Server API
 *
 * This API can be used to implement multithreaded XML-RPC server.
 */

#ifndef __XR_SERVER_H__
#define __XR_SERVER_H__

#include "xr-call.h"

/** Opaque data structrure that represents XML-RPC server.
 */
typedef struct _xr_server xr_server;

/** Opaque data structrure that represents single instance of servlet
 * object.
 */
typedef struct _xr_servlet xr_servlet;

/** Servlet method callback type.
 */
typedef int (*servlet_method_t)(xr_servlet* servlet, xr_call* call);

/** Servlet init callback type.
 */
typedef int (*servlet_init_t)(xr_servlet* servlet);

/** Servlet fini callback type.
 */
typedef void (*servlet_fini_t)(xr_servlet* servlet);

/** Servlet method description structure.
 */
typedef struct _xr_servlet_method_def xr_servlet_method_def;

/** Servlet description structure.
 */
typedef struct _xr_servlet_def xr_servlet_def;

/** Servlet method description structure.
 */
struct _xr_servlet_method_def
{
  char* name;                 /**< Method name. */
  servlet_method_t cb;        /**< Method callback. */
};

/** Servlet description structure.
 */
struct _xr_servlet_def
{
  char* name;                       /**< Servlet name (/Name resource for client). */
  int size;                         /**< Size of the private object. */
  servlet_init_t init;              /**< Servlet constructor. */
  servlet_fini_t fini;              /**< Servlet destructor. */
  int methods_count;                /**< Count of the methods implemented by the server. */
  xr_servlet_method_def* methods;   /**< Methods descriptions. */
};

/** Initialize libxr server library. This must be first function called
 * in the server code.
 */
void xr_server_init();

/** Create new server object.
 *
 * @param port Port and IP address to bind to.
 * @param cert Combined PEM file with server certificate and private
 *   key. Use NULL to create non-secure server.
 *
 * @return New server object.
 */
xr_server* xr_server_new(const char* port, const char* cert);

/** Run server. This function will start listening for incomming
 * connections and push them to the thread pool where they are
 * handled individually.
 *
 * @param server Server object.
 *
 * @return Function returns -1 on fatal error or 0 on safe stop
 *   by @ref xr_server_stop. Otherwise it will block.
 */
int xr_server_run(xr_server* server);

/** Stop server.
 *
 * @param server Server object.
 */
void xr_server_stop(xr_server* server);

/** Free server object.
 *
 * @param server Server object.
 */
void xr_server_free(xr_server* server);

/** Register servlet type with the server.
 *
 * @param server Server object.
 * @param servlet Servlet object.
 *
 * @return Function returns -1 on error, 0 on success.
 */
int xr_server_register_servlet(xr_server* server, xr_servlet_def* servlet);

/** Get private data for the servlet.
 *
 * @param servlet Servlet object.
 *
 * @return Returns private data associated with the servlet.
 */
void* xr_servlet_get_priv(xr_servlet* servlet);

/** Set error return value on the current call.
 *
 * @param servlet Servlet object.
 * @param code Error code.
 * @param msg Error message.
 */
void xr_servlet_return_error(xr_servlet* servlet, int code, char* msg);

#endif
