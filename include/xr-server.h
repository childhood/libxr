/* 
 * Copyright 2006-2008 Ondrej Jirman <ondrej.jirman@zonio.net>
 * 
 * This file is part of libxr.
 *
 * Libxr is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 2 of the License, or (at your option) any
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

/** @file xr-server.h
 *
 * XML-RPC Server API
 *
 * This API can be used to implement multithreaded XML-RPC server.
 */

#ifndef __XR_SERVER_H__
#define __XR_SERVER_H__

#include "xr-call.h"
#include "xr-http.h"

/** Opaque data structrure that represents XML-RPC server.
 */
typedef struct _xr_server xr_server;

/** Opaque data structrure that represents single instance of servlet
 * object.
 */
typedef struct _xr_servlet xr_servlet;

/** Servlet method callback type.
 */
typedef gboolean (*servlet_method_t)(xr_servlet* servlet, xr_call* call);

/** Servlet init callback type.
 */
typedef gboolean (*servlet_init_t)(xr_servlet* servlet);

/** Servlet fini callback type.
 */
typedef void (*servlet_fini_t)(xr_servlet* servlet);

/** Servlet download callback type.
 */
typedef gboolean (*servlet_download_t)(xr_servlet* servlet);

/** Servlet upload callback type.
 */
typedef gboolean (*servlet_upload_t)(xr_servlet* servlet);

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
  servlet_method_t pre_call;        /**< Pre-call hook. */
  servlet_method_t post_call;       /**< Post-call hook. */
  servlet_download_t download;      /**< Download hook. */
  servlet_upload_t upload;          /**< Upload hook. */
  int methods_count;                /**< Count of the methods implemented by the server. */
  xr_servlet_method_def* methods;   /**< Methods descriptions. */
};

#define XR_SERVER_ERROR xr_server_error_quark()

typedef enum
{
  XR_SERVER_ERROR_FAILED
} XRServerError;

G_BEGIN_DECLS

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
 * @return Function returns FALSE on error, TRUE on success.
 */
gboolean xr_server_bind(xr_server* server, const char* port, GError** err);

/** Run server. This function will start listening for incomming
 * connections and push them to the thread pool where they are
 * handled individually.
 *
 * @param server Server object.
 * @param err Pointer to the variable to store error to on error.
 *
 * @return Function returns FALSE on fatal error or TRUE on safe stop
 *   by @ref xr_server_stop(). Otherwise it will block, waiting for 
 *   connections.
 */
gboolean xr_server_run(xr_server* server, GError** err);

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
 * @return Function returns FALSE on error, TRUE on success.
 */
gboolean xr_server_register_servlet(xr_server* server, xr_servlet_def* servlet);

/** Get private data for the servlet.
 *
 * @param servlet Servlet object.
 *
 * @return Returns private data associated with the servlet.
 */
void* xr_servlet_get_priv(xr_servlet* servlet);

/** Get http object for the servlet.
 *
 * @param servlet Servlet object.
 *
 * @return Returns HTTP object that can be used to upload/download data.
 */
xr_http* xr_servlet_get_http(xr_servlet* servlet);

/** Use this function as a simple way to quickly start a server.
 *
 * @param cert Combined PEM file with server certificate and private.
 * @param threads Number of threads in the pool.
 * @param bind Port and IP address to bind to.
 * @param servlets Servlet definition objects array (NULL termianted).
 * @param err Pointer to the variable to store error to on error.
 *
 * @return Function returns FALSE on error, TRUE on success.
 */
gboolean xr_server_simple(const char* cert, int threads, const char* bind,
  xr_servlet_def** servlets, GError** err);

GQuark xr_server_error_quark();

G_END_DECLS

#endif
