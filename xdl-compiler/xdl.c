#include <stdio.h>

#include "xdl.h"

xdl_typedef* xdl_typedef_new(int type, char* name, char* ctype, char* cnull)
{
  xdl_typedef* t = g_new0(xdl_typedef, 1);
  t->type = type;
  t->name = g_strdup(name);
  t->ctype = g_strdup(ctype);
  t->cnull = g_strdup(cnull);
  return t;
}

xdl_model* xdl_new()
{
  xdl_model* c = g_new0(xdl_model, 1);
  c->types = g_slist_append(c->types, xdl_typedef_new(TD_BASE, "int",      "int",    "-1"));
  c->types = g_slist_append(c->types, xdl_typedef_new(TD_BASE, "boolean",  "int",    "-1"));
  c->types = g_slist_append(c->types, xdl_typedef_new(TD_BASE, "string",   "char*",  "NULL"));
  c->types = g_slist_append(c->types, xdl_typedef_new(TD_BASE, "double",   "double", "0.0"));
  c->types = g_slist_append(c->types, xdl_typedef_new(TD_BASE, "time",     "char*",  "NULL"));
  c->types = g_slist_append(c->types, xdl_typedef_new(TD_BLOB, "blob",     "char*",  "NULL"));
  c->types = g_slist_append(c->types, xdl_typedef_new(TD_ANY,  "any",      "xr_value*", "NULL"));
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

xdl_typedef* xdl_typedef_find_array(xdl_model *xdl, xdl_typedef* item)
{
  GSList* i;
  for (i=xdl->types; i; i=i->next)
  {
    xdl_typedef* t = i->data;
    if (t->type == TD_ARRAY && t->item_type == item)
      return t;
  }
  return NULL;
}

void xdl_process(xdl_model *xdl)
{
  GSList *i, *j;
  for (i=xdl->types; i; i=i->next)
  {
    xdl_typedef* t = i->data;
    if (t->type == TD_STRUCT)
    {
      t->name = g_strdup_printf("%s%s", xdl->name, t->name);
      t->ctype = g_strdup_printf("%s*", t->name);
    }
  }

  for (i=xdl->servlets; i; i=i->next)
  {
    xdl_servlet* s = i->data;
    for (j=s->types; j; j=j->next)
    {
      xdl_typedef* t = j->data;
      if (t->type == TD_STRUCT)
      {
        t->name = g_strdup_printf("%s%s_%s", xdl->name, s->name, t->name);
        t->ctype = g_strdup_printf("%s*", t->name);
      }
    }
  }
}

