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
 *
 * Author: David Lee <live4thee@gmail.com> (2007-12-27)
 * Small cleanups by Ondrej Jirman <ondrej.jirman@zonio.net>.
 */

#include <string.h>
#include "xr-value-utils.h"

/* xr_value_build */

#define xr_value_check_type(v, type, ch, rc) \
  do { \
    if (v == NULL) { \
      xr_debug(XR_DEBUG_VALUE, "NULL value corresponds to specifier: %c", ch); \
      return rc; \
    } \
    if (xr_value_get_type(v) != (type)) { \
      xr_debug(XR_DEBUG_VALUE, "Invalid type (%d) for specifier: %c", type, ch); \
      return rc; \
    } \
  } while (0)

static xr_value* xr_value_fmt_build_value(const char** const fmt, va_list* args, const char** const tail);

static xr_value* xr_value_fmt_build_struct(xr_value* value, const char** const fmt, va_list* args, const char** const tail)
{
  char* key = NULL;
  xr_value* val = NULL;

  while (**fmt != '*' && **fmt != '}' && **fmt != '\0')
  {
    if (**fmt != 's')
    {
      xr_debug(XR_DEBUG_VALUE, "Format string does not has a key for member");
      return NULL;
    }

    key = va_arg(*args, char *);
    if (key == NULL)
    {
      xr_debug(XR_DEBUG_VALUE, "NULL pointer encountered for a member name");
      return NULL;
    }

    (*fmt)++;

    if (**fmt != ':')
    {
      xr_debug(XR_DEBUG_VALUE, "Format string requires a ':' aftert a member name");
      return NULL;
    }

    (*fmt)++;
    val = xr_value_fmt_build_value(fmt, args, tail);
    if (val)
      xr_value_struct_set_member(value, key, val);
    else
      return NULL;

    if (**fmt == ',')
      (*fmt)++;
  }

  if (**fmt == '*')
  {
    (*fmt)++;

    if (**fmt != '}')
    {
      xr_debug(XR_DEBUG_VALUE, "Specifier '*' in struct is only for tagging the end");
      return NULL;
    }

    (*fmt)++;
  }
  else
  {
    xr_debug(XR_DEBUG_VALUE, "You must specify '*' as the last member of a struct!");
    return NULL;
  }

  return value;
}

static xr_value* xr_value_fmt_build_array(xr_value* value, const char** const fmt, va_list* args, const char** const tail)
{
  xr_value* val = NULL;

  while (**fmt != ')')
  {
    if (**fmt == '\0')
    {
      xr_debug(XR_DEBUG_VALUE, "You must specify ')' as the end of an array!");
      return NULL;
    }

    val = xr_value_fmt_build_value(fmt, args, tail);
    if (val)
      xr_value_array_append(value, val);
    else
      return NULL;
  }

  (*fmt)++;

  return value;
}

static xr_value* xr_value_fmt_build_value(const char** const fmt, va_list* args, const char** const tail)
{
  xr_value* value;
  xr_value* tmp;
  char ch;

  ch = *(*fmt)++;

  switch (ch)
  {
     case 'i':
       value = xr_value_int_new(va_arg(*args, int));
       break;

     case 'b':
       value = xr_value_bool_new(va_arg(*args, int));
       break;

     case 'd':
       value = xr_value_double_new(va_arg(*args, double));
       break;

     case 's':
       value = xr_value_string_new(va_arg(*args, char *));
       break;

     case 'S':
       value = va_arg(*args, xr_value*);
       xr_value_check_type(value, XRV_STRUCT, 'S', NULL);
       xr_value_ref(value);
       break;

     case 'A':
       value = va_arg(*args, xr_value*);
       xr_value_check_type(value, XRV_ARRAY, 'A', NULL);
       xr_value_ref(value);
       break;

     case '{':
       value = xr_value_struct_new();
       tmp = xr_value_fmt_build_struct(value, fmt, args, tail);
       if (!tmp)
         goto error;
       break;

     case '(':
       value = xr_value_array_new();
       tmp = xr_value_fmt_build_array(value, fmt, args, tail);
       if (!tmp)
         goto error;
       break;

     default:
       xr_debug(XR_DEBUG_VALUE, "Unexpected token '%c' in format string", ch);
       return NULL;
  }

  *tail = *fmt;
  return value;

error:
  xr_value_unref(value);
  return NULL;
}

static xr_value* xr_value_build_va(const char* fmt, va_list args)
{
  const char* ch = NULL;
  xr_value* value = NULL;
  va_list args_copy;

  g_return_val_if_fail(fmt != NULL && fmt[0] != '\0', NULL);

  va_copy(args_copy, args);
  value = xr_value_fmt_build_value(&fmt, &args_copy, &ch);
  if (!ch || *ch != '\0')
  {
    if (ch)
      xr_debug(XR_DEBUG_VALUE, "Junk specifier: '%s'", ch);

    xr_value_unref(value);
    return NULL;
  }

  return value;
}

xr_value* xr_value_build(const char* fmt, ...)
{
  xr_value* value = NULL;
  va_list ap;

  g_return_val_if_fail(fmt != NULL && fmt[0] != '\0', NULL);

  va_start(ap, fmt);
  value = xr_value_build_va(fmt, ap);
  va_end(ap);

  return value;
}

/* parser */

static gboolean xr_value_fmt_parse_value(xr_value* value, const char** const fmt, va_list* args, const char** const tail);

