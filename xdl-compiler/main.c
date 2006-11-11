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

  for (j=xdl->types; j; j=j->next)
  {
    xdl_typedef* t = j->data;

    if (t->type == TD_STRUCT)
    {
      EL(0, "typedef struct _%s %s;", t->name, t->name);
      EL(0, "struct _%s", t->name);
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
    if (t->type == TD_STRUCT)
    {
      EL(0, "xr_value* __%s_to_xr_value(%s val);", t->name, t->ctype);
      EL(0, "int __xr_value_to_%s(xr_value* xrv, %s* val);", t->name, t->ctype);
      NL;
    }
  }

  EL(0, "#endif");

  OPEN("%s/%sCommon.xrm.c", out_dir, xdl->name);

  EL(0, "#include \"%sCommon.xrm.h\"", xdl->name);
  NL;

  for (j=xdl->types; j; j=j->next)
  {
    xdl_typedef* t = j->data;
    if (t->type == TD_STRUCT)
    {
      EL(0, "xr_value* __%s_to_xr_value(%s val)", t->name, t->ctype);
      EL(0, "{");
      //XXX: impleemnt
      EL(0, "}");
      NL;
      EL(0, "int __xr_value_to_%s(xr_value* xrv, %s* val)", t->name, t->ctype);
      EL(0, "{");
      //XXX: impleemnt
      EL(0, "}");
      NL;
    }
  }

  /* client/servlet implementations for specific interfaces */

  for (i=xdl->servlets; i; i=i->next)
  {
    xdl_servlet* s = i->data;

    OPEN("%s/%s%s.h", out_dir, xdl->name, s->name);

    EL(0, "#ifndef __%s_%s_H__", xdl->name, s->name);
    EL(0, "#define __%s_%s_H__", xdl->name, s->name);
    NL;

//    EL(0, "#include <xr-value.h>");
    EL(0, "#include <xr-client.h>");
    EL(0, "#include \"%sCommon.h\"", xdl->name);
    NL;

    for (j=s->types; j; j=j->next)
    {
      xdl_typedef* t = j->data;
  
      if (t->type == TD_STRUCT)
      {
        EL(0, "typedef struct _%s %s;", t->name, t->name);
        EL(0, "struct _%s", t->name);
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
  }
  
  fclose(f);

  return 0;
}
