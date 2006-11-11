#include <stdio.h>

#include "xdl.h"

xdl_typedef* xdl_typedef_new(int type, char* name, char* cname, char* ctype, char* cnull, char* dem_name, char* mar_name)
{
  xdl_typedef* t = g_new0(xdl_typedef, 1);
  t->type = type;
  t->name = g_strdup(name);
  t->cname = g_strdup(cname);
  t->ctype = g_strdup(ctype);
  t->cnull = g_strdup(cnull);

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
  c->types = g_slist_append(c->types, xdl_typedef_new(TD_BASE, "int",      "int",      "int",    "-1",    "xr_value_to_int", "xr_value_int_new"));
  c->types = g_slist_append(c->types, xdl_typedef_new(TD_BASE, "boolean",  "boolean",  "int",    "-1",    "xr_value_to_bool", "xr_value_bool_new"));
  c->types = g_slist_append(c->types, xdl_typedef_new(TD_BASE, "string",   "string",   "char*",  "NULL",  "xr_value_to_string", "xr_value_string_new"));
  c->types = g_slist_append(c->types, xdl_typedef_new(TD_BASE, "double",   "double",   "double", "0.0",   "xr_value_to_double", "xr_value_double_new"));
  c->types = g_slist_append(c->types, xdl_typedef_new(TD_BASE, "time",     "time",     "char*",  "NULL",  "xr_value_to_time", "xr_value_time_new"));
  c->types = g_slist_append(c->types, xdl_typedef_new(TD_BLOB, "blob",     "blob",     "xr_blob*",  "NULL",  "xr_value_to_blob", "xr_value_blob_new"));
  c->types = g_slist_append(c->types, xdl_typedef_new(TD_ANY,  "any",      "any",      "xr_value*", "NULL", NULL, NULL));
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
  xdl_typedef* a = xdl_typedef_new(TD_ARRAY, NULL, g_strdup_printf("array_%s", item->cname), "GSList*", "NULL", NULL, NULL);
  a->item_type = item;
  if (servlet != NULL)
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
    "NULL", NULL, NULL
  );
  return s;
}

void xdl_process(xdl_model *xdl)
{
}
