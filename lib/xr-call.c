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

#include <string.h>
#include <stdio.h>

#include "xr-call.h"

struct _xr_call
{
  xr_call_transport transport;
  char* method;
  GSList* params;
  xr_value* retval;

  gboolean error_set;
  int errcode;    /* this must be > 0 for errors */
  char* errmsg;   /* Non-NULL on error. */
};

/* construct/destruct */

xr_call* xr_call_new(const char* method)
{
  xr_call* c = g_new0(xr_call, 1);
  c->method = g_strdup(method);
  c->transport = XR_CALL_XML_RPC;

  xr_trace(XR_DEBUG_CALL_TRACE, "(method=%s) = %p", method, c);

  return c;
}

void xr_call_free(xr_call* call)
{
  xr_trace(XR_DEBUG_CALL_TRACE, "(call=%p)", call);

  if (call == NULL)
    return;

  g_free(call->method);
  g_slist_foreach(call->params, (GFunc)xr_value_unref, NULL);
  g_slist_free(call->params);
  xr_value_unref(call->retval);
  g_free(call->errmsg);
  g_free(call);
}

void xr_call_set_transport(xr_call* call, xr_call_transport transport)
{
  xr_trace(XR_DEBUG_CALL_TRACE, "(call=%p, transport=%d)", call, transport);

  g_return_if_fail(call != NULL);
  g_return_if_fail(transport < XR_CALL_TRANSPORT_COUNT);

  call->transport = transport;
}

const char* xr_call_get_method_full(xr_call* call)
{
  xr_trace(XR_DEBUG_CALL_TRACE, "(call=%p)", call);

  g_return_val_if_fail(call != NULL, NULL);

  return call->method;
}

const char* xr_call_get_method(xr_call* call)
{
  xr_trace(XR_DEBUG_CALL_TRACE, "(call=%p)", call);

  g_return_val_if_fail(call != NULL, NULL);

  if (call->method == NULL)
    return NULL;

  if (strchr(call->method, '.'))
    return strchr(call->method, '.') + 1;

  return call->method;
}

char* xr_call_get_servlet_name(xr_call* call, const char* fallback)
{
  char* servlet_name;

  xr_trace(XR_DEBUG_CALL_TRACE, "(call=%p)", call);

  g_return_val_if_fail(call != NULL, NULL);

  if (call->method == NULL || strchr(call->method, '.') == NULL)
    return g_strdup(fallback);

  servlet_name = g_strdup(call->method);
  *strchr(servlet_name, '.') = '\0';

  return servlet_name;
}

/* param manipulation */

void xr_call_add_param(xr_call* call, xr_value* val)
{
  xr_trace(XR_DEBUG_CALL_TRACE, "(call=%p, val=%p)", call, val);

  g_return_if_fail(call != NULL);
  g_return_if_fail(val != NULL);

  call->params = g_slist_append(call->params, val);
}

xr_value* xr_call_get_param(xr_call* call, unsigned int pos)
{
  xr_trace(XR_DEBUG_CALL_TRACE, "(call=%p, pos=%u)", call, pos);

  g_return_val_if_fail(call != NULL, NULL);

  return g_slist_nth_data(call->params, pos);
}

/* retval manipulation */

void xr_call_set_retval(xr_call* call, xr_value* val)
{
  xr_trace(XR_DEBUG_CALL_TRACE, "(call=%p, val=%p)", call, val);

  g_return_if_fail(call != NULL);
  g_return_if_fail(val != NULL);

  xr_value_unref(call->retval);
  call->retval = val;
}

xr_value* xr_call_get_retval(xr_call* call)
{
  xr_trace(XR_DEBUG_CALL_TRACE, "(call=%p)", call);

  g_return_val_if_fail(call != NULL, NULL);

  return call->retval;
}

/* error manipulation */

void xr_call_set_error(xr_call* call, int code, const char* msg, ...)
{
  va_list args;

  xr_trace(XR_DEBUG_CALL_TRACE, "(call=%p, code=%d, msg=%s)", call, code, msg);

  g_return_if_fail(call != NULL);

  call->error_set = TRUE;
  call->errcode = code;

  va_start(args, msg);
  g_free(call->errmsg);
  call->errmsg = g_strdup_vprintf(msg, args);
  va_end(args);
}

int xr_call_get_error_code(xr_call* call)
{
  xr_trace(XR_DEBUG_CALL_TRACE, "(call=%p)", call);

  g_return_val_if_fail(call != NULL, -1);

  return call->errcode;
}

const char* xr_call_get_error_message(xr_call* call)
{
  xr_trace(XR_DEBUG_CALL_TRACE, "(call=%p)", call);

  g_return_val_if_fail(call != NULL, NULL);

  return call->errmsg;
}

/* transport specific API */

#include "xr-call-xml-rpc.c"
#ifdef XR_JSON_ENABLED
#include "xr-call-json-rpc.c"
#endif

