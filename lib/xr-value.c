/* 
 * Copyright 2006, 2007 Ondrej Jirman <ondrej.jirman@zonio.net>
 * 
 * This file is part of libxr.
 *
 * Libxr is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
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

#include "xr-value.h"

struct _xr_value
{
  int type;                      /**< Type of the value. */
  volatile guint ref;

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

static xr_value* _xr_value_new()
{
  xr_value* v = g_slice_new0(xr_value);
  return xr_value_ref(v);
}

xr_blob* xr_blob_new(char* buf, int len)
{
  if (buf == NULL)
    return NULL;

  xr_blob* b = g_new0(xr_blob, 1);
  b->buf = buf;
  b->len = len < 0 ? strlen(buf) : len;
  b->refs = 1;
  return b;
}

xr_blob* xr_blob_ref(xr_blob* b)
{
  if (b == NULL)
    return NULL;

  b->refs++;
  return b;
}

void xr_blob_unref(xr_blob* b)
{
  if (b == NULL)
    return;

  if (--b->refs == 0)
  {
    g_free(b->buf);
    g_free(b);
  }
}

xr_value* xr_value_ref(xr_value* val)
{
  if (val == NULL)
    return NULL;

  g_atomic_int_inc(&val->ref);
  return val;
}

/*XXX: this is probably not a thread-safe way of doing unref */
void xr_value_unref(xr_value* val)
{
  if (val == NULL)
    return;

  if (g_atomic_int_dec_and_test(&val->ref))
  {
    GSList* i;

    if (val->type == XRV_BLOB)
      xr_blob_unref(val->blob_val);

    g_free(val->str_val);
    g_free(val->member_name);
    xr_value_unref(val->member_value);
    g_slist_foreach(val->children, (GFunc)xr_value_unref, NULL);
    g_slist_free(val->children);
    g_slice_free(xr_value, val);
  }
}

/* base types */
xr_value* xr_value_string_new(const char* val)
{
  xr_value* v = _xr_value_new();
  v->type = XRV_STRING;
  v->str_val = g_strdup(val != NULL ? val : "");
  return v;
}

xr_value* xr_value_int_new(int val)
{
  xr_value* v = _xr_value_new();
  v->type = XRV_INT;
  v->int_val = val;
  return v;
}

xr_value* xr_value_bool_new(int val)
{
  xr_value* v = _xr_value_new();
  v->type = XRV_BOOLEAN;
  v->int_val = val;
  return v;
}

xr_value* xr_value_double_new(double val)
{
  xr_value* v = _xr_value_new();
  v->type = XRV_DOUBLE;
  v->dbl_val = val;
  return v;
}

xr_value* xr_value_time_new(const char* val)
{
  xr_value* v = _xr_value_new();
  v->type = XRV_TIME;
  v->str_val = g_strdup(val != NULL ? val : "");
  return v;
}

xr_value* xr_value_blob_new(xr_blob* val)
{
  if (val == NULL)
    return NULL;

  xr_value* v = _xr_value_new();
  v->type = XRV_BLOB;
  v->blob_val = xr_blob_ref(val);
  return v;
}

gboolean xr_value_to_int(xr_value* val, int* nval)
{
  g_return_val_if_fail(nval != NULL, FALSE);

  if (val == NULL || val->type != XRV_INT)
    return FALSE;

  *nval = val->int_val;
  return TRUE;
}

gboolean xr_value_to_string(xr_value* val, char** nval)
{
  g_return_val_if_fail(nval != NULL, FALSE);

  if (val == NULL || val->type != XRV_STRING)
    return FALSE;

  *nval = g_strdup(val->str_val);
  return TRUE;
}

gboolean xr_value_to_bool(xr_value* val, int* nval)
{
  g_return_val_if_fail(nval != NULL, FALSE);

  if (val == NULL || val->type != XRV_BOOLEAN)
    return FALSE;

  *nval = val->int_val;
  return TRUE;
}

gboolean xr_value_to_double(xr_value* val, double* nval)
{
  g_return_val_if_fail(nval != NULL, FALSE);

  if (val == NULL || val->type != XRV_DOUBLE)
    return FALSE;

  *nval = val->dbl_val;
  return TRUE;
}

