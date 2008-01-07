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

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xdl.h"

/* codegen helpers */

#define EL(i, fmt, args...) \
  line_fprintf(f, i, fmt "\n", ##args)

#define E(i, fmt, args...) \
  line_fprintf(f, i, fmt, ##args)

#define NL \
  line_fprintf(f, 0, "\n")

#define S(args...) \
  g_strdup_printf(args)

#define OPEN(fmt, args...) { \
  if (f) \
    fclose(f); \
  f = fopen(current_file = S(fmt, ##args), "w"); \
  if (f == NULL) { \
    fprintf(stderr, "Can't open output file for writing."); exit(1); } \
  } \
  current_line = 1

static int current_line;
char* current_file = NULL;

static void line_fprintf(FILE* f, int indent, const char* fmt, ...)
{
  int i;
  va_list ap;

  for (i=0; i<indent; i++)
    fprintf(f, "    ");

  va_start(ap, fmt);
  char* v = g_strdup_vprintf(fmt, ap);
  va_end(ap);

  fprintf(f, "%s", v);

  for (i=0; i<strlen(v); i++)
    if (v[i] == '\n')
      current_line++;
}

#define ORIG_LINE() \
  EL(0, "#line %d \"%s\"", current_line+1, current_file)

#define FILE_LINE(_line, _file) \
  EL(0, "#line %d \"%s\"", _line, _file)

#define STUB(s) \
  do { \
    if (s) \
    { \
      FILE_LINE(s##_line+1, xdl_file); \
      EL(0, "%s", s); \
      ORIG_LINE(); \
    } \
  } while(0)

void gen_type_marchalizers(FILE* f, xdl_typedef* t)
{
  GSList *i, *j, *k;
    if (t->type == TD_STRUCT)
    {
      EL(0, "G_GNUC_UNUSED static xr_value* %s(%s _nstruct)", t->march_name, t->ctype);
      EL(0, "{");
      EL(1, "xr_value* _struct;");
      for (k=t->struct_members; k; k=k->next)
      {
        xdl_struct_member* m = k->data;
        EL(1, "xr_value* %s = NULL;", m->name);
      }
      NL;
      EL(1, "if (_nstruct == NULL)");
      EL(2, "return NULL;");
      NL;
      EL(1, "if (");
      for (k=t->struct_members; k; k=k->next)
      {
        xdl_struct_member* m = k->data;
        EL(2, "(%s = %s(_nstruct->%s)) == NULL%s", m->name, m->type->march_name, m->name, k->next ? " ||" : "");
      }
      EL(1, ")");
      EL(1, "{");
      for (k=t->struct_members; k; k=k->next)
      {
        xdl_struct_member* m = k->data;
        EL(2, "xr_value_unref(%s);", m->name);
      }
      EL(2, "return NULL;");
      EL(1, "}");
      NL;
      EL(1, "_struct = xr_value_struct_new();");
      for (k=t->struct_members; k; k=k->next)
      {
        xdl_struct_member* m = k->data;
        EL(1, "xr_value_struct_set_member(_struct, \"%s\", %s);", m->name, m->name);
      }
      EL(1, "return _struct;");
      EL(0, "}");
      NL;

      EL(0, "G_GNUC_UNUSED static gboolean %s(xr_value* _struct, %s* _nstruct)", t->demarch_name, t->ctype);
      EL(0, "{");
      EL(1, "%s _tmp_nstruct;", t->ctype);
      NL;
      EL(1, "g_return_val_if_fail(_nstruct != NULL, FALSE);");
      NL;
      EL(1, "if (_struct == NULL || xr_value_get_type(_struct) != XRV_STRUCT)");
      EL(2, "return FALSE;");
      NL;
      EL(1, "_tmp_nstruct = %s_new();", t->cname);
      EL(1, "if (");
      for (k=t->struct_members; k; k=k->next)
      {
        xdl_struct_member* m = k->data;
        EL(2, "!%s(xr_value_get_member(_struct, \"%s\"), &_tmp_nstruct->%s)%s", m->type->demarch_name, m->name, m->name, k->next ? " ||" : "");
      }
      EL(1, ")");
      EL(1, "{");
      EL(2, "%s(_tmp_nstruct);", t->free_func);
      EL(2, "return FALSE;");
      EL(1, "}");
      NL;
      EL(1, "*_nstruct = _tmp_nstruct;");
      EL(1, "return TRUE;");
      EL(0, "}");
      NL;
    }
    else if (t->type == TD_ARRAY)
    {
      EL(0, "G_GNUC_UNUSED static xr_value* %s(%s _narray)", t->march_name, t->ctype);
      EL(0, "{");
      EL(1, "GSList* _item;");
      EL(1, "xr_value* _array = xr_value_array_new();");
      NL;
      EL(1, "for (_item = _narray; _item; _item = _item->next)");
      EL(1, "{");
      EL(2, "xr_value* _item_value = %s((%s)_item->data);", t->item_type->march_name, t->item_type->ctype);
      NL;
      EL(2, "if (_item_value == NULL)");
      EL(2, "{");
      EL(3, "xr_value_unref(_array);");
      EL(3, "return NULL;");
      EL(2, "}");
      NL;
      EL(2, "xr_value_array_append(_array, _item_value);");
      EL(1, "}");
      NL;
      EL(1, "return _array;");
      EL(0, "}");
      NL;

      EL(0, "G_GNUC_UNUSED static gboolean %s(xr_value* _array, %s* _narray)", t->demarch_name, t->ctype);
      EL(0, "{");
      EL(1, "GSList *_tmp_narray = NULL, *_item;");
      NL;
      EL(1, "g_return_val_if_fail(_narray != NULL, FALSE);");
      NL;
      EL(1, "if (_array == NULL || xr_value_get_type(_array) != XRV_ARRAY)");
      EL(2, "return FALSE;");
      NL;
      EL(1, "for (_item = xr_value_get_items(_array); _item; _item = _item->next)");
      EL(1, "{");
      EL(2, "%s _item_value = %s;", t->item_type->ctype, t->item_type->cnull);
      NL;
      EL(2, "if (!%s((xr_value*)_item->data, &_item_value))", t->item_type->demarch_name);
      EL(2, "{");
      EL(3, "%s(_tmp_narray);", t->free_func);
      EL(3, "return FALSE;");
      EL(2, "}");
      NL;
      EL(2, "_tmp_narray = g_slist_append(_tmp_narray, (void*)_item_value);");
      EL(1, "}");
      NL;
      EL(1, "*_narray = _tmp_narray;");
      EL(1, "return TRUE;");
      EL(0, "}");
      NL;
    }
}

void gen_type_freealloc(FILE* f, xdl_typedef* t, int def)
{
  GSList *i, *j, *k;

  if (def)
  {
    if (t->type == TD_STRUCT)
      EL(0, "%s %s_new();", t->ctype, t->cname);
    if (t->type == TD_STRUCT || t->type == TD_ARRAY)
    {
      EL(0, "void %s(%s val);", t->free_func, t->ctype);
      EL(0, "%s %s(%s orig);", t->ctype, t->copy_func, t->ctype);
    }
    return;
  }

  if (t->type == TD_STRUCT)
  {
    /* new */
    EL(0, "%s %s_new()", t->ctype, t->cname);
    EL(0, "{");
    EL(1, "return g_new0(%s, 1);", t->cname);
    EL(0, "}");
    NL;

    /* free */
    EL(0, "void %s(%s val)", t->free_func, t->ctype);
    EL(0, "{");
    EL(1, "if (val == NULL)");
    EL(2, "return;");
    NL;
    for (k=t->struct_members; k; k=k->next)
    {
      xdl_struct_member* m = k->data;
      if (m->type->free_func)
        EL(1, "%s(val->%s);", m->type->free_func, m->name);
    }
    EL(1, "g_free(val);");
    EL(0, "}");
    NL;

    /* copy */
    EL(0, "%s %s(%s orig)", t->ctype, t->copy_func, t->ctype);
    EL(0, "{");
    EL(1, "%s copy;", t->ctype);
    NL;
    EL(1, "if (orig == NULL)");
    EL(2, "return NULL;");
    NL;
    EL(1, "copy = %s_new();", t->cname);
    for (k=t->struct_members; k; k=k->next)
    {
      xdl_struct_member* m = k->data;
      if (m->type->copy_func)
        EL(1, "copy->%s = %s(orig->%s);", m->name, m->type->copy_func, m->name);
      else
        EL(1, "copy->%s = orig->%s;", m->name, m->name);
    }
    NL;
    EL(1, "return copy;");
    EL(0, "}");
    NL;
  }
  else if (t->type == TD_ARRAY)
  {
    /* free */
    EL(0, "void %s(%s val)", t->free_func, t->ctype);
    EL(0, "{");
    if (t->item_type->free_func)
      EL(1, "g_slist_foreach(val, (GFunc)%s, NULL);", t->item_type->free_func);
    EL(1, "g_slist_free(val);");
    EL(0, "}");
    NL;

    /* copy */
    EL(0, "%s %s(%s orig)", t->ctype, t->copy_func, t->ctype);
    EL(0, "{");
    EL(1, "%s copy = NULL;", t->ctype);
    NL;
    EL(1, "while (orig)");
    EL(1, "{");
    if (t->item_type->copy_func)
      EL(2, "copy = g_slist_prepend(copy, (gpointer)%s((%s)orig->data));", t->item_type->copy_func, t->item_type->ctype);
    else
      EL(2, "copy = g_slist_prepend(copy, orig->data);");
    EL(2, "orig = orig->next;");
    EL(1, "}");
    NL;
    EL(1, "return g_slist_reverse(copy);");
    EL(0, "}");
    NL;
  }
}

void gen_type_defs(FILE* f, GSList* types)
{
  GSList *i, *j, *k;

  for (j=types; j; j=j->next)
  {
    xdl_typedef* t = j->data;
    if (t->type == TD_STRUCT)
    {
      if (t->doc)
      {
        NL;
        EL(0, "%s", t->doc);
      }
      EL(0, "typedef struct _%s %s;", t->cname, t->cname);
    }
  }
  NL;

  for (j=types; j; j=j->next)
  {
    xdl_typedef* t = j->data;
    if (t->type == TD_STRUCT)
    {
      if (t->doc)
        EL(0, "%s", t->doc);
      EL(0, "struct _%s", t->cname);
      EL(0, "{");
      for (k=t->struct_members; k; k=k->next)
      {
        xdl_struct_member* m = k->data;
        EL(1, "%s %s;%s", m->type->ctype, m->name, m->type->type == TD_ARRAY ? S(" /* %s */", m->type->cname) : "");
      }
      EL(0, "};");
      NL;
    }
  }
}

void gen_marchalizers(FILE* f, xdl_model* xdl, xdl_servlet* s)
{
  GSList *j;

  for (j=xdl->types; j; j=j->next)
  {
    xdl_typedef* t = j->data;
    if (t->type == TD_ANY)
      continue;
    gen_type_marchalizers(f, t);
  }
  for (j=s->types; j; j=j->next)
  {
    xdl_typedef* t = j->data;
    if (t->type == TD_ANY)
      continue;
    gen_type_marchalizers(f, t);
  }
}

void gen_errors_header(FILE* f, xdl_model* xdl, xdl_servlet* s)
{
  GSList* j;
  GSList* errs = s ? s->errors : xdl->errors;
  if (errs == NULL)
    return;

  EL(0, "enum");
  EL(0, "{");
  for (j=errs; j; j=j->next)
  {
    xdl_error_code* e = j->data;
    EL(1, "%s = %d,", e->cenum, e->code);
  }
  EL(0, "};");
  NL;
  EL(0, "const char* %s%s_xmlrpc_error_to_string(int code);", xdl->name, s ? s->name : "");
  NL;
}

void gen_errors_impl(FILE* f, xdl_model* xdl, xdl_servlet* s)
{
  GSList* j;
  GSList* errs = s ? s->errors : xdl->errors;
  if (errs == NULL)
    return;

  EL(0, "const char* %s%s_xmlrpc_error_to_string(int code)", xdl->name, s ? s->name : "");
  EL(0, "{");
  for (j=errs; j; j=j->next)
  {
    xdl_error_code* e = j->data;
    EL(1, "if (code == %s)", e->cenum);
    EL(2, "return \"%s\";", e->name);
  }
  EL(1, "return NULL;");
  EL(0, "}");
  NL;
}

/* main() */

static gchar* out_dir = NULL;
static gchar* xdl_file = NULL;
static gchar* mode = "all";
static GOptionEntry entries[] = 
{
  { "xdl", 'i', 0, G_OPTION_ARG_STRING, &xdl_file, "Interface description file.", "FILE" },
  { "out", 'o', 0, G_OPTION_ARG_STRING, &out_dir, "Output directory.", "DIR" },
  { "mode", 'm', 0, G_OPTION_ARG_STRING, &mode, "Compiler mode (all, server-impl, pub-headers, pub-impl, vapi).", "MODE" },
  { NULL, 0, 0, 0, NULL, NULL, NULL }
};

#define MODE_IS(str) !strcmp(mode, G_STRINGIFY(str))

int main(int ac, char* av[])
{
  GError* err = NULL;
  GOptionContext* ctx = g_option_context_new("- XML-RPC Interface Generator.");
  g_option_context_add_main_entries(ctx, entries, NULL);
  g_option_context_parse(ctx, &ac, &av, NULL);
  g_option_context_set_help_enabled(ctx, TRUE);

  if (out_dir == NULL)
  {
    printf("You must specify output directory.\n");
    return 1;
  }

  if (xdl_file == NULL)
  {
    printf("You must specify XDL file.\n");
    return 1;
  }
  
  xdl_model* xdl = xdl_parse_file(xdl_file, &err);
  if (err)
  {
    printf("%s\n", err->message);
    exit(1);
  }
  xdl_process(xdl);

  FILE* f = NULL;
  GSList *i, *j, *k;
  
  int pub_headers = !strcmp(mode, "all") || !strcmp(mode, "pub-headers");
  int pub_impl = !strcmp(mode, "all") || !strcmp(mode, "pub-impl");
  int server_impl = !strcmp(mode, "all") || !strcmp(mode, "server-impl");
  int vapi_file = !strcmp(mode, "vapi");
  int xdl_gen = !strcmp(mode, "xdl");
  int js_client = !strcmp(mode, "js");

  /***********************************************************
   * common types header                                     *
   ***********************************************************/

  if (pub_headers)
  {

  OPEN("%s/%sCommon.h", out_dir, xdl->name);

  EL(0, "#ifndef __%s_Common_H__", xdl->name);
  EL(0, "#define __%s_Common_H__", xdl->name);
  NL;

  EL(0, "#include <xr-value.h>");
  NL;

  gen_errors_header(f, xdl, NULL);

  gen_type_defs(f, xdl->types);

  for (j=xdl->types; j; j=j->next)
  {
    xdl_typedef* t = j->data;
    gen_type_freealloc(f, t, 1);
  }
  NL;

  EL(0, "#endif");

  }

  /***********************************************************
   * common types implementation                             *
   ***********************************************************/

  if (pub_impl)
  {

  OPEN("%s/%sCommon.c", out_dir, xdl->name);

  EL(0, "#include \"%sCommon.h\"", xdl->name);
  NL;

  for (j=xdl->types; j; j=j->next)
  {
    xdl_typedef* t = j->data;
    gen_type_freealloc(f, t, 0);
  }

  gen_errors_impl(f, xdl, NULL);

  }

  /* client/servlet implementations for specific interfaces */

  for (i=xdl->servlets; i; i=i->next)
  {
    xdl_servlet* s = i->data;

    /***********************************************************
     * servlet types header                                    *
     ***********************************************************/

    if (pub_headers)
    {

    OPEN("%s/%s%s.h", out_dir, xdl->name, s->name);

    EL(0, "#ifndef __%s_%s_H__", xdl->name, s->name);
    EL(0, "#define __%s_%s_H__", xdl->name, s->name);
    NL;

    EL(0, "#include \"%sCommon.h\"", xdl->name);
    NL;

    gen_errors_header(f, xdl, s);

    gen_type_defs(f, s->types);

    for (j=s->types; j; j=j->next)
    {
      xdl_typedef* t = j->data;
      gen_type_freealloc(f, t, 1);
    }
    NL;

    EL(0, "#endif");

    }

    /***********************************************************
     * servlet types implementation                            *
     ***********************************************************/

    if (pub_impl)
    {

    OPEN("%s/%s%s.c", out_dir, xdl->name, s->name);

    EL(0, "#include \"%s%s.h\"", xdl->name, s->name);
    NL;

    for (j=s->types; j; j=j->next)
    {
      xdl_typedef* t = j->data;
      gen_type_freealloc(f, t, 0);
    }

    gen_errors_impl(f, xdl, s);
    
    }

    /***********************************************************
     * servlet client interface definition                     *
     ***********************************************************/

    if (pub_headers)
    {

    OPEN("%s/%s%s.xrc.h", out_dir, xdl->name, s->name);

    EL(0, "#ifndef __%s_%s_XRC_H__", xdl->name, s->name);
    EL(0, "#define __%s_%s_XRC_H__", xdl->name, s->name);
    NL;

    EL(0, "#include <xr-client.h>");
    EL(0, "#include \"%s%s.h\"", xdl->name, s->name);
    NL;

    for (j=s->methods; j; j=j->next)
    {
      xdl_method* m = j->data;

      EL(0, "/** ");
      EL(0, " * ");
      EL(0, " * @param _conn Client connection object.");
      for (k=m->params; k; k=k->next)
      {
        xdl_method_param* p = k->data;
        EL(0, " * @param %s", p->name);
      }
      EL(0, " * @param _error Error variable pointer (may be NULL).");
      EL(0, " * ");
      EL(0, " * @return ");
      EL(0, " */ ");

      E(0, "%s %s%s_%s(xr_client_conn* _conn", m->return_type->ctype, xdl->name, s->name, m->name);
      for (k=m->params; k; k=k->next)
      {
        xdl_method_param* p = k->data;
        E(0, ", %s%s %s", !strcmp(p->type->ctype, "char*") ? "const " : "", p->type->ctype, p->name);
      }
      EL(0, ", GError** _error);");
      NL;
    }

    EL(0, "#endif");
    
    }

    /***********************************************************
     * servlet client interface implementation                 *
     ***********************************************************/

    if (pub_impl)
    {

    OPEN("%s/%s%s.xrc.c", out_dir, xdl->name, s->name);

    EL(0, "#include \"%s%s.xrc.h\"", xdl->name, s->name);
    NL;

    gen_marchalizers(f, xdl, s);
    NL;

    for (j=s->methods; j; j=j->next)
    {
      xdl_method* m = j->data;

      E(0, "%s %s%s_%s(xr_client_conn* _conn", m->return_type->ctype, xdl->name, s->name, m->name);
      for (k=m->params; k; k=k->next)
      {
        xdl_method_param* p = k->data;
        E(0, ", %s%s %s", !strcmp(p->type->ctype, "char*") ? "const " : "", p->type->ctype, p->name);
      }
      EL(0, ", GError** _error)");
      EL(0, "{");
      EL(1, "%s _retval = %s;", m->return_type->ctype, m->return_type->cnull);
      EL(1, "xr_value* _param_value;");
      EL(1, "xr_call* _call;");
      NL;
      EL(1, "g_return_val_if_fail(_conn != NULL, _retval);");
      EL(1, "g_return_val_if_fail(_error == NULL || *_error == NULL, _retval);");
      NL;
      EL(1, "_call = xr_call_new(\"%s%s.%s\");", xdl->name, s->name, m->name);
      for (k=m->params; k; k=k->next)
      {
        xdl_method_param* p = k->data;
        NL;
        EL(1, "_param_value = %s(%s);", p->type->march_name, p->name);
        EL(1, "if (_param_value == NULL)");
        EL(1, "{");
        EL(2, "g_set_error(_error, XR_CLIENT_ERROR, XR_CLIENT_ERROR_MARCHALIZER, \"Call parameter value marchalization failed (param=%s).\");", p->name);
        EL(2, "xr_call_free(_call);");
        EL(2, "return _retval;");
        EL(1, "}");
        EL(1, "xr_call_add_param(_call, _param_value);");
      }
      NL;
      EL(1, "if (xr_client_call(_conn, _call, _error))");
      EL(1, "{");
      EL(2, "if (!%s(xr_call_get_retval(_call), &_retval))", m->return_type->demarch_name);
      EL(3, "g_set_error(_error, XR_CLIENT_ERROR, XR_CLIENT_ERROR_MARCHALIZER, \"Call return value demarchalization failed.\");");
      EL(1, "}");
      NL;
      EL(1, "xr_call_free(_call);");
      EL(1, "return _retval;");
      EL(0, "}");
      NL;
    }

    }

    /***********************************************************
     * servlet server stubs for implementation                 *
     ***********************************************************/

    if (server_impl)
    {

    OPEN("%s/%s%s.stubs.h", out_dir, xdl->name, s->name);

    EL(0, "#ifndef __%s_%s_STUBS_H__", xdl->name, s->name);
    EL(0, "#define __%s_%s_STUBS_H__", xdl->name, s->name);
    NL;

    EL(0, "#include <xr-server.h>");
    EL(0, "#include \"%s%s.h\"", xdl->name, s->name);
    NL;

    if (s->doc)
      EL(0, "%s", s->doc);
    else
    {
      EL(0, "/** Implementation specific servlet data.");
      EL(0, " */ ");
    }
    EL(0, "typedef struct _%s%sServlet %s%sServlet;", xdl->name, s->name, xdl->name, s->name);
    NL;

    EL(0, "/** Utility method that returns size of the servlet private data.");
    EL(0, " * ");
    EL(0, " * @return Size of the @ref %s%sServlet struct.", xdl->name, s->name);
    EL(0, " */ ");
    EL(0, "int __%s%sServlet_get_priv_size();", xdl->name, s->name);
    NL;

    EL(0, "/** Servlet constructor.");
    EL(0, " * ");
    EL(0, " * @param _servlet Servlet object.");
    EL(0, " * ");
    EL(0, " * @return TRUE if all is ok.");
    EL(0, " */ ");
    EL(0, "gboolean %s%sServlet_init(xr_servlet* _servlet);", xdl->name, s->name);
    NL;

    EL(0, "/** Servlet destructor.");
    EL(0, " * ");
    EL(0, " * @param _servlet Servlet object.");
    EL(0, " */ ");
    EL(0, "void %s%sServlet_fini(xr_servlet* _servlet);", xdl->name, s->name);
    NL;

    EL(0, "/** Pre-call hook.");
    EL(0, " * ");
    EL(0, " * @param _servlet Servlet object.");
    EL(0, " * @param _call Call object.");
    EL(0, " * ");
    EL(0, " * @return TRUE if you want to continue execution of the call.");
    EL(0, " */ ");
    EL(0, "gboolean %s%sServlet_pre_call(xr_servlet* _servlet, xr_call* _call);", xdl->name, s->name);
    NL;

    EL(0, "/** Post-call hook.");
    EL(0, " * ");
    EL(0, " * @param _servlet Servlet object.");
    EL(0, " * @param _call Call object.");
    EL(0, " * ");
    EL(0, " * @return TRUE if you want to continue execution of the call.");
    EL(0, " */ ");
    EL(0, "gboolean %s%sServlet_post_call(xr_servlet* _servlet, xr_call* _call);", xdl->name, s->name);
    NL;

    EL(0, "/** Download hook.");
    EL(0, " * ");
    EL(0, " * @param _servlet Servlet object.");
    EL(0, " * ");
    EL(0, " * @return TRUE if you want to continue execution of the call.");
    EL(0, " */ ");
    EL(0, "gboolean %s%sServlet_download(xr_servlet* _servlet);", xdl->name, s->name);
    NL;

    EL(0, "/** Upload hook.");
    EL(0, " * ");
    EL(0, " * @param _servlet Servlet object.");
    EL(0, " * ");
    EL(0, " * @return TRUE if you want to continue execution of the call.");
    EL(0, " */ ");
    EL(0, "gboolean %s%sServlet_upload(xr_servlet* _servlet);", xdl->name, s->name);
    NL;

    for (j=s->methods; j; j=j->next)
    {
      xdl_method* m = j->data;

      if (m->doc)
      {
        EL(0, "%s", m->doc);
      }
      else
      {
        EL(0, "/** ");
        EL(0, " * ");
        EL(0, " * @param _servlet Servlet object.");
        for (k=m->params; k; k=k->next)
        {
          xdl_method_param* p = k->data;
          EL(0, " * @param %s", p->name);
        }
        EL(0, " * ");
        EL(0, " * @return ");
        EL(0, " */ ");
      }

      E(0, "%s %s%sServlet_%s(xr_servlet* _servlet", m->return_type->ctype, xdl->name, s->name, m->name);
      for (k=m->params; k; k=k->next)
      {
        xdl_method_param* p = k->data;
        E(0, ", %s %s", p->type->ctype, p->name);
      }
      EL(0, ", GError** _error);");
      NL;
    }

    EL(0, "#endif");

    /***********************************************************
     * servlet server stubs for implementation                 *
     ***********************************************************/

    OPEN("%s/%s%s.stubs.c", out_dir, xdl->name, s->name);

    EL(0, "#include \"%s%s.stubs.h\"", xdl->name, s->name);
    NL;

    if (s->stub_header)
    {
      STUB(s->stub_header);
      NL;
    }

    EL(0, "struct _%s%sServlet", xdl->name, s->name);
    EL(0, "{");
    STUB(s->stub_attrs);
    EL(0, "};");
    NL;

    EL(0, "int __%s%sServlet_get_priv_size()", xdl->name, s->name);
    EL(0, "{");
    EL(1, "return sizeof(%s%sServlet);", xdl->name, s->name);
    EL(0, "}");
    NL;

    EL(0, "gboolean %s%sServlet_init(xr_servlet* _servlet)", xdl->name, s->name);
    EL(0, "{");
    EL(1, "%s%sServlet* _priv = xr_servlet_get_priv(_servlet);", xdl->name, s->name);
    STUB(s->stub_init);
    EL(1, "return TRUE;");
    EL(0, "}");
    NL;

    EL(0, "void %s%sServlet_fini(xr_servlet* _servlet)", xdl->name, s->name);
    EL(0, "{");
    EL(1, "%s%sServlet* _priv = xr_servlet_get_priv(_servlet);", xdl->name, s->name);
    STUB(s->stub_fini);
    EL(0, "}");
    NL;

    EL(0, "gboolean %s%sServlet_pre_call(xr_servlet* _servlet, xr_call* _call)", xdl->name, s->name);
    EL(0, "{");
    EL(1, "%s%sServlet* _priv = xr_servlet_get_priv(_servlet);", xdl->name, s->name);
    STUB(s->stub_pre_call);
    EL(1, "return TRUE;");
    EL(0, "}");
    NL;

    EL(0, "gboolean %s%sServlet_post_call(xr_servlet* _servlet, xr_call* _call)", xdl->name, s->name);
    EL(0, "{");
    EL(1, "%s%sServlet* _priv = xr_servlet_get_priv(_servlet);", xdl->name, s->name);
    STUB(s->stub_post_call);
    EL(1, "return TRUE;");
    EL(0, "}");
    NL;

    EL(0, "gboolean %s%sServlet_download(xr_servlet* _servlet)", xdl->name, s->name);
    EL(0, "{");
    EL(1, "%s%sServlet* _priv = xr_servlet_get_priv(_servlet);", xdl->name, s->name);
    EL(1, "xr_http* _http = xr_servlet_get_http(_servlet);");
    STUB(s->stub_download);
    EL(1, "return FALSE;");
    EL(0, "}");
    NL;

    EL(0, "gboolean %s%sServlet_upload(xr_servlet* _servlet)", xdl->name, s->name);
    EL(0, "{");
    EL(1, "%s%sServlet* _priv = xr_servlet_get_priv(_servlet);", xdl->name, s->name);
    EL(1, "xr_http* _http = xr_servlet_get_http(_servlet);");
    STUB(s->stub_upload);
    EL(1, "return FALSE;");
    EL(0, "}");
    NL;

    for (j=s->methods; j; j=j->next)
    {
      xdl_method* m = j->data;

      E(0, "%s %s%sServlet_%s(xr_servlet* _servlet", m->return_type->ctype, xdl->name, s->name, m->name);
      for (k=m->params; k; k=k->next)
      {
        xdl_method_param* p = k->data;
        E(0, ", %s %s", p->type->ctype, p->name);
      }
      EL(0, ", GError** _error)");
      EL(0, "{");
      EL(1, "%s%sServlet* _priv = xr_servlet_get_priv(_servlet);", xdl->name, s->name);
      EL(1, "%s retval = %s;", m->return_type->ctype, m->return_type->cnull);
      if (m->stub_impl)
        STUB(m->stub_impl);
      else
        EL(1, "g_set_error(_error, 0, 1, \"Method is not implemented. (%s)\");", m->name);
      EL(1, "return retval;");
      EL(0, "}");
      NL;
    }

    /***********************************************************
     * servlet server interface internals header               *
     ***********************************************************/

    OPEN("%s/%s%s.xrs.h", out_dir, xdl->name, s->name);

    EL(0, "#ifndef __%s_%s_XRS_H__", xdl->name, s->name);
    EL(0, "#define __%s_%s_XRS_H__", xdl->name, s->name);
    NL;

    EL(0, "#include <xr-server.h>");
    EL(0, "#include \"%s%s.stubs.h\"", xdl->name, s->name);
    NL;

    EL(0, "xr_servlet_def* __%s%sServlet_def();", xdl->name, s->name);
    NL;

    EL(0, "#endif");

    /***********************************************************
     * servlet server interface implementation                 *
     ***********************************************************/

    OPEN("%s/%s%s.xrs.c", out_dir, xdl->name, s->name);

    EL(0, "#include \"%s%s.xrs.h\"", xdl->name, s->name);
    NL;

    gen_marchalizers(f, xdl, s);
    NL;

    for (j=s->methods; j; j=j->next)
    {
      xdl_method* m = j->data;
      int n = 0;

      EL(0, "static gboolean __method_%s(xr_servlet* _servlet, xr_call* _call)", m->name);
      EL(0, "{");
      // forward declarations
      EL(1, "gboolean _retval = FALSE;");
      EL(1, "%s _nreturn_value = %s;", m->return_type->ctype, m->return_type->cnull);
      EL(1, "xr_value* _return_value;");
      EL(1, "GError* _error = NULL;");
      for (k=m->params; k; k=k->next)
      {
        xdl_method_param* p = k->data;
        EL(1, "%s %s = %s;", p->type->ctype, p->name, p->type->cnull);
      }
      NL;
      EL(1, "g_return_val_if_fail(_servlet != NULL, FALSE);");
      EL(1, "g_return_val_if_fail(_call != NULL, FALSE);");
      // prepare parameters
      for (k=m->params; k; k=k->next)
      {
        xdl_method_param* p = k->data;
        NL;
        EL(1, "if (!%s(xr_call_get_param(_call, %d), &%s))", p->type->demarch_name, n++, p->name);
        EL(1, "{");
        EL(2, "xr_call_set_error(_call, -1, \"Stub parameter value demarchalization failed. (%s:%s)\");", m->name, p->name);
        EL(2, "goto out;");
        EL(1, "}");
      }

      // call stub
      NL;
      E(1, "_nreturn_value = %s%sServlet_%s(_servlet", xdl->name, s->name, m->name);
      for (k=m->params; k; k=k->next)
      {
        xdl_method_param* p = k->data;
        E(0, ", %s", p->name);
      }
      EL(0, ", &_error);");

      // check for errors
      EL(1, "if (_error)");
      EL(1, "{");
      EL(2, "xr_call_set_error(_call, _error->code, _error->message);");
      EL(2, "g_error_free(_error);");
      EL(2, "goto out;");
      EL(1, "}");

      // prepare retval
      NL;
      EL(1, "_return_value = %s(_nreturn_value);", m->return_type->march_name);
      EL(1, "if (_return_value == NULL)");
      EL(1, "{");
      EL(2, "xr_call_set_error(_call, -1, \"Stub return value marchalization failed. (%s)\");", m->name);
      EL(2, "goto out;");
      EL(1, "}");
      NL;
      EL(1, "xr_call_set_retval(_call, _return_value);");
      EL(1, "_retval = TRUE;");

      // free native types and return
      NL;
      EL(0, "out:");
      if (m->return_type->free_func)
        EL(1, "%s(_nreturn_value);", m->return_type->free_func);
      for (k=m->params; k; k=k->next)
      {
        xdl_method_param* p = k->data;
        if (!p->pass_ownership && p->type->free_func)
          EL(1, "%s(%s);", p->type->free_func, p->name);
      }
      EL(1, "return _retval;");
      EL(0, "}");
      NL;
    }

    EL(0, "static xr_servlet_method_def __servlet_methods[] = {");
    for (j=s->methods; j; j=j->next)
    {
      xdl_method* m = j->data;
      EL(1, "{");
      EL(2, ".name = \"%s\",", m->name);
      EL(2, ".cb = __method_%s", m->name);
      EL(1, "}%s", j->next ? "," : "");
    }
    EL(0, "};");
    NL;

#define SET_STUB(n) \
  if (s->stub_##n) \
    EL(1, "." G_STRINGIFY(n) " = %s%sServlet_" G_STRINGIFY(n) ",", xdl->name, s->name); \
  else \
    EL(1, "." G_STRINGIFY(n) " = NULL,")

    EL(0, "static xr_servlet_def __servlet = {");
    EL(1, ".name = \"%s%s\",", xdl->name, s->name);
    SET_STUB(init);
    SET_STUB(fini);
    SET_STUB(pre_call);
    SET_STUB(post_call);
    SET_STUB(download);
    SET_STUB(upload);
    EL(1, ".methods_count = %d,", g_slist_length(s->methods));
    EL(1, ".methods = __servlet_methods");
    EL(0, "};");
    NL;

    EL(0, "xr_servlet_def* __%s%sServlet_def()", xdl->name, s->name);
    EL(0, "{");
    EL(1, "__servlet.size = __%s%sServlet_get_priv_size();", xdl->name, s->name);
    EL(1, "return &__servlet;");
    EL(0, "}");
  }

  }
  
  /***********************************************************
   * VAPI file                                               *
   ***********************************************************/

  if (vapi_file)
  {

  OPEN("%s/%s.vapi", out_dir, xdl->name);

  EL(0, "/* VALA bindings */");
  NL;
  EL(0, "[CCode (cheader_filename = \"xr-lib.h\", lower_case_cprefix = \"xr_\", cprefix = \"xr_\")]");
  EL(0, "namespace XR");
  EL(0, "{");
  EL(1, "public static void init();");
  EL(1, "public static void fini();");
  NL;
  EL(1, "[CCode (cheader_filename = \"xr-value.h\", free_function = \"xr_blob_unref\", cname = \"xr_blob\", cprefix = \"xr_blob_\")]");
  EL(1, "public class Blob");
  EL(1, "{");
  EL(2, "public string buf;");
  EL(2, "public int len;");
  EL(2, "[CCode (cname = \"xr_blob_new\")]");
  EL(2, "public Blob(string buf, int len);");
  EL(1, "}");
  NL;
  EL(1, "[CCode (cprefix = \"XRV_\")]");
  EL(1, "public enum ValueType { ARRAY, STRUCT, MEMBER, INT, STRING, BOOLEAN, DOUBLE, TIME, BLOB }");
  NL;
  EL(1, "[CCode (cheader_filename = \"xr-value.h\", unref_function = \"xr_value_unref\", ref_function = \"xr_value_ref\", cname = \"xr_value\", cprefix = \"xr_value_\")]");
  EL(1, "public class Value");
  EL(1, "{");
  EL(2, "[CCode (cname = \"xr_value_int_new\")]");
  EL(2, "public Value.int(int val);");
  EL(2, "[CCode (cname = \"xr_value_string_new\")]");
  EL(2, "public Value.string(string val);");
  EL(2, "[CCode (cname = \"xr_value_bool_new\")]");
  EL(2, "public Value.bool(bool val);");
  EL(2, "[CCode (cname = \"xr_value_double_new\")]");
  EL(2, "public Value.double(double val);");
  EL(2, "[CCode (cname = \"xr_value_time_new\")]");
  EL(2, "public Value.time(string val);");
  EL(2, "[CCode (cname = \"xr_value_blob_new\")]");
  EL(2, "public Value.blob(XR.Blob# val);");
  EL(2, "[CCode (cname = \"xr_value_array_new\")]");
  EL(2, "public Value.array();");
  EL(2, "[CCode (cname = \"xr_value_struct_new\")]");
  EL(2, "public Value.@struct();");
  EL(2, "public bool to_int(ref int nval);");
  EL(2, "public bool to_string(ref string nval);");
  EL(2, "public bool to_bool(ref bool nval);");
  EL(2, "public bool to_double(ref double nval);");
  EL(2, "public bool to_time(ref string nval);");
  EL(2, "public bool to_blob(ref XR.Blob nval);");
  EL(2, "public bool to_value(ref XR.Value nval);");
  EL(2, "public ValueType get_type();");
  EL(2, "public void array_append(Value val);");
  EL(2, "public GLib.SList<weak Value> get_items();");
  EL(2, "public void set_member(string name, Value# val);");
  EL(2, "public weak Value get_member(string name);");
  EL(2, "public weak GLib.SList<weak Value> get_members();");
  EL(2, "public weak string get_member_name();");
  EL(2, "public weak Value get_member_value();");
  EL(2, "public bool is_error_retval(ref int code, ref string msg);");
  EL(2, "public void dump(GLib.String str, int indent);");
  EL(1, "}");
  NL;
  EL(1, "[CCode (cheader_filename = \"xr-call.h\", free_function = \"xr_call_free\", cname = \"xr_call\", cprefix = \"xr_call_\")]");
  EL(1, "public class Call");
  EL(1, "{");
  EL(2, "public Call(string! method);");
  EL(2, "public weak string! get_method();");
  EL(2, "public weak string! get_method_full();");
  EL(2, "public void add_param(Value# val);");
  EL(2, "public weak Value! get_param(uint pos);");
  EL(2, "public void set_retval(Value# val);");
  EL(2, "public weak Value! get_retval();");
  EL(2, "public void set_error(int code, string msg);");
  EL(2, "public int get_error_code();");
  EL(2, "public weak string! get_error_message();");
  EL(2, "public string dump_string(int indent);");
  EL(2, "public void dump(int indent);");
  EL(1, "}");
  EL(0, "}");
  NL;  
  EL(0, "[CCode (cheader_filename = \"%sCommon.h\", lower_case_cprefix = \"xr_\", cprefix = \"%s\")]", xdl->name, xdl->name);
  EL(0, "namespace %s", xdl->name);
  EL(0, "{");
  EL(1, "[CCode (cprefix = \"%s_XMLRPC_ERROR_\")]", xdl->name);
  EL(1, "public enum Error");
  EL(1, "{");
  for (j=xdl->errors; j; j=j->next)
  {
    xdl_error_code* e = j->data;
    EL(2, "%s = %d,", e->name, e->code);
  }
  EL(1, "}");
  for (j=xdl->types; j; j=j->next)
  {
    xdl_typedef* t = j->data;
    if (t->type == TD_STRUCT)
    {
      NL;
      EL(1, "[CCode (free_function = \"%s\", copy_function = \"%s\")]", t->free_func, t->copy_func);
      EL(1, "public class %s", t->name);
      EL(1, "{");
      for (k=t->struct_members; k; k=k->next)
      {
        xdl_struct_member* m = k->data;
        EL(2, "public %s %s;", xdl_typedef_vala_name(m->type), m->name);
      }
      EL(1, "}");
    }
  }

  for (i=xdl->servlets; i; i=i->next)
  {
    xdl_servlet* s = i->data;
    NL;
    EL(1, "[CCode (cheader_filename = \"%s%s.xrc.h\", free_function = \"xr_client_free\", cname = \"xr_client_conn\", cprefix = \"%s%s_\")]", xdl->name, s->name, xdl->name, s->name);
    EL(1, "public class %s", s->name);
    EL(1, "{");
    EL(2, "[CCode (cname = \"xr_client_new\")]");
    EL(2, "public %s() throws GLib.Error;", s->name);
    EL(2, "[CCode (cname = \"xr_client_open\")]");
    EL(2, "public bool open(string uri) throws GLib.Error;");
    EL(2, "[CCode (cname = \"xr_client_close\")]");
    EL(2, "public void close();");
    EL(2, "[CCode (cname = \"xr_client_set_http_header\")]");
    EL(2, "public void set_http_header(string! name, string value);");
    EL(2, "[CCode (cname = \"xr_client_reset_http_headers\")]");
    EL(2, "public void reset_http_headers();");
    EL(2, "[CCode (cname = \"xr_client_basic_auth\")]");
    EL(2, "public void basic_auth(string! username, string! password);");
    for (j=s->methods; j; j=j->next)
    {
      xdl_method* m = j->data;

      E(2, "public %s %s(", xdl_typedef_vala_name(m->return_type), m->name);
      for (k=m->params; k; k=k->next)
      {
        xdl_method_param* p = k->data;
        E(0, "%s %s%s", xdl_typedef_vala_name(p->type), p->name, k->next ? ", " : "");
      }
      EL(0, ") throws GLib.Error;");
    }
    EL(1, "}");
  }
  EL(0, "}");

  }

  if (xdl_gen)
  {
  OPEN("%s/%s.xdl", out_dir, xdl->name);

  EL(0, "/* Generated XDL */");
  NL;

  EL(0, "namespace %s;", xdl->name);
  NL;

  for (j=xdl->errors; j; j=j->next)
  {
    xdl_error_code* e = j->data;
    EL(0, "error %-22s = %d;", e->name, e->code);
  }

  for (j=xdl->types; j; j=j->next)
  {
    xdl_typedef* t = j->data;
    if (t->type == TD_STRUCT)
    {
      NL;
      EL(0, "struct %s", t->name);
      EL(0, "{");
      for (k=t->struct_members; k; k=k->next)
      {
        xdl_struct_member* m = k->data;
        EL(1, "%-24s %s;", xdl_typedef_xdl_name(m->type), m->name);
      }
      EL(0, "}");
    }
  }

  for (i=xdl->servlets; i; i=i->next)
  {
    xdl_servlet* s = i->data;
    NL;
    EL(0, "servlet %s", s->name);
    EL(0, "{");

    for (j=s->errors; j; j=j->next)
    {
      xdl_error_code* e = j->data;
      EL(1, "error %-18s = %d;", e->name, e->code);
    }
    if (s->errors)
      NL;

    for (j=s->types; j; j=j->next)
    {
      xdl_typedef* t = j->data;
      if (t->type == TD_STRUCT)
      {
        EL(1, "struct %s", t->name);
        EL(1, "{");
        for (k=t->struct_members; k; k=k->next)
        {
          xdl_struct_member* m = k->data;
          EL(2, "%-20s %s;", xdl_typedef_xdl_name(m->type), m->name);
        }
        EL(1, "}");
        NL;
      }
    }

    s->methods = g_slist_sort(s->methods, (GCompareFunc)xdl_method_compare);

    for (j=s->methods; j; j=j->next)
    {
      xdl_method* m = j->data;

      E(1, "%-24s %-25s(", xdl_typedef_xdl_name(m->return_type), m->name);
      for (k=m->params; k; k=k->next)
      {
        xdl_method_param* p = k->data;
        if (k == m->params)
          EL(0, "%-15s %s%s", xdl_typedef_xdl_name(p->type), p->name, k->next ? "," : ");");
        else
          EL(0, "%-55s%-15s %s%s", "", xdl_typedef_xdl_name(p->type), p->name, k->next ? "," : ");");
      }
      if (m->params == NULL)
        EL(0, ");");
    }
    EL(0, "}");
  }

  }

  if (js_client)
  {
    OPEN("%s/%s.js", out_dir, xdl->name);

    EL(0, "/* Generated JS client code */");
    NL;
    EL(0, "var %s = {};", xdl->name);

    if (xdl->errors)
      NL;
    for (j = xdl->errors; j; j = j->next)
    {
      xdl_error_code* e = j->data;
      EL(0, "%-30s = %d;", S("%s.%s", xdl->name, e->name), e->code);
    }

    for (i = xdl->servlets; i; i = i->next)
    {
      xdl_servlet* s = i->data;

      NL;
      EL(0, "%s.%s = Class.create(Client,", xdl->name, s->name);
      EL(0, "{");

      for (j=s->methods; j; j=j->next)
      {
        xdl_method* m = j->data;

        E(1, "%s: function(", m->name);
        for (k=m->params; k; k=k->next)
        {
          xdl_method_param* p = k->data;
          E(0, "%s%s", p->name, k->next ? ", " : "");
        }
        EL(0, ")");
        EL(1, "{");
        /*
        for (k=m->params; k; k=k->next)
        {
          xdl_method_param* p = k->data;
          EL(2, "if (!Object.is%s(%s))", p->type->name, p->name);
          EL(3, "throw new Error('Parameter %s must be of type %s.');", p->name, p->type->name);
        }
        */
        E(2, "return this.call('%s%s.%s', [ ", xdl->name, s->name, m->name);
        for (k=m->params; k; k=k->next)
        {
          xdl_method_param* p = k->data;
          E(0, "%s%s", p->name, k->next ? ", " : "");
        }
        EL(0, " ]);");
        EL(1, "}%s", j->next ? "," : "");
        if (j->next)
          NL;
      }
      EL(0, "});");
    }
  }

  /* end */

  if (f)
    fclose(f);

  return 0;
}
