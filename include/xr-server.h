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

GQuark xr_server_error_quark();
#define XR_SERVER_ERROR xr_server_error_quark()

typedef enum
{
  XR_SERVER_ERROR_FAILED
} XRServerError;

/** Create new server object.
 *
 * @param cert Combined PEM file with server certificate and private
 *   key. Use NULL to create non-secure server.
 * @param threads Number of the threads in the pool.
 * @param err Pointer to the variable to store error to on error.
 *
 * @return New server object on success.
 */
xr_server* xr_server_new(const char* cert, int threads, GError** err);

/** Bind to the specified host/port.
 *
 * @param server Server object.
 * @param port Port and IP address to bind to.
 * @param err Pointer to the variable to store error to on error.
 *
 * @return Function returns -1 on error or 0 on success.
 */
int xr_server_bind(xr_server* server, const char* port, GError** err);

/** Run server. This function will start listening for incomming
 * connections and push them to the thread pool where they are
 * handled individually.
 *
 * @param server Server object.
 * @param err Pointer to the variable to store error to on error.
 *
 * @return Function returns -1 on fatal error or 0 on safe stop
 *   by @ref xr_server_stop. Otherwise it will block.
 */
int xr_server_run(xr_server* server, GError** err);

/** Get event source for this server. Pass this to the GMainContext
 * and run g_main_loop_run().
 *
 * @param server Server object.
 *
 * @return Function returns event source.
 */
GSource* xr_server_source(xr_server* server);

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

#endif
