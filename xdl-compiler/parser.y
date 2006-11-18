%{

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "xdl.h"
#include "parser.h"

#define yyin xdl_in
#define yylex_destroy xdl_lex_destroy

int yylex (YYSTYPE *yylval_param, YYLTYPE *yylloc_param, xdl_model *xdl);
static void yyerror(YYLTYPE *locp, xdl_model *xdl, const char *msg);

xdl_servlet* cur_servlet = NULL;

%}

%defines
%locations
%pure-parser
%parse-param {xdl_model *xdl}
%lex-param {xdl_model *xdl}
%error-verbose
%name-prefix="xdl_"

%union {
  int num;
  char* str;
  GSList* list;
  xdl_typedef* type;
  xdl_method_param* param;
  xdl_struct_member* member;
  xdl_method* method;
  xdl_servlet* servlet;
}

%token OPEN_BRACE "{"
%token CLOSE_BRACE "}"
%token OPEN_SBRACE "<"
%token CLOSE_SBRACE ">"
%token OPEN_RBRACE "("
%token CLOSE_RBRACE ")"
%token SEMICOLON ";"
%token COMMA ","
%token STRUCT "struct"
%token ARRAY "array"
%token SERVLET "servlet"
%token NAMESPACE "namespace"

%token KW_INIT "__init__"
%token KW_FINI "__fini__"
%token KW_ATTRS "__attrs__"

%token <str> DOC_COMMENT
%token <str> STRING_LITERAL
%token <str> IDENTIFIER
%token <str> INLINE_CODE
%token <num> INTEGER_LITERAL

%type <str> opt_inline_code
%type <type> type
%type <type> struct_decl
%type <member> struct_member
%type <list> struct_members
%type <list> params opt_params
%type <param> param
%type <method> method_decl
%type <servlet> servlet_decl

%start compilation_unit

%%

compilation_unit
  : opt_namespace_decl opt_inline_code toplevel_decls
    {
      xdl->stub_header = $2;
    }
  ;

opt_inline_code
  :
    {
      $$ = NULL;
    }
  | INLINE_CODE
  ;

opt_namespace_decl
  :
  | namespace_decl
  ;

namespace_decl
  : "namespace" IDENTIFIER ";"
    {
      xdl->name = g_strdup($2);
    }
  ;

toplevel_decls
  : toplevel_decl
  | toplevel_decls toplevel_decl
  ;

toplevel_decl
  : struct_decl
    {
      xdl->types = g_slist_append(xdl->types, $1);
    }
  | servlet_decl
    {
      xdl->servlets = g_slist_append(xdl->servlets, $1);
    }
  ;

/* structs */

struct_decl
  : "struct" IDENTIFIER "{" struct_members "}"
    {
      if (xdl_typedef_find(xdl, cur_servlet, $2))
      {
        printf("Redefining already defined type %s\n", $2);
        exit(1);
      }
      $$ = xdl_typedef_new_struct(xdl, cur_servlet, $2);
      $$->struct_members = $4;
    }
  ;

struct_members
  : struct_member
    {
      $$ = g_slist_append(NULL, $1);
    }
  | struct_members struct_member
    {
      $$ = g_slist_append($1, $2);
    }
  ;

struct_member
  : type IDENTIFIER ";"
    {
      $$ = g_new0(typeof(*$$), 1);
      $$->type = $1;
      $$->name = $2;
    }
  ;

/* servlets */

servlet_decl
  : "servlet" IDENTIFIER
    {
      cur_servlet = g_new0(xdl_servlet, 1);
      cur_servlet->name = g_strdup($2);
    }
    "{" servlet_body_decls "}"
    {
      $$ = cur_servlet;
      cur_servlet = NULL;
    }
  ;

servlet_body_decls
  : servlet_body_decl
  | servlet_body_decls servlet_body_decl
  ;

servlet_body_decl
  : struct_decl
    {
      cur_servlet->types = g_slist_append(cur_servlet->types, $1);
    }
  | method_decl INLINE_CODE
    {
      cur_servlet->methods = g_slist_append(cur_servlet->methods, $1);
      $1->stub_impl = $2;
    }
  | method_decl ";"
    {
      cur_servlet->methods = g_slist_append(cur_servlet->methods, $1);
    }
  | "__init__" INLINE_CODE
    {
      cur_servlet->stub_init = $2;
    }
  | "__fini__" INLINE_CODE
    {
      cur_servlet->stub_fini = $2;
    }
  | "__attrs__" INLINE_CODE
    {
      cur_servlet->stub_attrs = $2;
    }
  ;

/* type use */

type
  : "array" "<" type ">"
    {
      $$ = xdl_typedef_new_array(xdl, cur_servlet, $3);
    }
  | IDENTIFIER
    {
      $$ = xdl_typedef_find(xdl, cur_servlet, $1);
      if ($$ == NULL)
      {
        printf("Undefined type %s\n", $1);
        exit(1);
      }
    }
  ;

/* methods */

method_decl
  : type IDENTIFIER "(" opt_params ")"
    {
      $$ = g_new0(typeof(*$$), 1);
      $$->name = $2;
      $$->return_type = $1;
      $$->params = $4;
    }
  ;

opt_params
  :
    {
      $$ = NULL;
    } 
  | params
  ;

params
  : param
    {
      $$ = g_slist_append(NULL, $1);
    }
  | params "," param
    {
      $$ = g_slist_append($1, $3);
    }
  ;

param
  : type IDENTIFIER
    {
      $$ = g_new0(typeof(*$$), 1);
      $$->type = $1;
      $$->name = $2;
    }
  ;

%%

static void yyerror(YYLTYPE *locp, xdl_model *xdl, const char *msg)
{
  printf("Syntax error at [line %d, col %d]: %s.\n", locp->first_line, locp->first_column, msg);
  exit(1);
}

extern FILE* yyin;
extern int yylex_destroy(void);

int xdl_load(xdl_model *xdl, const char* path)
{
  yylex_destroy();

  yyin = fopen(path, "r");
  if (yyin == NULL)
  {
    printf("Couldn't open source file: %s.\n", path);
    return -1;
  }
  yyparse(xdl);
  fclose(yyin);

  yylex_destroy();
  return 0;
}
