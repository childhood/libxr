#ifndef __XR_CALL_H__
#define __XR_CALL_H__

#include <glib.h>

#include "xr-value.h"

/** @file xmlrpc_call Header
 */

struct _xr_call
{
  char* method;
  GSList* params;
  xr_value* retval;
  xr_value* errval;
};

xr_call* xr_call_new();
void xr_call_add_param(xr_call* call, xr_value* val);
void xr_call_set_retval(xr_call* call, xr_value* val);
void xr_call_set_errval(xr_call* call, int code, char* msg);
int xr_call_serialize_request(xr_call* call, char** buf, int* len);
int xr_call_serialize_response(xr_call* call, char** buf, int* len);
int xr_call_unserialize_request(xr_call* call, char* buf, int len);
int xr_call_unserialize_response(xr_call* call, char* buf, int len);
void xr_call_free_buffer(char* buf);
void xr_call_free(xr_call* call);

#endif

