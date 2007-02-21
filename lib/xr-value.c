#include <string.h>
#include <stdio.h>

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
  if (buf == NULL || len < 0)
    return NULL;
  xr_blob* b = g_new0(xr_blob, 1);
  b->buf = buf;
  b->len = len;
  return b;
}

void xr_blob_free(xr_blob* b)
{
  if (b == NULL)
    return;
  g_free(b->buf);
  g_free(b);
}

/* base types */
xr_value* xr_value_string_new(char* val)
{
  xr_value* v = g_new0(xr_value, 1);
  v->type = XRV_STRING;
  v->str_val = g_strdup(val != NULL ? val : "");
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
  xr_value* v = g_new0(xr_value, 1);
  v->type = XRV_TIME;
  v->str_val = g_strdup(val != NULL ? val : "");
  return v;
}

xr_value* xr_value_blob_new(xr_blob* val)
{
  if (val == NULL)
    return NULL;
  xr_value* v = g_new0(xr_value, 1);
  v->type = XRV_BLOB;
  v->blob_val = val;
  return v;
}

int xr_value_to_int(xr_value* val, int* nval)
{
  g_assert(nval != NULL);
  if (val == NULL || val->type != XRV_INT)
    return -1;
  *nval = val->int_val;
  return 0;
}

int xr_value_to_string(xr_value* val, char** nval)
{
  g_assert(nval != NULL);
  if (val == NULL || val->type != XRV_STRING)
    return -1;
  *nval = g_strdup(val->str_val);
  return 0;
}

int xr_value_to_bool(xr_value* val, int* nval)
{
  g_assert(nval != NULL);
  if (val == NULL || val->type != XRV_BOOLEAN)
    return -1;
  *nval = val->int_val;
  return 0;
}

int xr_value_to_double(xr_value* val, double* nval)
{
  g_assert(nval != NULL);
  if (val == NULL || val->type != XRV_DOUBLE)
    return -1;
  *nval = val->dbl_val;
  return 0;
}

int xr_value_to_time(xr_value* val, char** nval)
{
  g_assert(nval != NULL);
  if (val == NULL || val->type != XRV_TIME)
    return -1;
  *nval = g_strdup(val->str_val);
  return 0;
}

int xr_value_to_blob(xr_value* val, xr_blob** nval)
{
  g_assert(nval != NULL);
  if (val == NULL || val->type != XRV_BLOB)
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
  v->member_name = g_strdup(name);
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
  GSList* i;
  if (val == NULL)
    return;
  if (val->type == XRV_STRING || val->type == XRV_TIME)
    g_free(val->str_val);
  if (val->type == XRV_BLOB)
    xr_blob_free(val->blob_val);
  g_free(val->member_name);
  xr_value_free(val->member_value);
  for (i=val->children; i; i=i->next)
    xr_value_free((xr_value*)i->data);
  g_free(val);
}

int xr_value_is_error_retval(xr_value* v, int* errcode, char** errmsg)
{
  g_assert(v != NULL);
  g_assert(errcode != NULL);
  g_assert(errmsg != NULL);
  if (v->type != XRV_STRUCT)
    return 0;
  if (xr_value_to_int(xr_value_get_member(v, "faultCode"), errcode) < 0 ||
      xr_value_to_string(xr_value_get_member(v, "faultString"), errmsg) < 0)
    return 0;
  return 1;
}

void xr_value_dump(xr_value* v, int indent)
{
  g_assert(v != NULL);

  char buf[1024];
  memset(buf, 0, sizeof(buf));
  memset(buf, ' ', indent*2);
  printf("%s", buf);
  
  GSList* i;

  switch (xr_value_get_type(v))
  {
    case XRV_ARRAY:
      printf("ARRAY\n");
      for (i = xr_value_get_items(v); i; i = i->next)
        xr_value_dump(i->data, indent+1);
      printf("%sARRAY END\n", buf);
      break;
    case XRV_STRUCT:
      printf("STRUCT\n");
      for (i = xr_value_get_members(v); i; i = i->next)
        xr_value_dump(i->data, indent+1);
      printf("%sSTRUCT END\n", buf);
      break;
    case XRV_MEMBER:
      printf("MEMBER:%s\n", xr_value_get_member_name(v));
      xr_value_dump(xr_value_get_member_value(v), indent+1);
      break;
    case XRV_INT:
    {
      printf("INT:%d\n", v->int_val);
      break;
    }
    case XRV_STRING:
    {
      printf("STRING:%s\n", v->str_val);
      break;
    }
    case XRV_BOOLEAN:
    {
      printf("BOOLEAN:%s\n", v->int_val ? "true" : "false");
      break;
    }
    case XRV_DOUBLE:
    {
      printf("DOUBLE:%g\n", v->dbl_val);
      break;
    }
    case XRV_TIME:
    {
      printf("TIME:%s\n", v->str_val);
      break;
    }
    case XRV_BLOB:
    {
      printf("BLOB:\n");
      break;
    }
  }
}
