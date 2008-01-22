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

/** @file xr-value-utils.h
 *
 * xr_value parser/builder.
 *
 * API which accepts a subset of xmlrpc-c format string. c.f.
 * http://xmlrpc-c.sourceforge.net/doc/libxmlrpc.html#formatstring
 *
 * Supported specifiers:
 *  i -- int
 *  b -- boolean
 *  d -- double
 *  s -- string
 *  A -- array
 *  S -- struct
 *  ( -- begin of an array
 *  ) -- end of an array
 *  { -- begin of a struct
 *  } -- end of a struct
 *  : -- key-value pair separator in a struct
 *  , -- separator for members of a struct
 *  * -- tagging the end of a struct, or ingore rest items of an array
 */

#ifndef __XR_VALUE_UTILS_H__
#define __XR_VALUE_UTILS_H__

#include <xr-value.h>

G_BEGIN_DECLS

/** Build a value node from format string.
 *
 * @param fmt The format string.
 *
 * @return A NULL pointer indicates a failure.
 */
xr_value* xr_value_build(const char* fmt, ...);

/** Parse a value node using format string.
 *
 * @param value The node to be parsed.
 * @param fmt The format string.
 *
 * @return TRUE on success, FALSE on failure.
 */
gboolean xr_value_parse(xr_value* value, const char* fmt, ...);

G_END_DECLS

#endif
