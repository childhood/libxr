#define __STRICT_ANSI__
#include <json.h>
#undef __STRICT_ANSI__

static struct json_object* _xr_value_serialize_json(xr_value* val)
{
  GSList* i;

  switch (xr_value_get_type(val))
  {
    case XRV_ARRAY:
    {
      struct json_object* array = json_object_new_array();
      for (i = xr_value_get_items(val); i; i = i->next)
        json_object_array_add(array, _xr_value_serialize_json(i->data));
      return array;
    }
    case XRV_STRUCT:
    {
      struct json_object* obj = json_object_new_object();
      for (i = xr_value_get_members(val); i; i = i->next)
        json_object_object_add(obj, (char*)xr_value_get_member_name(i->data), _xr_value_serialize_json(xr_value_get_member_value(i->data)));
      return obj;
    }
    case XRV_INT:
    {
      int int_val = -1;
      xr_value_to_int(val, &int_val);
      return json_object_new_int(int_val);
    }
    case XRV_STRING:
    {
      char* str_val = NULL;
      xr_value_to_string(val, &str_val);
      struct json_object* tmp = json_object_new_string(str_val);
      g_free(str_val);
      return tmp;
    }
    case XRV_BOOLEAN:
    {
      int bool_val = -1;
      xr_value_to_bool(val, &bool_val);
      return json_object_new_boolean(bool_val);
    }
    case XRV_DOUBLE:
    {
      double dbl_val = -1;
      xr_value_to_double(val, &dbl_val);
      return json_object_new_double(dbl_val);
    }
    case XRV_TIME:
    {
      char* str_val = NULL;
      xr_value_to_time(val, &str_val);
      struct json_object* tmp = json_object_new_string(str_val);
      g_free(str_val);
      return tmp;
    }
    case XRV_BLOB:
    {
      char* data = NULL;
      xr_blob* b = NULL;
      xr_value_to_blob(val, &b);
      data = g_base64_encode(b->buf, b->len);
      xr_blob_unref(b);
      struct json_object* tmp = json_object_new_string(data);
      g_free(data);
      return tmp;
    }
  }

  return NULL;
}

static xr_value* _xr_value_unserialize_json(struct json_object* obj)
{
  switch (json_object_get_type(obj))
  {
    case json_type_null:
      g_return_val_if_reached(NULL);

    case json_type_boolean:
      return xr_value_bool_new(json_object_get_boolean(obj));

    case json_type_double:
      return xr_value_double_new(json_object_get_double(obj));

    case json_type_int:
      return xr_value_int_new(json_object_get_int(obj));

    case json_type_object:
    {
      xr_value* str = xr_value_struct_new();
      json_object_object_foreach(obj, key, val)
        xr_value_struct_set_member(str, key, _xr_value_unserialize_json(val));
      return str;
    }

    case json_type_array:
    {
      int i;
      xr_value* arr = xr_value_array_new();
      for (i = 0; i < json_object_array_length(obj); i++) 
        xr_value_array_append(arr, _xr_value_unserialize_json(json_object_array_get_idx(obj, i)));
      return arr;
    }

    case json_type_string:
      return xr_value_string_new(json_object_get_string(obj));
  }

  g_return_val_if_reached(NULL);
}

static void xr_call_serialize_request_json(xr_call* call, char** buf, int* len)
{
  GSList* i;
  struct json_object *r, *params;

  r = json_object_new_object();
  json_object_object_add(r, "method", json_object_new_string(call->method));
  json_object_object_add(r, "params", params = json_object_new_array());
  json_object_object_add(r, "id", json_object_new_string("1"));

  for (i = call->params; i; i = i->next)
    json_object_array_add(params, _xr_value_serialize_json(i->data));

  *buf = g_strdup(json_object_to_json_string(r));
  *len = strlen(*buf);

  json_object_put(r);
}

