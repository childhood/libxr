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
  fprintf(f, "\n")

#define S(args...) \
  g_strdup_printf(args)

#define OPEN(fmt, args...) { \
  if (f) \
    fclose(f); \
  f = fopen(S(fmt, ##args), "w"); \
  if (f == NULL) { \
    fprintf(stderr, "Can't open output file for writing."); exit(1); } \
  }

static void line_fprintf(FILE* f, int indent, const char* fmt, ...)
{
  va_list ap;
  int i;
  for (i=0;i<indent;i++)
    fprintf(f, "\t");
  va_start(ap, fmt);
  vfprintf(f, fmt, ap);
  va_end(ap);
}

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
      EL(1, "if (_nstruct == NULL)");
      EL(2, "return NULL;");
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
        EL(2, "xr_value_free(%s);", m->name);
      }
      EL(2, "return NULL;");
      EL(1, "}");
      EL(1, "_struct = xr_value_struct_new();");
      for (k=t->struct_members; k; k=k->next)
      {
        xdl_struct_member* m = k->data;
        EL(1, "xr_value_struct_set_member(_struct, \"%s\", %s);", m->name, m->name);
      }
      EL(1, "return _struct;");
      EL(0, "}");
      NL;

      EL(0, "G_GNUC_UNUSED static int %s(xr_value* _struct, %s* _nstruct)", t->demarch_name, t->ctype);
      EL(0, "{");
      EL(1, "%s _tmp_nstruct;", t->ctype);
      EL(1, "g_assert(_nstruct != NULL);");
      EL(1, "if (_struct == NULL || xr_value_get_type(_struct) != XRV_STRUCT)");
      EL(2, "return -1;");
      EL(1, "_tmp_nstruct = %s_new();", t->cname);
      EL(1, "if (");
      for (k=t->struct_members; k; k=k->next)
      {
        xdl_struct_member* m = k->data;
        EL(2, "%s(xr_value_get_member(_struct, \"%s\"), &_tmp_nstruct->%s)%s", m->type->demarch_name, m->name, m->name, k->next ? " ||" : "");
      }
      EL(1, ")");
      EL(1, "{");
      EL(2, "%s(_tmp_nstruct);", t->free_func);
      EL(2, "return -1;");
      EL(1, "}");
      EL(1, "*_nstruct = _tmp_nstruct;");
      EL(1, "return 0;");
      EL(0, "}");
      NL;
    }
    else if (t->type == TD_ARRAY)
    {
      EL(0, "G_GNUC_UNUSED static xr_value* %s(%s _narray)", t->march_name, t->ctype);
      EL(0, "{");
      EL(1, "GSList* _item;");
      EL(1, "xr_value* _array = xr_value_array_new();");
      EL(1, "for (_item = _narray; _item; _item = _item->next)");
      EL(1, "{");
      EL(2, "xr_value* _item_value = %s((%s)_item->data);", t->item_type->march_name, t->item_type->ctype);
      EL(2, "if (_item_value == NULL)");
      EL(2, "{");
      EL(3, "xr_value_free(_array);");
      EL(3, "return NULL;");
      EL(2, "}");
      EL(2, "xr_value_array_append(_array, _item_value);");
      EL(1, "}");
      EL(1, "return _array;");
      EL(0, "}");
      NL;

      EL(0, "G_GNUC_UNUSED static int %s(xr_value* _array, %s* _narray)", t->demarch_name, t->ctype);
      EL(0, "{");
      EL(1, "GSList *_tmp_narray = NULL, *_item;");
      EL(1, "g_assert(_narray != NULL);");
      EL(1, "if (_array == NULL || xr_value_get_type(_array) != XRV_ARRAY)");
      EL(2, "return -1;");
      EL(1, "for (_item = xr_value_get_items(_array); _item; _item = _item->next)");
      EL(1, "{");
      EL(2, "%s _item_value = %s;", t->item_type->ctype, t->item_type->cnull);
      EL(2, "if (%s((xr_value*)_item->data, &_item_value))", t->item_type->demarch_name);
      EL(2, "{");
      EL(3, "%s(_tmp_narray);", t->free_func);
      EL(3, "return -1;");
      EL(2, "}");
      EL(2, "_tmp_narray = g_slist_append(_tmp_narray, (void*)_item_value);");
      EL(1, "}");
      EL(1, "*_narray = _tmp_narray;");
      EL(1, "return 0;");
      EL(0, "}");
      NL;
    }
}

