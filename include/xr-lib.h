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

/** @file xr-lib.h
 *
 * XML-RPC Library Setup API
 *
 * Using this API you can turn on debuging and logging.
 */

#ifndef __XR_LIB_H__
#define __XR_LIB_H__

#include <glib.h>
#include "xr-config.h"

enum
{
  XR_DEBUG_HTTP             = (1 << 0),  /**< Dump HTTP requests/responses. */
  XR_DEBUG_HTTP_TRACE       = (1 << 1),  /**< Trace HTTP calls. */
  XR_DEBUG_SERVER           = (1 << 2),  /**< Not implemented. */
  XR_DEBUG_SERVER_TRACE     = (1 << 3),  /**< Trace xr-server.c code. */
  XR_DEBUG_CLIENT           = (1 << 4),  /**< Not implemented. */
  XR_DEBUG_CLIENT_TRACE     = (1 << 5),  /**< Trace xr-client.c code. */
  XR_DEBUG_SERVLET          = (1 << 6),  /**< Not implemented. */
  XR_DEBUG_SERVLET_TRACE    = (1 << 7),  /**< Not implemented. */
  XR_DEBUG_CALL             = (1 << 8),  /**< Dump RPC calls. */
  XR_DEBUG_CALL_TRACE       = (1 << 9),  /**< Trace xr-call.c code. */
  XR_DEBUG_VALUE            = (1 << 10),  /**< Debug xr_value parser/builder code. */
  XR_DEBUG_LIB              = (1 << 11), /**< Not implemented. */
  XR_DEBUG_ALL              = 0xffffffff
};

/** Global variable used to enable debugging messages.
 */
extern int xr_debug_enabled;

/** Conditional debug message.
 * 
 * @param mask XR_DEBUG_* constant that enables this message.
 * @param fmt Message format (printf like).
 * @param args Variable arguments.
 */
#define xr_debug(mask, fmt, args...) \
  do { if (G_UNLIKELY(xr_debug_enabled & mask)) _xr_debug(G_STRLOC ": ", fmt, ## args); } while(0)

/** Conditional trace message.
 * 
 * @param mask XR_DEBUG_* constant that enables this message.
 * @param fmt Message format (printf like).
 * @param args Variable arguments.
 */
#define xr_trace(mask, fmt, args...) \
  do { if (G_UNLIKELY(xr_debug_enabled & mask)) _xr_debug(G_STRFUNC, fmt, ## args); } while(0)

G_BEGIN_DECLS

/** Log message.
 *
 * @param loc Location.
 * @param fmt Message format (printf like).
 *
 * @return Method name or NULL if not set.
 */
void _xr_debug(const char* loc, const char* fmt, ...);

/** Initialize libxr.
 */
void xr_init();

/** Finalize libxr.
 *
 * Cleanup resources. Libxr functions should not be used after this call.
 */
void xr_fini();

G_END_DECLS

#endif
