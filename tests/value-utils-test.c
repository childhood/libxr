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

#include <stdio.h>
#include "xr-value-utils.h"

static void xr_value_print(xr_value * value)
{
  GString *string;
  char *tmp;

  g_return_if_fail(value != NULL);

  string = g_string_sized_new(1024);
  xr_value_dump(value, string, 0);
  tmp = g_string_free(string, FALSE);
  g_print("%s\n", tmp);
  g_free(tmp);
}

int main(void)
{
  xr_value *value;
  char *s;
  int i;
  double d;
  xr_value *arr;
  xr_value *st;

  xr_debug_enabled = XR_DEBUG_VALUE;

  /* builder tests */
  g_print("Builder:\n");

  value = xr_value_build("s",
    "str"
  );
  xr_value_print(value);

  value = xr_value_build("(si)",
    "str", 100
  );
  xr_value_print(value);

  value = xr_value_build("{s:s,s:i,*}",
    "a", "val a",
    "b", 100
  );
  xr_value_print(value);

  value = xr_value_build("{s:{s:{s:d,*},*},s:i,s:s,s:(ii),*}",
    "retCode", 
      "temprature", 
        "evilmember", 1.0,
    "number", 2,
    "name", "this is good",
    "array", 3, 4
  );
  xr_value_print(value);

  /* invalid specs tests */
  value = xr_value_build("{s:s,s:i,*}{s:s,*}",
    "a", "val a",
    "b", 100
  );
  g_return_val_if_fail(value == NULL, 1);

  value = xr_value_build("ss",
    "a", "val a"
  );
  g_return_val_if_fail(value == NULL, 1);

  value = xr_value_build("{s}",
    "a", "val a"
  );
  g_return_val_if_fail(value == NULL, 1);

  value = xr_value_build("{s:}",
    "a", "val a"
  );
  g_return_val_if_fail(value == NULL, 1);

  value = xr_value_build("{:s}",
    "a", "val a"
  );
  g_return_val_if_fail(value == NULL, 1);

  value = xr_value_build("{s:s}",
    "a", "val a"
  );
  g_return_val_if_fail(value == NULL, 1);

  value = xr_value_build("{s:s,}",
    "a", "val a"
  );
  g_return_val_if_fail(value == NULL, 1);

  value = xr_value_build("{i:s,*}",
    "a", "val a"
  );
  g_return_val_if_fail(value == NULL, 1);

  value = xr_value_build("{s:s",
    "a", "val a"
  );
  g_return_val_if_fail(value == NULL, 1);

  value = xr_value_build("{s",
    "a", "val a"
  );
  g_return_val_if_fail(value == NULL, 1);

  value = xr_value_build("{}",
    "a", "val a"
  );
  g_return_val_if_fail(value == NULL, 1);

  value = xr_value_build("{,}",
    "a", "val a"
  );
  g_return_val_if_fail(value == NULL, 1);

  value = xr_value_build("}",
    "a", "val a"
  );
  g_return_val_if_fail(value == NULL, 1);

  value = xr_value_build("(",
    "a", "val a"
  );
  g_return_val_if_fail(value == NULL, 1);

  value = xr_value_build("(,)",
    "a", "val a"
  );
  g_return_val_if_fail(value == NULL, 1);

  /* parser tests */
  g_print("Parser:\n");

  value = xr_value_string_new("this is a test");
  g_return_val_if_fail(xr_value_parse(value, "s", &s), 1);
  g_print("str: %s\n", s);

  value = xr_value_int_new(100);
  g_return_val_if_fail(xr_value_parse(value, "i", &i), 1);
  g_print("int: %d\n", i);

  value = xr_value_bool_new(1);
  g_return_val_if_fail(xr_value_parse(value, "b", &i), 1);
  g_print("bool: %d\n", i);

  value = xr_value_double_new(3.14);
  g_return_val_if_fail(xr_value_parse(value, "d", &d), 1);
  g_print("double: %g\n", d);

  value = xr_value_array_new();
  xr_value_array_append(value, xr_value_string_new("element1"));
  xr_value_array_append(value, xr_value_string_new("element2"));
  g_return_val_if_fail(xr_value_parse(value, "A", &arr), 1);
  xr_value_print(arr);

  value = xr_value_struct_new();
  xr_value_struct_set_member(value, "member1", xr_value_string_new("element"));
  xr_value_struct_set_member(value, "member2", xr_value_string_new("element"));
  g_return_val_if_fail(xr_value_parse(value, "S", &st), 1);
  xr_value_print(st);

  value = xr_value_struct_new();
  xr_value_struct_set_member(value, "member", xr_value_string_new("element"));
  xr_value_struct_set_member(value, "retcode", xr_value_int_new(200));
  g_return_val_if_fail(xr_value_parse(value, "{s:s,s:i,*}", "member", &s, "retcode", &i), 1);
  g_print("member:  %s\n", s);
  g_print("retcode: %d\n", i);

  return 0;

  g_print("\n" "------------------------------------------,\n" "| Test building and parsing complex value |\n" "`----------------------------------------/\n\n");

  {
    xr_value *value = xr_value_build("{s:{s:{s:d,*},*},s:i,s:s,s:(ii),*}",
                                     "retCode", "temprature", "evilmember", 2.5,
                                     "number", 5,
                                     "name", "this is good",
                                     "array", 3, 4);
    double evilmember = 0.0;
    char *name = NULL;
    int array[] = { 0, 0 };

    xr_value_print(value);
    g_print("\n");

    g_return_val_if_fail(xr_value_parse(value, "{s:{s:{s:d,*},*},s:s,s:(ii),*}", "retCode", "temprature", "evilmember", &evilmember, "name", &name, "array", &array[0], &array[1]), 1);
    g_print("evilmember: %g\n", evilmember);
    g_print("name: %s\n", name);
    g_print("array: [ %d, %d ]\n", array[0], array[1]);

    xr_value_unref(value);
    g_free(name);
  }

  return 0;
}
