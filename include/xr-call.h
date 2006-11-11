#ifndef __XR_CALL_H__
#define __XR_CALL_H__

#include <glib.h>

#include "xr-value.h"

/** @file xmlrpc_call Header
 */

typedef struct _xr_call xr_call;

xr_call* xr_call_new();
void xr_call_free(xr_call* call);

void xr_call_add_param(xr_call* call, xr_value* val);
xr_value* xr_call_get_param(xr_call* call, unsigned int pos);

void xr_call_set_retval(xr_call* call, xr_value* val);
xr_value* xr_call_get_retval(xr_call* call);

void xr_call_set_error(xr_call* call, int code, char* msg);
int xr_call_get_error_code(xr_call* call);
char* xr_call_get_error_message(xr_call* call);

void xr_call_serialize_request(xr_call* call, char** buf, int* len);
void xr_call_serialize_response(xr_call* call, char** buf, int* len);
int xr_call_unserialize_request(xr_call* call, char* buf, int len);
int xr_call_unserialize_response(xr_call* call, char* buf, int len);
void xr_call_free_buffer(char* buf);


#endif

