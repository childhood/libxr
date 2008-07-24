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

#ifndef __XDL_MODEL_H__
#define __XDL_MODEL_H__

#include <glib.h>

/** @file xdl model header
 */

/* types */

enum
{
  TD_BASE,
  TD_STRUCT,
  TD_ARRAY,
  TD_BLOB,
  TD_ANY,
};

typedef struct _xdl_typedef xdl_typedef;
typedef struct _xdl_struct_member xdl_struct_member;
typedef struct _xdl_method_param xdl_method_param;
typedef struct _xdl_method xdl_method;
typedef struct _xdl_servlet xdl_servlet;
typedef struct _xdl_model xdl_model;
typedef struct _xdl_error_code xdl_error_code;

struct _xdl_typedef
{
  int type;                /* typedef node type */
  char* name;              /* name of the type (for use in XDL) */
  char* cname;             /* name of the type (for use in XDL) */
  char* ctype;             /* C type name */
  char* cnull;             /* null value in C for this type */
  xdl_servlet* servlet;    /* servlet owning this type */

  char* march_name;
  char* demarch_name;
  char* free_func;
  char* copy_func;

  GSList* struct_members;  /* struct memners list */
  xdl_typedef* item_type; /* array item type */
  char* doc;
};

struct _xdl_struct_member
{
  char* name;
  xdl_typedef* type;
};

/* methods */

struct _xdl_method_param
{
  char* name;
  int pass_ownership;
  xdl_typedef* type;
};

struct _xdl_method
{
  char* name;
  GSList* params;
  xdl_typedef* return_type;
  char* stub_impl;
  int stub_impl_line;
  char* doc;
};

/* servlets */

struct _xdl_servlet
{
  char* name;

  GSList* types;    /* servlet types */
  GSList* methods;  /* methods */
  GSList* errors;

  char* stub_header;
  char* stub_init;
  char* stub_fini;
  char* stub_attrs;
  char* stub_pre_call;
  char* stub_post_call;
  char* stub_fallback;
  char* stub_download;
  char* stub_upload;
  int stub_header_line;
  int stub_init_line;
  int stub_fini_line;
  int stub_attrs_line;
  int stub_pre_call_line;
  int stub_post_call_line;
  int stub_fallback_line;
  int stub_download_line;
  int stub_upload_line;

  char* doc;
};

/* error */

struct _xdl_error_code
{
  char* name;
  char* cenum; /* C enum value */
  int code;
  char* doc;
};

/* parser */

struct _xdl_model
{
  char* name;
  GSList* errors;
  GSList* servlets;
  GSList* types;    /* global types */

  // parser helpers
  xdl_servlet* cur_servlet;
};

xdl_model* xdl_new();

xdl_model* xdl_parse_string(const char* str, GError** err);
xdl_model* xdl_parse_file(const char* str, GError** err);

xdl_error_code* xdl_error_new(xdl_model *xdl, xdl_servlet *servlet, char* name, int code);

xdl_typedef* xdl_typedef_new_array(xdl_model *xdl, xdl_servlet *servlet, xdl_typedef* item);
xdl_typedef* xdl_typedef_new_struct(xdl_model *xdl, xdl_servlet *servlet, char* name);

gint xdl_method_compare(xdl_method* m1, xdl_method* m2);

xdl_typedef* xdl_typedef_find(xdl_model *xdl, xdl_servlet *servlet, const char* name);
char* xdl_typedef_vala_name(xdl_typedef* t);
char* xdl_typedef_xdl_name(xdl_typedef* t);

void xdl_process(xdl_model *ctx);

#endif
