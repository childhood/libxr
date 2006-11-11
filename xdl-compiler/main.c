#include <glib.h>
#include <stdio.h>

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
      EL(0, "static void %s(%s val)", t->free_func, t->ctype);
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

      EL(0, "static xr_value* %s(%s val)", t->march_name, t->ctype);
      EL(0, "{");
      EL(1, "xr_value* v = xr_value_struct_new();");
      for (k=t->struct_members; k; k=k->next)
      {
        xdl_struct_member* m = k->data;
        EL(1, "xr_value_struct_set_member(v, \"%s\", %s(val->%s));", m->name, m->type->march_name, m->name);
      }
      EL(1, "return v;");
      EL(0, "}");
      NL;

      EL(0, "static int %s(xr_value* v, %s* val)", t->demarch_name, t->ctype);
      EL(0, "{");
      EL(1, "if (v == NULL)");
      EL(2, "return -1;");
      EL(1, "%s s = g_new0(typeof(*s), 1);", t->ctype, t->ctype);
      EL(1, "if (");
      for (k=t->struct_members; k; k=k->next)
      {
        xdl_struct_member* m = k->data;
        EL(2, "%s(xr_value_get_member(v, \"%s\"), &s->%s)%s", m->type->demarch_name, m->name, m->name, k->next ? " ||" : "");
      }
      EL(1, ")");
      EL(1, "{");
      EL(2, "%s(s);", t->free_func);
      EL(2, "return -1;");
      EL(1, "}");
      EL(1, "*val = s;");
      EL(1, "return 0;");
      EL(0, "}");
      NL;
    }
    else if (t->type == TD_ARRAY)
    {
      EL(0, "static void %s(%s val)", t->free_func, t->ctype);
      EL(0, "{");
      if (t->item_type->free_func)
        EL(1, "g_slist_foreach(val, (GFunc)%s, NULL);", t->item_type->free_func);
      EL(1, "g_slist_free(val);");
      EL(0, "}");
      NL;

      EL(0, "static xr_value* %s(%s val)", t->march_name, t->ctype);
      EL(0, "{");
      EL(1, "GSList* i;");
      EL(1, "xr_value* v = xr_value_array_new();");
      EL(1, "for (i = val; i; i = i->next)");
      EL(2, "xr_value_array_append(v, %s(i->data));", t->item_type->march_name);
      EL(1, "return v;");
      EL(0, "}");
      NL;

      EL(0, "static int %s(xr_value* v, %s* val)", t->demarch_name, t->ctype);
      EL(0, "{");
      EL(1, "GSList *a, *i;");
      EL(1, "%s ival;", t->item_type->ctype);
      EL(1, "if (v == NULL)");
      EL(2, "return -1;");
      EL(1, "for (i = xr_value_get_items(v); i; i = i->next)");
      EL(1, "{");
      EL(2, "%s(i->data, &ival);", t->item_type->demarch_name);
      EL(2, "a = g_slist_append(a, ival);");
      EL(1, "}");
      EL(1, "*val = a;");
      EL(1, "return 0;");
      EL(0, "}");
      NL;
    }
}

/* main() */

