#ifndef __MPKG_MODEL_H__
#define __MPKG_MODEL_H__

#include <glib.h>

/** @file mpkg model header
*/

struct parser_context
{
  GSList* types;
  GSList* methods;
};

enum { T_INT, T_BOOL, T_STRING, T_DOUBLE, T_TIME, T_BLOB, T_STRUCT, T_ARRAY, T_ANY };

struct type
{
  int type;
  char* name;
  GSList* members; // for struct
  struct type* elem_type; // for array
};

struct member
{
  struct type* type;
  char* name;
};

struct param
{
  struct type* type;
  char* name;
};

struct method
{
  char* name;
  struct type* return_type;
  GSList* params;
};

int xri_load(struct parser_context *ctx, const char* path);
struct type* find_type(struct parser_context *ctx, const char* name);
struct parser_context* pctx_new();
void gen_c(struct parser_context *ctx, const char* name);

#endif
