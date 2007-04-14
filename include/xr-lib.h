/** @file xr-lib.h
 *
 * XML-RPC Library Setup API
 *
 * Using this API you can turn on debuging and logging.
 */

#ifndef __XR_LIB_H__
#define __XR_LIB_H__

#include <glib.h>

enum
{
  XR_DEBUG_HTTP             = (1 << 0),
  XR_DEBUG_HTTP_TRACE       = (1 << 1),
  XR_DEBUG_SERVER           = (1 << 2),
  XR_DEBUG_SERVER_TRACE     = (1 << 3),
  XR_DEBUG_CLIENT           = (1 << 4),
  XR_DEBUG_CLIENT_TRACE     = (1 << 5),
  XR_DEBUG_SERVLET          = (1 << 6),
  XR_DEBUG_SERVLET_TRACE    = (1 << 7),
  XR_DEBUG_CALL             = (1 << 8),
  XR_DEBUG_CALL_TRACE       = (1 << 9),
  XR_DEBUG_LIB              = (1 << 10),
  XR_DEBUG_ALL              = 0xffffffff
};

extern int xr_debug_enabled;

#define xr_debug(mask, fmt, args...) \
  do { if (G_UNLIKELY(xr_debug_enabled & mask)) _xr_debug(G_STRFUNC, fmt ": ", ## args); } while(0)

#define xr_trace(mask, fmt, args...) \
  do { if (G_UNLIKELY(xr_debug_enabled & mask)) _xr_debug(G_STRFUNC, fmt, ## args); } while(0)

/** Log message.
 *
 * @param call Call obejct.
 *
 * @return Method name or NULL if not set.
 */
void _xr_debug(const char* loc, const char* fmt, ...);

#endif