void gen_type_freealloc(FILE* f, xdl_typedef* t, int def)
{
  GSList *i, *j, *k;

  if (def)
  {
    if (t->type == TD_STRUCT || t->type == TD_ARRAY)
      EL(0, "void %s(%s val);", t->free_func, t->ctype);
    if (t->type == TD_STRUCT)
      EL(0, "%s %s_new();", t->ctype, t->cname);
    return;
  }

  if (t->type == TD_STRUCT)
  {
    EL(0, "void %s(%s val)", t->free_func, t->ctype);
    EL(0, "{");
    EL(1, "if (val == NULL)");
    EL(2, "return;");
    for (k=t->struct_members; k; k=k->next)
    {
      xdl_struct_member* m = k->data;
      if (m->type->free_func)
        EL(1, "%s(val->%s);", m->type->free_func, m->name);
    }
    EL(1, "g_free(val);");
    EL(0, "}");
    NL;

    EL(0, "%s %s_new()", t->ctype, t->cname);
    EL(0, "{");
    EL(1, "return g_new0(%s, 1);", t->cname);
    EL(0, "}");
    NL;
  }
  else if (t->type == TD_ARRAY)
  {
    EL(0, "void %s(%s val)", t->free_func, t->ctype);
    EL(0, "{");
    if (t->item_type->free_func)
      EL(1, "g_slist_foreach(val, (GFunc)%s, NULL);", t->item_type->free_func);
    EL(1, "g_slist_free(val);");
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
    gen_type_marchalizers(f, t);
  }
  for (j=s->types; j; j=j->next)
  {
    xdl_typedef* t = j->data;
    gen_type_marchalizers(f, t);
  }
}

/* main() */

static gchar* out_dir = NULL;
static gchar* xdl_file = NULL;
static gchar* mode = "all";
static GOptionEntry entries[] = 
{
  { "xdl", 'i', 0, G_OPTION_ARG_STRING, &xdl_file, "Interface description file.", "FILE" },
  { "out", 'o', 0, G_OPTION_ARG_STRING, &out_dir, "Output directory.", "DIR" },
  { "mode", 'm', 0, G_OPTION_ARG_STRING, &mode, "Compiler mode (all, server-impl, pub-headers, pub-impl).", "MODE" },
  { NULL, 0, 0, 0, NULL, NULL, NULL }
};

#define MODE_IS(str) !strcmp(mode, G_STRINGIFY(str))