static gboolean xr_value_fmt_parse_struct(xr_value* value, const char** fmt, va_list* args, const char** tail)
{
  const char* name = NULL;
  xr_value* memb = NULL;

  xr_value_check_type(value, XRV_STRUCT, '{', FALSE);
  while (**fmt != '*' && **fmt != '}' && **fmt != '\0')
  {
    if (**fmt != 's')
    {
      xr_debug(XR_DEBUG_VALUE, "Format string does not has a key for member");
      return FALSE;
    }

    name = va_arg(*args, char *);
    if (name == NULL)
    {
      xr_debug(XR_DEBUG_VALUE, "NULL pointer encountered for a member name");
      return FALSE;
    }

    (*fmt)++;

    if (**fmt != ':')
    {
      xr_debug(XR_DEBUG_VALUE, "Format string requires a ':' aftert a member name");
      return FALSE;
    }

    (*fmt)++;

    memb = xr_value_get_member(value, name);
    if (memb == NULL)
    {
      xr_debug(XR_DEBUG_VALUE, "Member '%s' not found in the struct", name);
      return FALSE;
    }

    if (!xr_value_fmt_parse_value(memb, fmt, args, tail))
      return FALSE;

    if (**fmt == ',')
      (*fmt)++;
  }

  if (**fmt == '*')
  {
    (*fmt)++;

    if (**fmt != '}')
    {
      xr_debug(XR_DEBUG_VALUE, "Specifier '*' in struct is only for tagging the end");
      return FALSE;
    }

    (*fmt)++;
  }
  else
  {
    xr_debug(XR_DEBUG_VALUE, "You must specify '*' as the last member of a struct!");
    return FALSE;
  }

  return TRUE;
}

static gboolean xr_value_fmt_parse_array(xr_value* value, const char** fmt, va_list* args, const char** tail)
{
  GSList *array = NULL;
  guint length = 0;
  guint index;

  xr_value_check_type(value, XRV_ARRAY, '(', FALSE);
  array = xr_value_get_items(value);
  length = g_slist_length(array);

  for (index = 0; index < length; index++)
  {
    xr_value *item = NULL;

    if (**fmt == '*')
    {
      (*fmt)++;
      break;
    }

    if (**fmt == '\0')
    {
      xr_debug(XR_DEBUG_VALUE, "You must specify ')' as the end of an array!");
      return FALSE;
    }

    if (**fmt == ')')
    {
      xr_debug(XR_DEBUG_VALUE, "Too many items in array (consider '*' specifier?).");
      return FALSE;
    }

    item = (xr_value*)g_slist_nth_data(array, index);
    if (!xr_value_fmt_parse_value(item, fmt, args, tail))
      return FALSE;
  }

  if (**fmt != ')')
  {
    xr_debug(XR_DEBUG_VALUE, "Not enough items in array!");
    return FALSE;
  }

  (*fmt)++;

  return TRUE;
}

static gboolean xr_value_fmt_parse_value(xr_value* value, const char** const fmt, va_list* args, const char** const tail)
{
  char ch;

  ch = *(*fmt)++;

  switch (ch)
  {
     case 'i':
     {
       int *nval = va_arg(*args, int *);

       if (nval == NULL)
         return FALSE;
       if (!xr_value_to_int(value, nval))
         return FALSE;
     }
       break;

     case 'b':
     {
       int *nval = va_arg(*args, int *);

       if (nval == NULL)
         return FALSE;
       if (!xr_value_to_bool(value, nval))
         return FALSE;
     }
       break;

     case 'd':
     {
       double *nval = va_arg(*args, double *);

       if (nval == NULL)
         return FALSE;
       if (!xr_value_to_double(value, nval))
         return FALSE;
     }
       break;

     case 's':
     {
       char **nval = va_arg(*args, char **);

       if (nval == NULL)
         return FALSE;
       if (!xr_value_to_string(value, nval))
         return FALSE;
     }
       break;

     case 'S':
     {
       xr_value **val = va_arg(*args, xr_value **);

       if (val == NULL)
         return FALSE;
       xr_value_check_type(value, XRV_STRUCT, 'S', FALSE);
       *val = value;
       xr_value_ref(value);
     }
       break;

     case 'A':
     {
       xr_value **val = va_arg(*args, xr_value **);

       if (val == NULL)
         return FALSE;
       xr_value_check_type(value, XRV_ARRAY, 'A', FALSE);
       *val = value;
       xr_value_ref(value);
     }
       break;

     case '{':
       if (!xr_value_fmt_parse_struct(value, fmt, args, tail))
         return FALSE;
       break;

     case '(':
       if (!xr_value_fmt_parse_array(value, fmt, args, tail))
         return FALSE;
       break;

     default:
       xr_debug(XR_DEBUG_VALUE, "Unexpected token '%c' in format string", ch);
       return FALSE;
  }

  *tail = *fmt;

  return TRUE;
}

static gboolean xr_value_parse_va(xr_value* value, const char* fmt, va_list args)
{
  const char *ch = NULL;
  va_list args_copy;
  gboolean rc;

  g_return_val_if_fail(value != NULL, FALSE);
  g_return_val_if_fail(fmt != NULL && fmt[0] != '\0', FALSE);

  va_copy(args_copy, args);
  rc = xr_value_fmt_parse_value(value, &fmt, &args_copy, &ch);
  if (!ch || *ch != '\0' || rc == FALSE)
  {
    if (ch)
      xr_debug(XR_DEBUG_VALUE, "Junk specifier: '%s'", ch);
    return FALSE;
  }

  return rc;
}

gboolean xr_value_parse(xr_value* value, const char* fmt, ...)
{
  va_list args;
  gboolean retv;

  g_return_val_if_fail(value != NULL, FALSE);
  g_return_val_if_fail(fmt != NULL && fmt[0] != '\0', FALSE);

  va_start(args, fmt);
  retv = xr_value_parse_va(value, fmt, args);
  va_end(args);

  return retv;
}