struct transport_module
{
  void (*serialize_request)(xr_call* call, char** buf, int* len);
  void (*serialize_response)(xr_call* call, char** buf, int* len);
  void (*free_buffer)(xr_call* call, char* buf);
  gboolean (*unserialize_request)(xr_call* call, const char* buf, int len);
  gboolean (*unserialize_response)(xr_call* call, const char* buf, int len);
};

const struct transport_module transports[XR_CALL_TRANSPORT_COUNT] = {
  { /* XR_CALL_XML_RPC */
    .serialize_request = xr_call_serialize_request_xmlrpc,
    .serialize_response = xr_call_serialize_response_xmlrpc,
    .free_buffer = xr_call_free_buffer_xmlrpc,
    .unserialize_request = xr_call_unserialize_request_xmlrpc,
    .unserialize_response = xr_call_unserialize_response_xmlrpc,
  },
#ifdef XR_JSON_ENABLED
  { /* XR_CALL_JSON_RPC */
    .serialize_request = xr_call_serialize_request_json,
    .serialize_response = xr_call_serialize_response_json,
    .free_buffer = xr_call_free_buffer_json,
    .unserialize_request = xr_call_unserialize_request_json,
    .unserialize_response = xr_call_unserialize_response_json,
  },
#endif
};

void xr_call_serialize_request(xr_call* call, char** buf, int* len)
{
  g_return_if_fail(call != NULL);
  g_return_if_fail(call->method != NULL);
  g_return_if_fail(buf != NULL);
  g_return_if_fail(len != NULL);

  transports[call->transport].serialize_request(call, buf, len);

  xr_trace(XR_DEBUG_CALL_TRACE, "(call=%p, *buf=%p, *len=%d)", call, *buf, *len);
}

void xr_call_serialize_response(xr_call* call, char** buf, int* len)
{
  g_return_if_fail(call != NULL);
  g_return_if_fail(buf != NULL);
  g_return_if_fail(len != NULL);

  transports[call->transport].serialize_response(call, buf, len);

  xr_trace(XR_DEBUG_CALL_TRACE, "(call=%p, *buf=%p, *len=%d)", call, *buf, *len);
}

void xr_call_free_buffer(xr_call* call, char* buf)
{
  g_return_if_fail(call != NULL);

  transports[call->transport].free_buffer(call, buf);
}

gboolean xr_call_unserialize_request(xr_call* call, const char* buf, int len)
{
  xr_trace(XR_DEBUG_CALL_TRACE, "(call=%p, buf=%p, len=%d)", call, buf, len);
  
  g_return_val_if_fail(call != NULL, FALSE);
  g_return_val_if_fail(buf != NULL, FALSE);

  if (len < 0)
    len = strlen(buf);

  return transports[call->transport].unserialize_request(call, buf, len);
}

gboolean xr_call_unserialize_response(xr_call* call, const char* buf, int len)
{
  xr_trace(XR_DEBUG_CALL_TRACE, "(call=%p, buf=%p, len=%d)", call, buf, len);
  
  g_return_val_if_fail(call != NULL, FALSE);
  g_return_val_if_fail(buf != NULL, FALSE);

  if (len < 0)
    len = strlen(buf);

  return transports[call->transport].unserialize_response(call, buf, len);
}

/* internal use only */
gboolean __xr_value_is_complicated(xr_value* v, int max_strlen);

char* xr_call_dump_string(xr_call* call, int indent)
{
  GSList* i;
  GString* string;
  char buf[256];
  gboolean single_line = TRUE;

  g_return_val_if_fail(call != NULL, NULL);

  memset(buf, 0, sizeof(buf));
  memset(buf, ' ', MIN(indent * 2, sizeof(buf) - 1));
  string = g_string_sized_new(1024);

  // split parameters on spearate lines?
  if (g_slist_length(call->params) > 6)
    single_line = FALSE;
  else
    for (i = call->params; i; i = i->next)
      if (__xr_value_is_complicated(i->data, 25))
        single_line = FALSE;

  g_string_append_printf(string, "%s%s(", buf, call->method ? call->method : "<anonymous>");
  if (single_line)
  {
    for (i = call->params; i; i = i->next)
    {
      xr_value_dump(i->data, string, indent);
      if (i->next)
        g_string_append(string, ", ");
    }
  }
  else
  {
    for (i = call->params; i; i = i->next)
    {
      g_string_append_printf(string, "\n%s  ", buf);
      xr_value_dump(i->data, string, indent + 1);
      if (i->next)
        g_string_append(string, ",");
    }
    g_string_append_printf(string, "\n%s", buf);
  }
  g_string_append(string, ")");

  if (call->retval)
  {
    g_string_append(string, " = ");
    xr_value_dump(call->retval, string, indent);
  }
  if (call->errcode || call->errmsg)
  {
    g_string_append_printf(string, " = { faultCode: %d, faultString: \"%s\" }", call->errcode, call->errmsg ? call->errmsg : "");
  }

  return g_string_free(string, FALSE);
}

void xr_call_dump(xr_call* call, int indent)
{
  char* str;

  g_return_if_fail(call != NULL);

  str = xr_call_dump_string(call, indent);
  g_print("%s\n", str);
  g_free(str);
}