int main(int ac, char* av[])
{
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
  
  xdl_model* xdl = xdl_new();
  xdl_load(xdl, xdl_file);
  xdl_process(xdl);

  FILE* f = NULL;
  GSList *i, *j, *k;
  
  int pub_headers = !strcmp(mode, "all") || !strcmp(mode, "pub-headers");
  int pub_impl = !strcmp(mode, "all") || !strcmp(mode, "pub-impl");
  int server_impl = !strcmp(mode, "all") || !strcmp(mode, "server-impl");

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
        E(0, ", %s %s", p->type->ctype, p->name);
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
        E(0, ", %s %s", p->type->ctype, p->name);
      }
      EL(0, ", GError** _error)");
      EL(0, "{");
      EL(1, "%s _retval = %s;", m->return_type->ctype, m->return_type->cnull);
      EL(1, "xr_value* _param_value;");
      EL(1, "g_assert(_conn != NULL);");
      EL(1, "g_return_val_if_fail(_error == NULL || *_error == NULL, _retval);");
      EL(1, "xr_call* _call = xr_call_new(\"%s\");", m->name);
      for (k=m->params; k; k=k->next)
      {
        xdl_method_param* p = k->data;
        EL(1, "_param_value = %s(%s);", p->type->march_name, p->name);
        EL(1, "if (_param_value == NULL)");
        EL(1, "{");
        EL(2, "g_set_error(_error, XR_CLIENT_ERROR, XR_CLIENT_ERROR_MARCHALIZER, \"Call parameter value marchalization failed (param=%s).\");", p->name);
        EL(2, "xr_call_free(_call);");
        EL(2, "return _retval;");
        EL(1, "}");
        EL(1, "xr_call_add_param(_call, _param_value);");
      }
      EL(1, "if (xr_client_call(_conn, _call, _error) == 0)");
      EL(1, "{");
      EL(2, "if (%s(xr_call_get_retval(_call), &_retval) < 0)", m->return_type->demarch_name);
      EL(3, "g_set_error(_error, XR_CLIENT_ERROR, XR_CLIENT_ERROR_MARCHALIZER, \"Call return value demarchalization failed.\");");
      EL(1, "}");
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
    EL(0, " * @return ");
    EL(0, " */ ");
    EL(0, "int %s%sServlet_init(xr_servlet* _servlet);", xdl->name, s->name);
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
    EL(0, "int %s%sServlet_pre_call(xr_servlet* _servlet, xr_call* _call);", xdl->name, s->name);
    NL;

    EL(0, "/** Post-call hook.");
    EL(0, " * ");
    EL(0, " * @param _servlet Servlet object.");
    EL(0, " * @param _call Call object.");
    EL(0, " * ");
    EL(0, " * @return TRUE if you want to continue execution of the call.");
    EL(0, " */ ");
    EL(0, "int %s%sServlet_post_call(xr_servlet* _servlet, xr_call* _call);", xdl->name, s->name);
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
      EL(0, "#line %d \"%s\"", s->stub_header_line, xdl_file);
      EL(1, "%s", s->stub_header);
      NL;
    }

    EL(0, "struct _%s%sServlet", xdl->name, s->name);
    EL(0, "{");
    if (s->stub_attrs)
    {
      EL(0, "#line %d \"%s\"", s->stub_attrs_line, xdl_file);
      EL(1, "%s", s->stub_attrs);
    }
    EL(0, "};");
    NL;

    EL(0, "int __%s%sServlet_get_priv_size()", xdl->name, s->name);
    EL(0, "{");
    EL(1, "return sizeof(%s%sServlet);", xdl->name, s->name);
    EL(0, "}");
    NL;

    EL(0, "int %s%sServlet_init(xr_servlet* _servlet)", xdl->name, s->name);
    EL(0, "{");
    EL(1, "%s%sServlet* _priv = xr_servlet_get_priv(_servlet);", xdl->name, s->name);
    if (s->stub_init)
    {
      EL(0, "#line %d \"%s\"", s->stub_init_line, xdl_file);
      EL(1, "%s", s->stub_init);
    }
    EL(1, "return 0;");
    EL(0, "}");
    NL;

    EL(0, "void %s%sServlet_fini(xr_servlet* _servlet)", xdl->name, s->name);
    EL(0, "{");
    EL(1, "%s%sServlet* _priv = xr_servlet_get_priv(_servlet);", xdl->name, s->name);
    if (s->stub_fini)
    {
      EL(0, "#line %d \"%s\"", s->stub_fini_line, xdl_file);
      EL(1, "%s", s->stub_fini);
    }
    EL(0, "}");
    NL;

    EL(0, "int %s%sServlet_pre_call(xr_servlet* _servlet, xr_call* _call)", xdl->name, s->name);
    EL(0, "{");
    EL(1, "%s%sServlet* _priv = xr_servlet_get_priv(_servlet);", xdl->name, s->name);
    if (s->stub_pre_call)
    {
      EL(0, "#line %d \"%s\"", s->stub_pre_call_line, xdl_file);
      EL(1, "%s", s->stub_pre_call);
    }
    EL(1, "return TRUE;");
    EL(0, "}");
    NL;

    EL(0, "int %s%sServlet_post_call(xr_servlet* _servlet, xr_call* _call)", xdl->name, s->name);
    EL(0, "{");
    EL(1, "%s%sServlet* _priv = xr_servlet_get_priv(_servlet);", xdl->name, s->name);
    if (s->stub_post_call)
    {
      EL(0, "#line %d \"%s\"", s->stub_post_call_line, xdl_file);
      EL(1, "%s", s->stub_post_call);
    }
    EL(1, "return TRUE;");
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
      {
        EL(0, "#line %d \"%s\"", m->stub_impl_line, xdl_file);
        EL(1, "%s", m->stub_impl);
      }
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

      EL(0, "static int __method_%s(xr_servlet* _servlet, xr_call* _call)", m->name);
      EL(0, "{");
      // forward declarations
      EL(1, "int _retval = -1;");
      EL(1, "%s _nreturn_value = %s;", m->return_type->ctype, m->return_type->cnull);
      EL(1, "xr_value* _return_value;");
      EL(1, "GError* _error = NULL;");
      for (k=m->params; k; k=k->next)
      {
        xdl_method_param* p = k->data;
        EL(1, "%s %s = %s;", p->type->ctype, p->name, p->type->cnull);
      }
      EL(1, "g_assert(_servlet != NULL);");
      EL(1, "g_assert(_call != NULL);");
      // prepare parameters
      for (k=m->params; k; k=k->next)
      {
        xdl_method_param* p = k->data;
        EL(1, "if (%s(xr_call_get_param(_call, %d), &%s))", p->type->demarch_name, n++, p->name);
        EL(1, "{");
        EL(2, "xr_call_set_error(_call, -1, \"Stub parameter value demarchalization failed. (%s:%s)\");", m->name, p->name);
        EL(2, "goto out;");
        EL(1, "}");
      }

      // call stub
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
      EL(1, "_return_value = %s(_nreturn_value);", m->return_type->march_name);
      EL(1, "if (_return_value == NULL)");
      EL(1, "{");
      EL(2, "xr_call_set_error(_call, -1, \"Stub return value marchalization failed. (%s)\");", m->name);
      EL(2, "goto out;");
      EL(1, "}");
      EL(1, "xr_call_set_retval(_call, _return_value);");

      // free native types and return
      EL(0, "out:");
      if (m->return_type->free_func)
        EL(1, "%s(_nreturn_value);", m->return_type->free_func);
      for (k=m->params; k; k=k->next)
      {
        xdl_method_param* p = k->data;
        if (p->type->free_func)
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

    EL(0, "static xr_servlet_def __servlet = {");
    EL(1, ".name = \"%s%s\",", xdl->name, s->name);
    EL(1, ".init = %s%sServlet_init,", xdl->name, s->name);
    EL(1, ".fini = %s%sServlet_fini,", xdl->name, s->name);
    EL(1, ".pre_call = %s%sServlet_pre_call,", xdl->name, s->name);
    EL(1, ".post_call = %s%sServlet_post_call,", xdl->name, s->name);
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
  
  fclose(f);

  return 0;
}