static void xr_call_serialize_response_json(xr_call* call, char** buf, int* len)
{
  GSList* i;
  struct json_object *r, *error;

  if (call->error_set)
  {
    r = json_object_new_object();
    json_object_object_add(r, "result", NULL);
    json_object_object_add(r, "error", error = json_object_new_object());
    json_object_object_add(r, "id", json_object_new_string("1"));
    json_object_object_add(error, "code", json_object_new_int(call->errcode));
    json_object_object_add(error, "message", json_object_new_string(call->errmsg));
  }
  else if (call->retval)
  {
    r = json_object_new_object();
    json_object_object_add(r, "result", _xr_value_serialize_json(call->retval));
    json_object_object_add(r, "error", NULL);
    json_object_object_add(r, "id", json_object_new_string("1"));
  }
  else
    g_return_if_reached();

  *buf = g_strdup(json_object_to_json_string(r));
  *len = strlen(*buf);

  json_object_put(r);
}

static gboolean xr_call_unserialize_request_json(xr_call* call, const char* buf, int len)
{
  struct json_tokener* t;
  struct json_object* r;
  int i;

  t = json_tokener_new();
  r = json_tokener_parse_ex(t, (char*)buf, len);
  json_tokener_free(t);

  if (r == NULL)
  {
    xr_call_set_error(call, -1, "Can't parse JSON-RPC request. Invalid JSON object.");
    return FALSE;
  }

  call->method = g_strdup(json_object_get_string(json_object_object_get(r, "method")));
  if (call->method == NULL)
  {
    xr_call_set_error(call, -1, "Can't parse JSON-RPC request. Missing method.");
    json_object_put(r);
    return FALSE;
  }

  struct json_object* params = json_object_object_get(r, "params");
  if (params && !json_object_is_type(params, json_type_array))
  {
    xr_call_set_error(call, -1, "Can't parse JSON-RPC request. Invalid params.");
    json_object_put(r);
    return FALSE;
  }

  for (i = 0; i < json_object_array_length(params); i++) 
  {
    xr_value* v = _xr_value_unserialize_json(json_object_array_get_idx(params, i));
    if (v == NULL)
    {
      char* msg = g_strdup_printf("Can't parse JSON-RPC request. Failed to unserialize parameter %d.", i);
      xr_call_set_error(call, -1, msg);
      g_free(msg);
      json_object_put(r);
      return FALSE;
    }

    xr_call_add_param(call, v);
  }

  json_object_put(r);
  return TRUE;
}

static gboolean xr_call_unserialize_response_json(xr_call* call, const char* buf, int len)
{
  struct json_tokener* t;
  struct json_object* r;
  int i;

  t = json_tokener_new();
  r = json_tokener_parse_ex(t, (char*)buf, len);
  json_tokener_free(t);

  if (r == NULL)
  {
    xr_call_set_error(call, -1, "Can't parse JSON-RPC response. Invalid JSON object.");
    return FALSE;
  }

  struct json_object* error = json_object_object_get(r, "error");
  if (error && !json_object_is_type(error, json_type_null))
  {
    if (json_object_is_type(error, json_type_object))
    {
      struct json_object* code = json_object_object_get(error, "code");
      struct json_object* message = json_object_object_get(error, "message");
      if (code && message && json_object_is_type(code, json_type_int) && json_object_is_type(message, json_type_string))
        xr_call_set_error(call, json_object_get_int(code), json_object_get_string(message));
      else
        xr_call_set_error(call, -1, "Can't parse JSON-RPC response. Invalid error object.");
    }
    else
      xr_call_set_error(call, -1, "Can't parse JSON-RPC response. Invalid error object.");

    json_object_put(r);
    return FALSE;
  }

  struct json_object* result = json_object_object_get(r, "result");
  if (result && json_object_is_type(result, json_type_null))
  {
    xr_call_set_error(call, -1, "Can't parse JSON-RPC response. Null result.");
    json_object_put(r);
    return FALSE;
  }

  xr_value* v = _xr_value_unserialize_json(result);
  if (v == NULL)
  {
    xr_call_set_error(call, -1, "Can't parse JSON-RPC response. Invalid result.");
    json_object_put(r);
    return FALSE;
  }

  xr_call_set_retval(call, v);
  json_object_put(r);
  return TRUE;
}

static void xr_call_free_buffer_json(xr_call* call, char* buf)
{
  g_free(buf);
}
