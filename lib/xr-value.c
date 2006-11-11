#include <string.h>

#include "xr-value.h"

struct _xr_value
{
  int type;                      /**< Type of the value. */

  // values
  char* str_val;
  int int_val;
  double dbl_val;
  xr_blob* blob_val;

  // array
  GSList* children;       /**< Members or array items list. */

  // struct member fields
  char* member_name;             /**< Struct member name. */
  xr_value* member_value; /**< Struct member value. */
};

xr_blob* xr_blob_new(char* buf, int len)
{
  xr_blob* b = g_new0(xr_blob, 1);
  b->buf = buf;
  b->len = len;
  return b;
}

void xr_blob_free(xr_blob* b)
{
  g_free(b->buf);
  g_free(b);
}

/* base types */
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

xr_value* xr_value_blob_new(xr_blob* val)
{
  g_assert(val != NULL);
  xr_value* v = g_new0(xr_value, 1);
  v->type = XRV_BLOB;
  v->blob_val = val;
  return v;
}

int xr_value_to_int(xr_value* val, int* nval)
{
  g_assert(val != NULL);
  g_assert(nval != NULL);
  if (val->type != XRV_INT)
    return -1;
  *nval = val->int_val;
  return 0;
}

int xr_value_to_string(xr_value* val, char** nval)
{
  g_assert(val != NULL);
  g_assert(nval != NULL);
  if (val->type != XRV_STRING)
    return -1;
  *nval = val->str_val;
  return 0;
}

int xr_value_to_bool(xr_value* val, int* nval)
{
  g_assert(val != NULL);
  g_assert(nval != NULL);
  if (val->type != XRV_BOOLEAN)
    return -1;
  *nval = val->int_val;
  return 0;
}

int xr_value_to_double(xr_value* val, double* nval)
{
  g_assert(val != NULL);
  g_assert(nval != NULL);
  if (val->type != XRV_DOUBLE)
    return -1;
  *nval = val->dbl_val;
  return 0;
}

int xr_value_to_time(xr_value* val, char** nval)
{
  g_assert(val != NULL);
  g_assert(nval != NULL);
  if (val->type != XRV_TIME)
    return -1;
  *nval = val->str_val;
  return 0;
}

int xr_value_to_blob(xr_value* val, xr_blob** nval)
{
  g_assert(val != NULL);
  g_assert(nval != NULL);
  if (val->type != XRV_BLOB)
    return -1;
  *nval = val->blob_val;
  return 0;
}

int xr_value_get_type(xr_value* val)
{
  g_assert(val != NULL);
  return val->type;
}

GSList* xr_value_get_members(xr_value* val)
{
  g_assert(val != NULL);
  g_assert(val->type == XRV_STRUCT);
  return val->children;
}

char* xr_value_get_member_name(xr_value* val)
{
  g_assert(val != NULL);
  g_assert(val->type == XRV_MEMBER);
  return val->member_name;
}

xr_value* xr_value_get_member_value(xr_value* val)
{
  g_assert(val != NULL);
  g_assert(val->type == XRV_MEMBER);
  return val->member_value;
}

xr_value* xr_value_get_member(xr_value* val, char* name)
{
  g_assert(val != NULL);
  g_assert(val->type == XRV_STRUCT);
  GSList* i;
  for (i = val->children; i; i = i->next)
  {
    xr_value* m = i->data;
    if (!strcmp(xr_value_get_member_name(m), name))
      return xr_value_get_member_value(m);
  }
  return NULL;
}

GSList* xr_value_get_items(xr_value* val)
{
  g_assert(val != NULL);
  g_assert(val->type == XRV_ARRAY);
  return val->children;
}

/* composite types */

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

int xr_value_is_error_retval(xr_value* v, int* errcode, char** errmsg)
{
  g_assert(v != NULL);
  if (v->type != XRV_STRUCT)
    return 0;
  xr_value* faultCode = xr_value_get_member(v, "faultCode");
  xr_value* faultString = xr_value_get_member(v, "faultString");
  if (faultCode && faultCode->type == XRV_INT && faultString && faultString->type == XRV_STRING)
  {
    xr_value_to_int(faultString, errcode);
    xr_value_to_string(faultString, errmsg);
    return 1;
  }
  return 0;
}
