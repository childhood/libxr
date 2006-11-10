/*
 * this file implements genie commands multiplexing
 */

#include <glib.h>
#include "xri.h"

static gchar* xri_file = NULL;

static GOptionEntry entries[] = 
{
  { "xri", 'i', 0, G_OPTION_ARG_STRING, &xri_file, "Interface description file.", "FILE" },
  { NULL }
};

int main(int ac, char* av[])
{
  GOptionContext* ctx = g_option_context_new("- XML-RPC Interface Builder.");
  g_option_context_add_main_entries(ctx, entries, NULL);
  g_option_context_parse(ctx, &ac, &av, NULL);
  g_option_context_set_help_enabled(ctx, TRUE);

  if (xri_file == NULL)
  {
    printf("You must specify XRI source file.\n");
    return 1;
  }
  
  struct parser_context* c = pctx_new();
  xri_load(c, xri_file);
  gen_c(c, "out.c");

  return 0;

  GSList *i, *j;

  printf("TYPES:\n");
  for (i=c->types; i; i=i->next)
  {
    struct type* t = i->data;
    printf("  %s\n", t->name);
  }

  printf("METHODS:\n");
  for (i=c->methods; i; i=i->next)
  {
    struct method* m = i->data;
    printf("  %s%s %s\n", m->return_type->type & T_ARRAY ? "[]" : "", m->return_type->name, m->name);

    for (j=m->params; j; j=j->next)
    {
      struct param* p = j->data;
      printf("    %s %s%s\n", p->type->name, p->name, p->type->type & T_ARRAY ? "[]" : "");
    }
  }
  
  return 0;
}
