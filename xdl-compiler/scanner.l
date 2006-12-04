%{

#include "xdl.h"
#include "parser.h"

#define YY_DECL int yylex (YYSTYPE *yylval_param, YYLTYPE *yylloc_param, xdl_model *xdl)
#define uploc    { yylloc->first_column = yylloc->last_column + 1; yylloc->last_column += strlen (yytext); }

%}
 
%option yylineno
%option bison-bridge
%option bison-locations
%option noyywrap
%option nounput
%option prefix="xdl_" outfile="lex.yy.c"

%x IN_COMMENT
%x IN_DOC_COMMENT
%x IN_CODE

space                       [ \t\n]*
ident                       [[:alnum:]_]+
integer_literal             (0|[1-9][[:digit:]]*)
real_literal                [[:digit:]]+"."[[:digit:]]*
literal                     ({integer_literal}|{real_literal})

single_string_character     [^\"\\]
simple_escape_sequence      \\[\'\"\?\\abfnrtv]
string_character            ({single_string_character}|{simple_escape_sequence})
string_literal              \"{string_character}*\"

/* " */

%%

"/**"                       { uploc; BEGIN(IN_DOC_COMMENT); yymore (); }
<IN_DOC_COMMENT>"*/"        { uploc; BEGIN(INITIAL); yylval->str = g_strdup(yytext); return DOC_COMMENT; }
<IN_DOC_COMMENT>[^*\n]+     { uploc; yymore (); }
<IN_DOC_COMMENT>"*"         { uploc; yymore (); }
<IN_DOC_COMMENT>\n          { yylloc->first_line = yylloc->last_line = yylineno; yylloc->first_column = 1; yylloc->last_column = 0; yymore (); }

"/*"                        { uploc; BEGIN(IN_COMMENT); yymore (); }
<IN_COMMENT>"*/"            { uploc; BEGIN(INITIAL); yytext[strlen(yytext) - 2] = '\0'; }
<IN_COMMENT>[^*\n]+         { uploc; yymore (); }
<IN_COMMENT>"*"             { uploc; yymore (); }
<IN_COMMENT>\n              { yylloc->first_line = yylloc->last_line = yylineno; yylloc->first_column = 1; yylloc->last_column = 0; yymore (); }

"<%"                        { uploc; BEGIN(IN_CODE); yymore (); }
<IN_CODE>"%>"               { uploc; BEGIN(INITIAL); yylval->str = g_strstrip(g_strndup(yytext+2, strlen(yytext+2)-2)); return INLINE_CODE; }
<IN_CODE>[^%\n]+            { uploc; yymore (); }
<IN_CODE>"%"                { uploc; yymore (); }
<IN_CODE>\n                 { yylloc->first_line = yylloc->last_line = yylineno; yylloc->first_column = 1; yylloc->last_column = 0; yymore (); }

"//".*                      { uploc; }

"{"                         { uploc; return OPEN_BRACE; }
"}"                         { uploc; return CLOSE_BRACE; }
"("                         { uploc; return OPEN_RBRACE; }
")"                         { uploc; return CLOSE_RBRACE; }
"<"                         { uploc; return OPEN_SBRACE; }
">"                         { uploc; return CLOSE_SBRACE; }
";"                         { uploc; return SEMICOLON; }
","                         { uploc; return COMMA; }
struct                      { uploc; return STRUCT; }
array                       { uploc; return ARRAY; }
servlet                     { uploc; return SERVLET; }
namespace                   { uploc; return NAMESPACE; }
__init__                    { uploc; return KW_INIT; }
__fini__                    { uploc; return KW_FINI; }
__attrs__                   { uploc; return KW_ATTRS; }

{integer_literal}           { uploc; yylval->num = atoi(yytext); return INTEGER_LITERAL; }

{string_literal}            { uploc; yytext[strlen(yytext)-1] = '\0'; yylval->str = g_strcompress(yytext+1); return STRING_LITERAL; }
{ident}                     { uploc; yylval->str = g_strdup(yytext); return IDENTIFIER; }

[ \t]+                      { uploc; /* eat up whitespace */ }
[\n]+                       { yylloc->first_line = yylloc->last_line = yylineno; yylloc->first_column = 1; yylloc->last_column = 0; }

.                           { uploc; fprintf (stderr, "%d: syntax error: unexpected character '%s'\n", yylloc->first_line, yytext); }
