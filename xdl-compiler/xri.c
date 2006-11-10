#include <stdio.h>
#include "xri.h"

struct type t_int = { .type = T_INT, .name = "int" };
struct type t_bool = { .type = T_BOOL, .name = "boolean" };
struct type t_string = { .type = T_STRING, .name = "string" };
struct type t_double = { .type = T_DOUBLE, .name = "double" };
struct type t_time = { .type = T_TIME, .name = "time" };
struct type t_blob = { .type = T_BLOB, .name = "blob" };
struct type t_any = { .type = T_ANY, .name = "any" };

struct parser_context* pctx_new()
{
  struct parser_context* c = g_new0(struct parser_context, 1);
  c->types = g_slist_append(c->types, &t_int);
  c->types = g_slist_append(c->types, &t_bool);
  c->types = g_slist_append(c->types, &t_string);
  c->types = g_slist_append(c->types, &t_double);
  c->types = g_slist_append(c->types, &t_time);
  c->types = g_slist_append(c->types, &t_blob);
  c->types = g_slist_append(c->types, &t_any);
  return c;
}

struct type* find_type(struct parser_context *ctx, const char* name)
{
  GSList* i;
  for (i=ctx->types; i; i=i->next)
  {
    struct type* t = i->data;
    if (!strcmp(t->name, name))
      return t;
  }
  return NULL;
}

//////
#define EL(i, fmt, args...) \
  line_fprintf(f, i, fmt "\n", ##args)

#define E(i, fmt, args...) \
  line_fprintf(f, i, fmt, ##args)

#define NL \
  fprintf(f, "\n")

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
//////

char* get_type(struct type* t)
{
  if (t->type == T_STRUCT)
    return g_strdup_printf("struct %s*", t->name);
  else if (t->type == T_ARRAY)
    return "GSList*";
  else if (t->type == T_INT || t->type == T_BOOL)
    return "int";
  else if (t->type == T_STRING)
    return "char*";
  else if (t->type == T_DOUBLE)
    return "double";
  else if (t->type == T_TIME)
    return "time_t";
  else if (t->type == T_BLOB)
    return "blob*";
  else if (t->type == T_ANY)
    return "any*";
  printf("Unknown type %s\n", t->name);
  exit(1);
}

void gen_c(struct parser_context *ctx, const char* name)
{
  FILE* f = fopen(name, "w");
  if (f == NULL)
  {
    printf("Can't open output file for writing.");
    return;
  }

  EL(0, "#include <glib.h>");
  NL;

  GSList *i, *j;

/*
  for (i=ctx->types; i; i=i->next)
  { struct type* t = i->data;
    if (t->type != T_STRUCT)
      continue;
    EL(0, "typedef struct _%s %s;", t->name, t->name);
  }
  NL;
*/

  for (i=ctx->types; i; i=i->next)
  { struct type* t = i->data;
    
    if (t->type != T_STRUCT)
      continue;
    
    EL(0, "struct %s", t->name);
    EL(0, "{");
    for (j=t->members; j; j=j->next)
    { struct member* m = j->data;
      EL(1, "%s %s;", get_type(m->type), m->name);
    }
    EL(0, "};");
    NL;
  }

  for (i=ctx->methods; i; i=i->next)
  { struct method* m = i->data;

    E(0, "%s %s(", get_type(m->return_type), m->name);

    for (j=m->params; j; j=j->next)
    { struct param* p = j->data;
      E(0, "%s %s%s", get_type(p->type), p->name, j->next ? ", " : "");
    }

    EL(0, ");");
  }
}
