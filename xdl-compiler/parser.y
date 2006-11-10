%{

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "xri.h"
#include "parser.h"

#define yyin xri_in
#define yylex_destroy xri_lex_destroy

int yylex (YYSTYPE *yylval_param, YYLTYPE *yylloc_param, struct parser_context *ctx);
static void yyerror(YYLTYPE *locp, struct parser_context *ctx, const char *msg);

%}

%defines
%locations
%pure-parser
%parse-param {struct parser_context *ctx}
%lex-param {struct parser_context *ctx}
%error-verbose
%name-prefix="xri_"

%union {
  int num;
  char* str;
  GSList* list;
  struct type* type;
  struct param* param;
  struct member* member;
  struct method* method;
}

%token OPEN_BRACE "{"
%token CLOSE_BRACE "}"
%token OPEN_RBRACE "("
%token CLOSE_RBRACE ")"
%token ARRAY "[]"
%token SEMICOLON ";"
%token COMMA ","
%token STRUCT "struct"

%token <str> DOC_COMMENT
%token <str> STRING_LITERAL
%token <str> IDENTIFIER
%token <num> INTEGER_LITERAL

%type <type> type
%type <type> type_decl
%type <member> struct_member
%type <list> struct_members
%type <list> params opt_params
%type <param> param
%type <method> method_decl

%start compilation_unit

%%

compilation_unit
  : opt_type_decls method_decls
  ;

opt_type_decls
  :
  | type_decls
  ;

type_decls
  : type_decl
  | type_decls type_decl
  ;

method_decls
  : method_decl
  | method_decls method_decl
  ;

type_decl
  : "struct" IDENTIFIER "{" struct_members "}" ";"
    {
      $$ = g_new(typeof(*$$), 1);
      $$->type = T_STRUCT;
      $$->name = $2;
      $$->members = $4;
      ctx->types = g_slist_append(ctx->types, $$);
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
      $$ = g_new(typeof(*$$), 1);
      $$->type = $1;
      $$->name = $2;
    }
  ;

type
  : IDENTIFIER "[]"
    {
      $$ = g_new(typeof(*$$), 1);
      $$->type = T_ARRAY;
      $$->name = $1;
      $$->elem_type = find_type(ctx, $1);
      if ($$->elem_type == NULL)
      {
        printf("Undefined type %s\n", $1);
        exit(1);
      }
    }
  | IDENTIFIER
    {
      $$ = find_type(ctx, $1);
      if ($$ == NULL)
      {
        printf("Undefined type %s\n", $1);
        exit(1);
      }
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
      $$ = g_new(typeof(*$$), 1);
      $$->type = $1;
      $$->name = $2;
    }
  ;

method_decl
  : type IDENTIFIER "(" opt_params ")" ";"
    {
      $$ = g_new(typeof(*$$), 1);
      $$->name = $2;
      $$->return_type = $1;
      $$->params = $4;
      ctx->methods = g_slist_append(ctx->methods, $$);
    }
  ;

%%

static void yyerror(YYLTYPE *locp, struct parser_context *ctx, const char *msg)
{
  printf("Syntax error at [line %d, col %d]: %s.\n", locp->first_line, locp->first_column, msg);
  exit(1);
}

extern FILE* yyin;
extern int yylex_destroy(void);

int xri_load(struct parser_context *ctx, const char* path)
{
  yylex_destroy();

  yyin = fopen(path, "r");
  if (yyin == NULL)
  {
    printf("Couldn't open source file: %s.\n", path);
    return -1;
  }
  yyparse(ctx);
  fclose(yyin);

  yylex_destroy();
  return 0;
}
