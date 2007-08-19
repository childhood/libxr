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

/** Opaque data structrure for storing intermediate representation
 * of XML-RPC call.
 */
typedef struct _xr_call xr_call;

/** Create new call obejct.
 *
 * @param method Method name. This may be NULL.
 *
 * @return Newly created call object.
 */
xr_call* xr_call_new(char* method);

/** Free call object.
 *
 * @param call Call object.
 */
void xr_call_free(xr_call* call);

/** Get method name  (second part if in Servlet.Method format).
 *
 * @param call Call obejct.
 *
 * @return Method name or NULL if not set.
 *
 * @warning Returned value is still owned by the call object. Don't free it!
 */
char* xr_call_get_method(xr_call* call);

/** Get method name (as passed in XML-RPC).
 *
 * @param call Call obejct.
 *
 * @return Method name or NULL if not set.
 *
 * @warning Returned value is still owned by the call object. Don't free it!
 */
char* xr_call_get_method_full(xr_call* call);

/** Add parameter to the call obejct.
 *
 * @param call Call obejct.
 * @param val Parameter value. Call object takes ownership of this
 *   value and @ref xr_value_free will be called from @ref xr_call_free.
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
 *   value and @ref xr_value_free will be called from @ref xr_call_free.
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
void xr_call_set_error(xr_call* call, int code, char* msg);

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
char* xr_call_get_error_message(xr_call* call);

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
 * @return Returns 0 on success and -1 on failure. On failure
 *   @ref xr_call_set_error is used to set reason of the failure.
 */
int xr_call_unserialize_request(xr_call* call, char* buf, int len);

/** Unserialize XML-RPC response into call object.
 *
 * @param call Call obejct.
 * @param buf Pointer to the buffer with XML-RPC response.
 * @param len Length of the buffer.
 *
 * @return Returns 0 on success and -1 on failure. On failure
 *   @ref xr_call_set_error is used to set reason of the failure.
 */
int xr_call_unserialize_response(xr_call* call, char* buf, int len);

/** Free buffer allocated by serialize functions.
 *
 * @param buf Buffer pointer.
 */
void xr_call_free_buffer(char* buf);

/** Debugging function that dumps call object to the stdout.
 *
 * @param call Call obejct.
 * @param indent Indent level, usually 0.
 */
void xr_call_dump(xr_call* call, int indent);

#endif
