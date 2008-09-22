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

#ifndef __PARSER_LIB_H
#define __PARSER_LIB_H

#include <glib.h>
#include <string.h>

/* special tokens */
#define TK_EOF 0
#define TK_UNKNOWN (-1)

/* lexer helper macros */
#define RET(tok) \
  return token_new(s, tok, c-b)

#define EAT(next_state) \
  stream_advance(s, c-b); \
  b = c

#define DECLARE_LEXER() \
  static token *stream_peek_token(stream* s) \
  { \
    char *c, *b, *q; \
    if (s->length - s->index == 0) \
      return token_new(s, TK_EOF, 0); \
    c = b = s->buffer + s->index;

#define DECLARE_LEXER_END() \
    return NULL; \
  }

/* parser helper macros */
#define DECLARE_PARSER(name, fname, rtype) \
  extern void *name##Alloc(void *(*)(size_t)); \
  extern void name##Free(void*, void(*)(void *)); \
  extern void name(void*, int, token*, parser_context* ctx); \
  extern void name##Trace(FILE *TraceFILE, char *zTracePrompt); \
  extern const char* name##TokenName(int code); \
  rtype fname##_string(const char* str, GError** err) \
  { \
    return (rtype)__parse_string(str, err, name, name##Alloc, name##Free, stream_peek_token); \
  } \
  rtype fname##_file(const char* path, GError** err) \
  { \
    return (rtype)__parse_file(path, err, name, name##Alloc, name##Free, stream_peek_token); \
  }

#define HANDLE_SYNTAX_ERROR(TOKEN) \
  g_free(ctx->error); \
  if (TOKEN == NULL || TOKEN->type == TK_EOF) \
    ctx->error = g_strdup_printf("Unexpected EOF!\n"); \
  else \
    ctx->error = g_strdup_printf("Syntax error on line %d column %d: unexpected token %s\n", \
      TOKEN->sline+1, TOKEN->scol+1, TOKEN->type == TK_UNKNOWN ? "TK_UNKNOWN" : yyTokenName[TOKEN->type])

#define HANDLE_STACK_OVERFLOW() \
  g_free(ctx->error); \
  ctx->error = g_strdup_printf("Parser stack overflow!\n")

typedef struct stream stream;
struct stream {
  char* path;
  char* buffer;
  int index;
  int line;
  int col;
  int length;
};

typedef struct token token;
struct token 
{
  int type;
  char* text;       /* token text */
  int length;
  int sline;        /* token location (starts from 0) */
  int scol;
  int eline;
  int ecol;
};

typedef struct parser_context parser_context;
struct parser_context
{
  void* data;
  stream* stream;
  char* error;
};

/* parser functions types */
typedef void* (*parser_alloc)(void *(*)(size_t));
typedef void (*parser_free)(void*, void(*)(void *));
typedef void (*parser)(void*, int, token*, parser_context* ctx);
typedef token* (*lexer)(stream*);

stream* stream_new_from_string(const char* str);
void stream_advance(stream* s, int length);

token *token_new(stream* s, int type, int length);
void token_free(token * t);

void* __parse_string(const char* str, 
                     GError** err,
                     parser parser_cb, 
                     parser_alloc parser_alloc_cb, 
                     parser_free parser_free_cb,
                     lexer lexer_cb);
void* __parse_file(const char* path, 
                   GError** err,
                   parser parser_cb, 
                   parser_alloc parser_alloc_cb, 
                   parser_free parser_free_cb,
                   lexer lexer_cb);

#endif