static gchar* out_dir = NULL;
static gchar* xdl_file = NULL;
static GOptionEntry entries[] = 
{
  { "xdl", 'i', 0, G_OPTION_ARG_STRING, &xdl_file, "Interface description file.", "FILE" },
  { "out", 'o', 0, G_OPTION_ARG_STRING, &out_dir, "Output directory.", "DIR" },
  { NULL }
};

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

  /* common types header */

  OPEN("%s/%sCommon.h", out_dir, xdl->name);

  EL(0, "#ifndef __%s_Common_H__", xdl->name);
  EL(0, "#define __%s_Common_H__", xdl->name);
  NL;

  EL(0, "#include <xr-value.h>");
  NL;

  for (j=xdl->types; j; j=j->next)
  {
    xdl_typedef* t = j->data;
    if (t->type == TD_STRUCT)
      EL(0, "typedef struct _%s %s;", t->cname, t->cname);
  }
  NL;

  for (j=xdl->types; j; j=j->next)
  {
    xdl_typedef* t = j->data;
    if (t->type == TD_STRUCT)
    {
      EL(0, "struct _%s", t->cname);
      EL(0, "{");
      for (k=t->struct_members; k; k=k->next)
      {
        xdl_struct_member* m = k->data;
        EL(1, "%s %s;", m->type->ctype, m->name);
      }
      EL(0, "};");
      NL;
    }
  }

  EL(0, "#endif");

  /* common types marchalizer/demarchalizer functions */

  OPEN("%s/%sCommon.xrm.h", out_dir, xdl->name);

  EL(0, "#ifndef __%s_Common_XRM_H__", xdl->name);
  EL(0, "#define __%s_Common_XRM_H__", xdl->name);
  NL;

  EL(0, "#include \"%sCommon.h\"", xdl->name);
  NL;

  for (j=xdl->types; j; j=j->next)
  {
    xdl_typedef* t = j->data;
    gen_type_marchalizers(f, t);
  }

  EL(0, "#endif");

  /* client/servlet implementations for specific interfaces */

  for (i=xdl->servlets; i; i=i->next)
  {
    xdl_servlet* s = i->data;

    OPEN("%s/%s%s.h", out_dir, xdl->name, s->name);

    EL(0, "#ifndef __%s_%s_H__", xdl->name, s->name);
    EL(0, "#define __%s_%s_H__", xdl->name, s->name);
    NL;

    EL(0, "#include <xr-client.h>");
    EL(0, "#include \"%sCommon.h\"", xdl->name);
    NL;

    for (j=s->types; j; j=j->next)
    {
      xdl_typedef* t = j->data;
      if (t->type == TD_STRUCT)
        EL(0, "typedef struct _%s %s;", t->cname, t->cname);
    }

    for (j=s->types; j; j=j->next)
    {
      xdl_typedef* t = j->data;
  
      if (t->type == TD_STRUCT)
      {
        NL;
        EL(0, "struct _%s", t->cname);
        EL(0, "{");
        for (k=t->struct_members; k; k=k->next)
        {
          xdl_struct_member* m = k->data;
          EL(1, "%s %s;", m->type->ctype, m->name);
        }
        EL(0, "};");
      }
    }
    NL;

    for (j=s->methods; j; j=j->next)
    {
      xdl_method* m = j->data;

      E(0, "%s %s%s_%s(xr_client_conn* conn", m->return_type->ctype, xdl->name, s->name, m->name);
      for (k=m->params; k; k=k->next)
      {
        xdl_method_param* p = k->data;
        E(0, ", %s %s", p->type->ctype, p->name);
      }
      EL(0, ");");
    }
    NL;

    EL(0, "#endif");

    /* common types marchalizer/demarchalizer functions */

    OPEN("%s/%s%s.xrm.h", out_dir, xdl->name, s->name);

    EL(0, "#ifndef __%s_%s_XRM_H__", xdl->name, s->name);
    EL(0, "#define __%s_%s_XRM_H__", xdl->name, s->name);
    NL;

    EL(0, "#include \"%s%s.h\"", xdl->name, s->name);
    EL(0, "#include \"%sCommon.xrm.h\"", xdl->name);
    NL;

    for (j=s->types; j; j=j->next)
    {
      xdl_typedef* t = j->data;
      gen_type_marchalizers(f, t);
    }

    EL(0, "#endif");

    /* client interface */

    OPEN("%s/%s%s.xrc.c", out_dir, xdl->name, s->name);

    EL(0, "#include \"%s%s.h\"", xdl->name, s->name);
    EL(0, "#include \"%s%s.xrm.h\"", xdl->name, s->name);
    NL;

    for (j=s->methods; j; j=j->next)
    {
      xdl_method* m = j->data;

      E(0, "%s %s%s_%s(xr_client_conn* conn", m->return_type->ctype, xdl->name, s->name, m->name);
      for (k=m->params; k; k=k->next)
      {
        xdl_method_param* p = k->data;
        E(0, ", %s %s", p->type->ctype, p->name);
      }
      EL(0, ")");
      EL(0, "{");
      EL(1, "%s retval = %s;", m->return_type->ctype, m->return_type->cnull);
      EL(1, "g_assert(conn != NULL);");
      for (k=m->params; k; k=k->next)
      {
        xdl_method_param* p = k->data;
        if (!strcmp(p->type->cnull, "NULL"))
          EL(1, "g_assert(%s != NULL);", p->name);
      }
      EL(1, "xr_call* call = xr_call_new(\"%s\");", m->name);
      for (k=m->params; k; k=k->next)
      {
        xdl_method_param* p = k->data;
        EL(1, "xr_call_add_param(call, %s(%s));", p->type->march_name, p->name);
      }
      EL(1, "xr_client_call_ex(conn, call, (xr_demarchalizer_t)%s, (void**)&retval);", m->return_type->demarch_name);
      EL(1, "xr_call_free(call);");
      EL(1, "return retval;");
      EL(0, "}");
      NL;
    }

    /* server interface */

    OPEN("%s/%s%s.xrs.c", out_dir, xdl->name, s->name);

    EL(0, "#include \"%s%s.h\"", xdl->name, s->name);
    EL(0, "#include \"%s%s.xrm.h\"", xdl->name, s->name);
    NL;

    EL(0, "typedef struct _%s%sServlet %s%sServlet;", xdl->name, s->name, xdl->name, s->name);
    NL;

    EL(0, "struct _%s%sServlet", xdl->name, s->name);
    EL(0, "{");
    EL(1, "xr_servlet* servlet;");
    EL(0, "};");
    NL;

    EL(0, "int %s%sServlet_init(%s%sServlet* _s)", xdl->name, s->name, xdl->name, s->name);
    EL(0, "{");
    EL(0, "}");
    NL;

    EL(0, "void %s%sServlet_fini(%s%sServlet* _s)", xdl->name, s->name, xdl->name, s->name);
    EL(0, "{");
    EL(0, "}");
    NL;

    for (j=s->methods; j; j=j->next)
    {
      xdl_method* m = j->data;

      E(0, "%s %s%sServlet_%s(%s%sServlet* _s", m->return_type->ctype, xdl->name, s->name, m->name, xdl->name, s->name);
      for (k=m->params; k; k=k->next)
      {
        xdl_method_param* p = k->data;
        E(0, ", %s %s", p->type->ctype, p->name);
      }
      EL(0, ")");
      EL(0, "{");
      EL(1, "%s retval = %s;", m->return_type->ctype, m->return_type->cnull);
      EL(1, "RETURN_ERROR(100, \"%s is not implemented!\");", m->name);
      EL(1, "return retval;");
      EL(0, "}");
      NL;
    }

    for (j=s->methods; j; j=j->next)
    {
      xdl_method* m = j->data;

      EL(0, "int __%s%sServlet_%s(%s%sServlet* _s, xr_call* _call)", xdl->name, s->name, m->name, xdl->name, s->name);
      EL(0, "{");
      // prepare parameters
      if (m->params)
      {
        for (k=m->params; k; k=k->next)
        {
        xdl_method_param* p = k->data;
          EL(1, "%s %s = %s;", p->type->ctype, p->name, p->type->cnull);
        }
        int n = 0;
        EL(1, "if (");
        for (k=m->params; k; k=k->next)
        {
          xdl_method_param* p = k->data;
          EL(2, "%s(xr_call_get_param(_call, %d), &%s)%s", p->type->demarch_name, n++, p->name, k->next ? " ||" : "");
        }
        EL(1, ")");
        EL(1, "{");
        for (k=m->params; k; k=k->next)
        {
          xdl_method_param* p = k->data;
           if (p->type->free_func)
            EL(2, "%s(%s);", p->type->free_func, p->name);
        }
        EL(2, "return -1;");
        EL(1, "}");
      }

      // call stub
      E(1, "%s _retval = %s%sServlet_%s(_s", m->return_type->ctype, xdl->name, s->name, m->name);
      for (k=m->params; k; k=k->next)
      {
        xdl_method_param* p = k->data;
        E(0, ", %s", p->name);
      }
      EL(0, ");");

      // free params
      for (k=m->params; k; k=k->next)
      {
        xdl_method_param* p = k->data;
        if (p->type->free_func)
          EL(1, "%s(%s);", p->type->free_func, p->name);
      }

      // prepare retval
      EL(1, "xr_call_set_retval(%s(_retval));", m->return_type->march_name);
      if (m->return_type->free_func)
        EL(1, "%s(_retval);", m->return_type->free_func);
      EL(1, "return 0;");
      EL(0, "}");
      NL;
    }
  }
  
  fclose(f);

  return 0;
}
