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

#include "parser-lib.h"

/* stream api */

stream* stream_new_from_string(const char* str)
{
  if (str == NULL)
    return NULL;
  stream* s = g_new0(stream, 1);
  s->buffer = g_strdup(str);
  s->length = strlen(str);
  return s;
}

void stream_advance(stream* s, int length)
{
  int i;

  for (i = 0; i < length && s->index < s->length; i++, s->index++)
  {
    s->col++;
    if (s->buffer[s->index] == '\n')
    {
      s->col = 0;
      s->line++;
    }
  }
}

/* tokenizer api */

token *token_new(stream* s, int type, int length)
{
  int i;
  token *t = g_new0(token, 1);

  t->type = type;
  t->text = g_strndup(s->buffer + s->index, length);
  t->length = length;
  t->sline = t->eline = s->line;
  t->scol = t->ecol = s->col;
  for (i = 0; i < length; i++)
  {
    t->ecol++;
    if (t->text[i] == '\n')
    {
      t->ecol = 0;
      t->eline++;
    }
  }
  stream_advance(s, length);
  return t;
}

void token_free(token * t)
{
  if (t == NULL)
    return;
  g_free(t->text);
  g_free(t);
}

/* parser api */

static int __parse(parser_context* ctx, 
            parser parser_cb, 
            parser_alloc parser_alloc_cb, 
            parser_free parser_free_cb,
            lexer lexer_cb)
{
  void* parser;
  token* t;
  stream* s = ctx->stream;
  int retval = -1;

  if (ctx == NULL || ctx->error != NULL || ctx->stream == NULL)
    return -1;

  parser = parser_alloc_cb((void *(*)(size_t))g_malloc);
  while ((t = lexer_cb(s)) != NULL)
  {
    if (t->type == TK_UNKNOWN)
    {
      ctx->error = g_strdup_printf("Unknown token '%s' at line %d char %d\n", t->text, t->sline, t->scol);
      token_free(t);
      goto err;
    }
    else if (t->type == TK_EOF)
    {
      token_free(t);
      parser_cb(parser, 0, NULL, ctx);
      break;
    }

    parser_cb(parser, t->type, t, ctx);
    if (ctx->error)
      goto err;
  }

  retval = 0;
 err:
  parser_free_cb(parser, g_free);
  return retval;
}

void* __parse_string(const char* str, 
                     GError** err,
                     parser parser_cb, 
                     parser_alloc parser_alloc_cb, 
                     parser_free parser_free_cb,
                     lexer lexer_cb)
{
  parser_context* ctx = g_new0(parser_context, 1);

  ctx->stream = stream_new_from_string(str);
  if (ctx->stream == NULL)
  {
    g_free(ctx);
    g_set_error(err, 0, 1, "Stream can't be created!");
    return NULL;
  }

  __parse(ctx, parser_cb, parser_alloc_cb, parser_free_cb, lexer_cb);
  if (ctx->error)
  {
    g_set_error(err, 0, 1, "%s", ctx->error);
    return NULL;
  }

  void* data = ctx->data;
  g_free(ctx);
  return data;
}

void* __parse_file(const char* path, 
                   GError** err,
                   parser parser_cb, 
                   parser_alloc parser_alloc_cb, 
                   parser_free parser_free_cb,
                   lexer lexer_cb)
{
  char* buffer;
  if (!g_file_get_contents(path, &buffer, NULL, err))
    return NULL;
  return __parse_string(buffer, err, parser_cb, parser_alloc_cb, parser_free_cb, lexer_cb);
}
