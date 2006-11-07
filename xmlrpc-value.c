#include <string.h>
#include "xmlrpc-value.h"

xr_value* xr_value_struct_new()
{
  xr_value* v = g_new0(xr_value, 1);
  v->type = XRV_STRUCT;
  return v;
}

xr_value* xr_value_array_new()
{
  xr_value* v = g_new0(xr_value, 1);
  v->type = XRV_ARRAY;
  return v;
}

xr_value* xr_value_string_new(char* val)
{
  g_assert(val != NULL);
  xr_value* v = g_new0(xr_value, 1);
  v->type = XRV_STRING;
  v->str_val = val;
  return v;
}

xr_value* xr_value_int_new(int val)
{
  xr_value* v = g_new0(xr_value, 1);
  v->type = XRV_INT;
  v->int_val = val;
  return v;
}

xr_value* xr_value_bool_new(int val)
{
  xr_value* v = g_new0(xr_value, 1);
  v->type = XRV_BOOLEAN;
  v->int_val = val;
  return v;
}

xr_value* xr_value_double_new(double val)
{
  xr_value* v = g_new0(xr_value, 1);
  v->type = XRV_DOUBLE;
  v->dbl_val = val;
  return v;
}

xr_value* xr_value_time_new(char* val)
{
  g_assert(val != NULL);
  xr_value* v = g_new0(xr_value, 1);
  v->type = XRV_TIME;
  v->str_val = val;
  return v;
}

xr_value* xr_value_blob_new(char* val, int len)
{
  g_assert(val != NULL);
  xr_value* v = g_new0(xr_value, 1);
  v->type = XRV_BLOB;
  v->str_val = val;
  v->buf_len = len;
  return v;
}

void xr_value_struct_set_member(xr_value* str, char* name, xr_value* val)
{
  g_assert(str != NULL);
  g_assert(str->type == XRV_STRUCT);
  g_assert(val != NULL);
  GSList* i;
  for (i=str->children; i; i=i->next)
  {
    xr_value* m = i->data;
    if (!strcmp(m->member_name, name))
      return;
  }
  xr_value* v = g_new0(xr_value, 1);
  v->type = XRV_MEMBER;
  v->member_name = name;
  v->member_value = val;
  str->children = g_slist_append(str->children, v);
}

void xr_value_array_append(xr_value* arr, xr_value* val)
{
  g_assert(arr != NULL);
  g_assert(arr->type == XRV_ARRAY);
  g_assert(val != NULL);
  arr->children = g_slist_append(arr->children, val);
}

void xr_value_free(xr_value* val)
{
  g_free(val);
}