gboolean xr_value_to_time(xr_value* val, char** nval)
{
  g_return_val_if_fail(nval != NULL, FALSE);

  if (val == NULL || val->type != XRV_TIME)
    return FALSE;

  *nval = g_strdup(val->str_val);
  return TRUE;
}

gboolean xr_value_to_blob(xr_value* val, xr_blob** nval)
{
  g_return_val_if_fail(nval != NULL, FALSE);

  if (val == NULL || val->type != XRV_BLOB)
    return FALSE;

  *nval = xr_blob_ref(val->blob_val);
  return TRUE;
}

gboolean xr_value_to_value(xr_value* val, xr_value** nval)
{
  g_return_val_if_fail(nval != NULL, FALSE);

  if (val == NULL)
    return FALSE;

  *nval = xr_value_ref(val);
  return TRUE;
}

int xr_value_get_type(xr_value* val)
{
  g_return_val_if_fail(val != NULL, -1);

  return val->type;
}

GSList* xr_value_get_members(xr_value* val)
{
  g_return_val_if_fail(val != NULL, NULL);
  g_return_val_if_fail(val->type == XRV_STRUCT, NULL);

  return val->children;
}

const char* xr_value_get_member_name(xr_value* val)
{
  g_return_val_if_fail(val != NULL, NULL);
  g_return_val_if_fail(val->type == XRV_MEMBER, NULL);

  return val->member_name;
}

xr_value* xr_value_get_member_value(xr_value* val)
{
  g_return_val_if_fail(val != NULL, NULL);
  g_return_val_if_fail(val->type == XRV_MEMBER, NULL);

  return val->member_value;
}

xr_value* xr_value_get_member(xr_value* val, const char* name)
{
  GSList* i;

  g_return_val_if_fail(val != NULL, NULL);
  g_return_val_if_fail(val->type == XRV_STRUCT, NULL);

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
  g_return_val_if_fail(val != NULL, NULL);
  g_return_val_if_fail(val->type == XRV_ARRAY, NULL);

  return val->children;
}

/* composite types */

xr_value* xr_value_struct_new()
{
  xr_value* v = _xr_value_new();
  v->type = XRV_STRUCT;
  return v;
}

xr_value* xr_value_array_new()
{
  xr_value* v = _xr_value_new();
  v->type = XRV_ARRAY;
  return v;
}

void xr_value_struct_set_member(xr_value* str, const char* name, xr_value* val)
{
  GSList* i;

  g_return_if_fail(str != NULL);
  g_return_if_fail(str->type == XRV_STRUCT);
  g_return_if_fail(val != NULL);

  for (i=str->children; i; i=i->next)
  {
    xr_value* m = i->data;
    if (!strcmp(m->member_name, name))
    {
      xr_value_unref(m->member_value);
      m->member_value = val;
      return;
    }
  }

  xr_value* v = _xr_value_new();
  v->type = XRV_MEMBER;
  v->member_name = g_strdup(name);
  v->member_value = val;
  str->children = g_slist_append(str->children, v);
}

void xr_value_array_append(xr_value* arr, xr_value* val)
{
  g_return_if_fail(arr != NULL);
  g_return_if_fail(arr->type == XRV_ARRAY);
  g_return_if_fail(val != NULL);

  arr->children = g_slist_append(arr->children, val);
}

gboolean xr_value_is_error_retval(xr_value* v, int* errcode, char** errmsg)
{
  g_return_val_if_fail(v != NULL, FALSE);
  g_return_val_if_fail(errcode != NULL, FALSE);
  g_return_val_if_fail(errmsg != NULL, FALSE);

  if (v->type != XRV_STRUCT)
    return FALSE;

  if (!xr_value_to_int(xr_value_get_member(v, "faultCode"), errcode) ||
      !xr_value_to_string(xr_value_get_member(v, "faultString"), errmsg))
    return FALSE;

  return TRUE;
}

