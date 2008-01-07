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

#include "xdl.h"

xdl_typedef* xdl_typedef_new(int type, char* name, char* cname, char* ctype, char* cnull, char* dem_name, char* mar_name, char* free_func)
{
  xdl_typedef* t = g_new0(xdl_typedef, 1);
  t->type = type;
  t->name = g_strdup(name);
  t->cname = g_strdup(cname);
  t->ctype = g_strdup(ctype);
  t->cnull = g_strdup(cnull);
  t->free_func = g_strdup(free_func);

  if (type == TD_BASE && (!strcmp(name, "string") || !strcmp(name, "time")))
    t->copy_func = g_strdup("g_strdup");
  else if (type == TD_BLOB)
    t->copy_func = g_strdup("xr_blob_ref");
  else if (type == TD_ANY)
    t->copy_func = g_strdup("xr_value_ref");
  else if (type == TD_STRUCT || type == TD_ARRAY)
    t->copy_func = g_strdup_printf("%s_copy", t->cname);

  if (dem_name)
    t->demarch_name = g_strdup(dem_name);
  else
    t->demarch_name = g_strdup_printf("__xr_value_to_%s", t->cname);

  if (mar_name)
    t->march_name = g_strdup(mar_name);
  else
    t->march_name = g_strdup_printf("__%s_to_xr_value", t->cname);

  return t;
}

xdl_model* xdl_new()
{
  xdl_model* c = g_new0(xdl_model, 1);
  c->types = g_slist_append(c->types, xdl_typedef_new(TD_BASE, "int",      "int",      "int",       "-1",    "xr_value_to_int",    "xr_value_int_new",    NULL));
  c->types = g_slist_append(c->types, xdl_typedef_new(TD_BASE, "boolean",  "boolean",  "gboolean",  "FALSE", "xr_value_to_bool",   "xr_value_bool_new",   NULL));
  c->types = g_slist_append(c->types, xdl_typedef_new(TD_BASE, "string",   "string",   "char*",     "NULL",  "xr_value_to_string", "xr_value_string_new", "g_free"));
  c->types = g_slist_append(c->types, xdl_typedef_new(TD_BASE, "double",   "double",   "double",    "0.0",   "xr_value_to_double", "xr_value_double_new", NULL));
  c->types = g_slist_append(c->types, xdl_typedef_new(TD_BASE, "time",     "time",     "char*",     "NULL",  "xr_value_to_time",   "xr_value_time_new",   "g_free"));
  c->types = g_slist_append(c->types, xdl_typedef_new(TD_BLOB, "blob",     "blob",     "xr_blob*",  "NULL",  "xr_value_to_blob",   "xr_value_blob_new",   "xr_blob_unref"));
  c->types = g_slist_append(c->types, xdl_typedef_new(TD_ANY,  "any",      "any",      "xr_value*", "NULL",  "xr_value_to_value",  "xr_value_ref",        "xr_value_unref"));
  c->name = "";
  return c;
}

xdl_typedef* xdl_typedef_find(xdl_model *xdl, xdl_servlet *servlet, const char* name)
{
  GSList* i;
  if (servlet != NULL)
  {
    for (i=servlet->types; i; i=i->next)
    {
      xdl_typedef* t = i->data;
      if (t->name && !strcmp(t->name, name))
        return t;
    }
  }
  for (i=xdl->types; i; i=i->next)
  {
    xdl_typedef* t = i->data;
    if (t->name && !strcmp(t->name, name))
      return t;
  }
  return NULL;
}

xdl_typedef* xdl_typedef_new_array(xdl_model *xdl, xdl_servlet *servlet, xdl_typedef* item)
{
  GSList* i;
  if (servlet != NULL)
  {
    for (i=servlet->types; i; i=i->next)
    {
      xdl_typedef* t = i->data;
      if (t->type == TD_ARRAY && t->item_type == item)
        return t;
    }
  }
  for (i=xdl->types; i; i=i->next)
  {
    xdl_typedef* t = i->data;
    if (t->type == TD_ARRAY && t->item_type == item)
      return t;
  }
  xdl_typedef* a = xdl_typedef_new(TD_ARRAY,
    NULL, g_strdup_printf("Array_%s", item->cname),
    "GSList*", "NULL", NULL, NULL,
    g_strdup_printf("Array_%s_free", item->cname));
  a->item_type = item;
  if (servlet != NULL && item->type == TD_STRUCT)
    servlet->types = g_slist_append(servlet->types, a);
  else
    xdl->types = g_slist_append(xdl->types, a);
  return a;
}

xdl_typedef* xdl_typedef_new_struct(xdl_model *xdl, xdl_servlet *servlet, char* name)
{
  xdl_typedef* s = xdl_typedef_new(TD_STRUCT,
    name,
    g_strdup_printf("%s%s", xdl->name, name),
    g_strdup_printf("%s%s*", xdl->name, name),
    "NULL", NULL, NULL,
    g_strdup_printf("%s%s_free", xdl->name, name)
  );
  return s;
}

gint xdl_method_compare(xdl_method* m1, xdl_method* m2)
{
  return strcmp(m1->name, m2->name);
}

void xdl_process(xdl_model *xdl)
{
}

xdl_error_code* xdl_error_new(xdl_model *xdl, xdl_servlet *servlet, char* name, int code)
{
  xdl_error_code* e = g_new0(xdl_error_code, 1);
  e->name = g_strdup(name);
  e->code = code;

  if (servlet != NULL)
  {
    e->cenum = g_strdup_printf("%s_XMLRPC_ERROR_%s_%s", g_ascii_strup(xdl->name, -1), g_ascii_strup(servlet->name, -1), g_ascii_strup(name, -1));
    servlet->errors = g_slist_append(servlet->errors, e);
  }
  else
  {
    e->cenum = g_strdup_printf("%s_XMLRPC_ERROR_%s", g_ascii_strup(xdl->name, -1), g_ascii_strup(name, -1));
    xdl->errors = g_slist_append(xdl->errors, e);
  }
  return e;
}

char* xdl_typedef_vala_name(xdl_typedef* t)
{
  if (t == NULL)
    return "";
  if (t->type == TD_BASE)
  {
    if (!strcmp(t->name, "time"))
      return "string";
    if (!strcmp(t->name, "boolean"))
      return "bool";
    return t->name;
  }
  else if (t->type == TD_STRUCT)
  {
    return t->name;
  }
  else if (t->type == TD_ARRAY)
  {
    return g_strdup_printf("GLib.SList<%s>", xdl_typedef_vala_name(t->item_type));
  }
  else if (t->type == TD_BLOB)
    return "XR.Blob";
  else if (t->type == TD_ANY)
    return "XR.Value";
  else
    g_assert_not_reached();
  return NULL;
}

char* xdl_typedef_xdl_name(xdl_typedef* t)
{
  if (t == NULL)
    return "";
  if (t->type == TD_ARRAY)
    return g_strdup_printf("array<%s>", xdl_typedef_xdl_name(t->item_type));
  return t->name;
}
