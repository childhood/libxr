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

/** @file xr-call.h
 *
 * XML-RPC Call Handling API
 *
 * This code is used to prepare XML-RPC calls. It converts XML
 * string representation of XML-RPC requests and responses into
 * intermediate representation described in @ref xr-value.h that
 * is further used for conversion into native data types.
 *
 */

#ifndef __XR_CALL_H__
#define __XR_CALL_H__

#include "xr-value.h"

/** Transport type.
 */
typedef enum {
  XR_CALL_XML_RPC = 0,
#ifdef XR_JSON_ENABLED
  XR_CALL_JSON_RPC,
#endif
  XR_CALL_TRANSPORT_COUNT /* must be last, not a real transport */
} xr_call_transport;

/** Opaque data structrure for storing intermediate representation
 * of XML-RPC call.
 */
typedef struct _xr_call xr_call;

G_BEGIN_DECLS

/** Create new call obejct.
 *
 * @param method Method name. This may be NULL.
 *
 * @return Newly created call object.
 */
xr_call* xr_call_new(const char* method);

/** Free call object.
 *
 * @param call Call object.
 */
void xr_call_free(xr_call* call);

/** Set transport type (default is XR_CALL_XML_RPC).
 *
 * @param call Call object.
 * @param transport Transport type.
 */
void xr_call_set_transport(xr_call* call, xr_call_transport transport);

/** Get method name  (second part if in Servlet.Method format).
 *
 * @param call Call obejct.
 *
 * @return Method name or NULL if not set.
 *
 * @warning Returned value is still owned by the call object. Don't free it!
 */
const char* xr_call_get_method(xr_call* call);

/** Get method name (as passed in XML-RPC).
 *
 * @param call Call obejct.
 *
 * @return Method name or NULL if not set.
 *
 * @warning Returned value is still owned by the call object. Don't free it!
 */
const char* xr_call_get_method_full(xr_call* call);

/** Get servlet name (as passed in XML-RPC).
 * 
 * @param call Call obejct.
 * @param fallback Fallback value.
 * 
 * @return Servlet name. Caller must free it.
 */
char* xr_call_get_servlet_name(xr_call* call, const char* fallback);

/** Add parameter to the call obejct.
 *
 * @param call Call obejct.
 * @param val Parameter value. Call object takes ownership of this
 *   value and @ref xr_value_unref will be called from @ref xr_call_free.
 */
void xr_call_add_param(xr_call* call, xr_value* val);

/** Get parameter from specified position (counts from 0).
 *
 * @param call Call obejct.
 * @param pos Parameter position (counts from 0).
 *
 * @return Value node or NULL if parameter does not exist.
 *
 * @warning Returned value is still owned by the call object. Don't free it!
 */
xr_value* xr_call_get_param(xr_call* call, unsigned int pos);

/** Set return value of the XML-RPC call.
 *
 * @param call Call obejct.
 * @param val Retuirn value node. Call object takes ownership of this
 *   value and @ref xr_value_unref will be called from @ref xr_call_free.
 */
void xr_call_set_retval(xr_call* call, xr_value* val);

/** Get return value of the XML-RPC call.
 *
 * @param call Call obejct.
 *
 * @return Return value node or NULL if it is not set.
 *
 * @warning Returned value is still owned by the call object. Don't free it!
 */
xr_value* xr_call_get_retval(xr_call* call);

/** Set retval to be stadard XML-RPC error structure. If error is set
 * and retval is set too, error gets preference on serialize response.
 *
 * @param call Call obejct.
 * @param code Error code.
 * @param msg Error message.
 */
void xr_call_set_error(xr_call* call, int code, const char* msg, ...);

/** Get error code that is set on the call object.
 *
 * @param call Call obejct.
 *
 * @return Error code.
 */
int xr_call_get_error_code(xr_call* call);

/** Get error message that is set on the call object.
 *
 * @param call Call obejct.
 *
 * @return Error message. String is owned by the call obejct.
 */
const char* xr_call_get_error_message(xr_call* call);

/** Serialize call object into XML-RPC request.
 *
 * @param call Call obejct.
 * @param buf Pointer to the variable to store buffer pointer to.
 *   This buffer has to be freed using @ref xr_call_free_buffer.
 * @param len Pointer to the variable to store buffer length to.
 */
void xr_call_serialize_request(xr_call* call, char** buf, int* len);

/** Serialize call object into XML-RPC response.
 *
 * @param call Call obejct.
 * @param buf Pointer to the variable to store buffer pointer to.
 *   This buffer has to be freed using @ref xr_call_free_buffer.
 * @param len Pointer to the variable to store buffer length to.
 */
void xr_call_serialize_response(xr_call* call, char** buf, int* len);

/** Unserialize XML-RPC request into call object.
 *
 * @param call Call obejct.
 * @param buf Pointer to the buffer with XML-RPC request.
 * @param len Length of the buffer.
 *
 * @return Returns TRUE on success and FALSE on failure. On failure
 *   @ref xr_call_set_error is used to set reason of the failure.
 */
gboolean xr_call_unserialize_request(xr_call* call, const char* buf, int len);

/** Unserialize XML-RPC response into call object.
 *
 * @param call Call obejct.
 * @param buf Pointer to the buffer with XML-RPC response.
 * @param len Length of the buffer.
 *
 * @return Returns TRUE on success and FALSE on failure. On failure
 *   @ref xr_call_set_error is used to set reason of the failure.
 */
gboolean xr_call_unserialize_response(xr_call* call, const char* buf, int len);

/** Free buffer allocated by serialize functions.
 *
 * @param call Call obejct.
 * @param buf Buffer pointer.
 */
void xr_call_free_buffer(xr_call* call, char* buf);

/** Debugging function that dumps call object to the string.
 *
 * @param call Call obejct.
 * @param indent Indent level, usually 0.
 *
 * @return String representing the call.
 */
char* xr_call_dump_string(xr_call* call, int indent);

/** Debugging function that prints call object using g_print().
 *
 * @param call Call obejct.
 * @param indent Indent level, usually 0.
 */
void xr_call_dump(xr_call* call, int indent);

G_END_DECLS

#endif