gboolean __xr_value_is_complicated(xr_value* v, int max_strlen)
{
  return (xr_value_get_type(v) == XRV_STRUCT && xr_value_get_members(v))
      || (xr_value_get_type(v) == XRV_ARRAY && xr_value_get_items(v))
      || (xr_value_get_type(v) == XRV_STRING && v->str_val && strlen(v->str_val) > max_strlen);
}

static gboolean __xr_value_list_is_complicated(xr_value* v)
{
  GSList* i;

  if (v == NULL)
    return FALSE;

  if (v->type == XRV_ARRAY)
  {
    if (g_slist_length(xr_value_get_items(v)) > 8)
      return TRUE;
    else
    {
      for (i = xr_value_get_items(v); i; i = i->next)
        if (__xr_value_is_complicated(i->data, 35))
          return TRUE;
    }
  }
  else if (v->type == XRV_STRUCT)
  {
    if (g_slist_length(xr_value_get_members(v)) > 5)
      return TRUE;
    else
    {
      for (i = xr_value_get_members(v); i; i = i->next)
        if (__xr_value_is_complicated(xr_value_get_member_value(i->data), 25))
          return TRUE;
    }
  }

  return FALSE;
}

void xr_value_dump(xr_value* v, GString* string, int indent)
{
  GSList* i;
  char buf[256];

  g_return_if_fail(v != NULL);

  memset(buf, 0, sizeof(buf));
  memset(buf, ' ', MIN(indent * 2, sizeof(buf) - 1));

  switch (xr_value_get_type(v))
  {
    case XRV_ARRAY:
    {
      if (xr_value_get_items(v) == NULL)
      {
        g_string_append(string, "[]");
      }
      else if (!__xr_value_list_is_complicated(v))
      {
        g_string_append(string, "[ ");
        for (i = xr_value_get_items(v); i; i = i->next)
        {
          xr_value_dump(i->data, string, indent+1);
          g_string_append_printf(string, "%s ", i->next ? "," : "");
        }
        g_string_append(string, "]");
      }
      else
      {
        g_string_append_printf(string, "[");
        for (i = xr_value_get_items(v); i; i = i->next)
        {
          g_string_append_printf(string, "\n%s  ", buf);
          xr_value_dump(i->data, string, indent + 1);
          if (i->next)
            g_string_append(string, ",");
        }
        g_string_append_printf(string, "\n%s]", buf);
      }
      break;
    }
    case XRV_STRUCT:
    {
      if (xr_value_get_members(v) == NULL)
      {
        g_string_append(string, "{}");
      }
      else if (!__xr_value_list_is_complicated(v))
      {
        g_string_append(string, "{ ");
        for (i = xr_value_get_members(v); i; i = i->next)
        {
          xr_value_dump(i->data, string, indent);
          g_string_append_printf(string, "%s ", i->next ? "," : "");
        }
        g_string_append(string, "}");
      }
      else
      {
        g_string_append(string, "{");
        for (i = xr_value_get_members(v); i; i = i->next)
        {
          g_string_append_printf(string, "\n%s  ", buf);
          xr_value_dump(i->data, string, indent);
          if (i->next)
            g_string_append(string, ",");
        }
        g_string_append_printf(string, "\n%s}", buf);
      }
      break;
    }
    case XRV_MEMBER:
    {
      g_string_append_printf(string, "%s: ", xr_value_get_member_name(v));
      xr_value_dump(xr_value_get_member_value(v), string, indent + 1);
      break;
    }
    case XRV_INT:
    {
      g_string_append_printf(string, "%d", v->int_val);
      break;
    }
    case XRV_STRING:
    {
      g_string_append_printf(string, "\"%s\"", v->str_val);
      break;
    }
    case XRV_BOOLEAN:
    {
      g_string_append_printf(string, "%s", v->int_val ? "true" : "false");
      break;
    }
    case XRV_DOUBLE:
    {
      g_string_append_printf(string, "%g", v->dbl_val);
      break;
    }
    case XRV_TIME:
    {
      g_string_append_printf(string, "%s", v->str_val);
      break;
    }
    case XRV_BLOB:
    {
      g_string_append_printf(string, "<BLOB>");
      break;
    }
  }
}
