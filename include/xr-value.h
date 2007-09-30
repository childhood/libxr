/* 
 * Copyright 2006, 2007 Ondrej Jirman <ondrej.jirman@zonio.net>
 * 
 * This file is part of libxr.
 *
 * Libxr is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
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

/** @file xr-value.h
 *
 * Intermediate Representation of XML-RPC Values
 *
 * This code is used to manipulate tree of @ref xr_value nodes,
 * which represent all types defined in the XML-RPC specification.
 *
 * These types are: int, string, boolean, double, time, base64
 * (blob), struct and array.
 *
 * There is one additional node type to represent struct memeber
 * (@ref XRV_MEMBER).
 *
 * @note You should not need to wory about this code unless you use
 * @b any type in the @ref xdl.
 */

#ifndef __XR_VALUE_H__
#define __XR_VALUE_H__

#include "xr-lib.h"

/** Value Node Types.
 */
enum _xr_value_node_type {
  XRV_ARRAY,   /**< Array. */
  XRV_STRUCT,  /**< Struct. */
  XRV_MEMBER,  /**< Struct member. */
  XRV_INT,     /**< Integer. */
  XRV_STRING,  /**< String. */
  XRV_BOOLEAN, /**< Boolean. */
  XRV_DOUBLE,  /**< Double. */
  XRV_TIME,    /**< Time. */
  XRV_BLOB     /**< Blob (base64). */
};

/** Opaque data structure that holds information about particular node.
 */
typedef struct _xr_value xr_value;

/** Type used to pass blobs around in the user code.
 */
typedef struct _xr_blob xr_blob;

/** Type used to pass blobs around in the user code.
 */
struct _xr_blob
{
  char* buf;   /**< Buffer. */
  int len;     /**< Buffer length. */
  char refs;   /**< Number of references. */
};

/** Create new blob.
 *
 * @param buf Buffer with blob data. Ownership of the buffer is transferred.
 *   Buffer will be freed using g_free when @ref xr_blob_unref is called.
 * @param len Lenght of the data in the buffer. If negative, length is
 *   calculated automatically by strlen().
 *
 * @return New blob.
 *
 * @note This may be modified in the future to pass along file name
 *   where the blob data are stored.
 */
xr_blob* xr_blob_new(char* buf, int len);

/** Free blob.
 *
 * @param blob Blob.
 */
void xr_blob_unref(xr_blob* blob);

/** Take reference to the node.
 *
 * @param val Node to be refed.
 *
 * @return Same node.
 */
xr_value* xr_value_ref(xr_value* val);

/** Unref @ref xr_value node.
 *
 * Children will be unrefed only if last reference of @a val node is being removed.
 *
 * @param val Node to be unrefed.
 */
void xr_value_unref(xr_value* val);

/** Create new @ref xr_value node of type @ref XRV_STRING.
 *
 * @param val Value to be contained in the node.
 *
 * @return New @ref xr_value node.
 */
xr_value* xr_value_string_new(const char* val);

/** Create new @ref xr_value node of type @ref XRV_INT.
 *
 * @param val Value to be contained in the node.
 *
 * @return New @ref xr_value node.
 */
xr_value* xr_value_int_new(int val);

/** Create new @ref xr_value node of type @ref XRV_BOOLEAN.
 *
 * @param val Value to be contained in the node.
 *
 * @return New @ref xr_value node.
 */
xr_value* xr_value_bool_new(int val);

/** Create new @ref xr_value node of type @ref XRV_DOUBLE.
 *
 * @param val Value to be contained in the node.
 *
 * @return New @ref xr_value node.
 */
xr_value* xr_value_double_new(double val);

/** Create new @ref xr_value node of type @ref XRV_TIME.
 *
 * @param val Value to be contained in the node.
 *
 * @return New @ref xr_value node.
 */
xr_value* xr_value_time_new(const char* val);

/** Create new @ref xr_value node of type @ref XRV_BLOB.
 *
 * @param val Value to be contained in the node. Ownership of the val
 *   is transferred to the @ref xr_value. Val will be freed using
 *   xr_blob_unref when @ref xr_value_unref is called.
 *
 * @return New @ref xr_value node.
 */
xr_value* xr_value_blob_new(xr_blob* val);

/** Extract @ref xr_value of type @ref XRV_INT into the native language type.
 *
 * @param val Value node. May be NULL, see below.
 * @param nval Pointer to the variable where value should be extracted.
 *   This pointer must not be NULL.
 *
 * @return This function returns -1 if value can't be extracted, this
 *   may happen when val is NULL or val is not of expected node type.
 *   On error value pointed to by nval is not modified. 0 is returned
 *   on success.
 */
int xr_value_to_int(xr_value* val, int* nval);

/** Extract @ref xr_value of type @ref XRV_STRING into the native language type.
 *
 * @param val Value node. May be NULL, see below.
 * @param nval Pointer to the variable where value should be extracted.
 *   This pointer must not be NULL.
 *
 * @return This function returns -1 if value can't be extracted, this
 *   may happen when val is NULL or val is not of expected node type.
 *   On error value pointed to by nval is not modified. 0 is returned
 *   on success.
 *
 * @warning Caller must free returned string using g_free.
 */
int xr_value_to_string(xr_value* val, char** nval);

/** Extract @ref xr_value of type @ref XRV_BOOLEAN into the native language type.
 *
 * @param val Value node. May be NULL, see below.
 * @param nval Pointer to the variable where value should be extracted.
 *   This pointer must not be NULL.
 *
 * @return This function returns -1 if value can't be extracted, this
 *   may happen when val is NULL or val is not of expected node type.
 *   On error value pointed to by nval is not modified. 0 is returned
 *   on success.
 */
int xr_value_to_bool(xr_value* val, int* nval);

/** Extract @ref xr_value of type @ref XRV_DOUBLE into the native language type.
 *
 * @param val Value node. May be NULL, see below.
 * @param nval Pointer to the variable where value should be extracted.
 *   This pointer must not be NULL.
 *
 * @return This function returns -1 if value can't be extracted, this
 *   may happen when val is NULL or val is not of expected node type.
 *   On error value pointed to by nval is not modified. 0 is returned
 *   on success.
 */
int xr_value_to_double(xr_value* val, double* nval);

/** Extract @ref xr_value of type @ref XRV_TIME into the native language type.
 *
 * @param val Value node. May be NULL, see below.
 * @param nval Pointer to the variable where value should be extracted.
 *   This pointer must not be NULL.
 *
 * @return This function returns -1 if value can't be extracted, this
 *   may happen when val is NULL or val is not of expected node type.
 *   On error value pointed to by nval is not modified. 0 is returned
 *   on success.
 *
 * @warning Caller must free returned string using g_free.
 */
int xr_value_to_time(xr_value* val, char** nval);

/** Extract @ref xr_value of type @ref XRV_BLOB into the native language type.
 *
 * @param val Value node. May be NULL, see below.
 * @param nval Pointer to the variable where value should be extracted.
 *   This pointer must not be NULL.
 *
 * @return This function returns -1 if value can't be extracted, this
 *   may happen when val is NULL or val is not of expected node type.
 *   On error value pointed to by nval is not modified. 0 is returned
 *   on success.
 *
 * @warning Returned blob is still owned by the xr_value node and will be freed
 *   whenever xr_value_unref is called on the original node.
 */
int xr_value_to_blob(xr_value* val, xr_blob** nval);

/** Just a convenience interface to xr_value_ref.
 *
 * @param val Value node. May be NULL, see below.
 * @param nval Pointer to the variable where value should be extracted.
 *   This pointer must not be NULL.
 *
 * @return This function returns -1 if @a val is NULL, otherwise 0 is returned.
 */
int xr_value_to_value(xr_value* val, xr_value** nval);

/** Get type of @ref xr_value node.
 *
 * @param val Value node. Must not be NULL.
 *
 * @return This function returns node type (see @ref _xr_value_node_type).
 */
int xr_value_get_type(xr_value* val);

/** Create new array @ref xr_value node.
 *
 * @return New @ref xr_value node.
 */
xr_value* xr_value_array_new();

/** Add value to the array node. Value is placed at the end of the array
 * represented by the @ref xr_value node.
 *
 * @param arr Array node.
 * @param val Value node. Ownership of the val is transferred to arr.
 */
void xr_value_array_append(xr_value* arr, xr_value* val);

/** Get list of items from the array node.
 *
 * @param arr Array node.
 *
 * @return List of items (@ref xr_value nodes) in the array. Returned list and
 *   values are owned by the arr. Don't free them!
 */
GSList* xr_value_get_items(xr_value* arr);

/** Create new struct @ref xr_value node.
 *
 * @return New @ref xr_value node.
 */
xr_value* xr_value_struct_new();

/** Set struct member value.
 *
 * @param str Struct node.
 * @param name Member name.
 * @param val Member value node. Ownership of the val is transferred to str.
 */
void xr_value_struct_set_member(xr_value* str, const char* name, xr_value* val);

/** Get member value from the struct node.
 *
 * @param str Struct node.
 * @param name Member name.
 *
 * @return Membr value or NULL if struct does not have member
 *   with specified name. Returned value is owned by the str.
 *   Don't free it!
 */
xr_value* xr_value_get_member(xr_value* str, const char* name);

/** Get list of @ref XRV_MEMBER nodes from the struct node.
 *
 * @param str Struct node.
 *
 * @return List of members (@ref xr_value nodes) in the array. Returned list and
 *   values are owned by the str. Don't free them!
 */
GSList* xr_value_get_members(xr_value* str);

/** Get name of the struct member from the @ref XRV_MEMBER node.
 *
 * @param mem Member node.
 *
 * @return Name of the member. Owned by the mem node.
 */
const char* xr_value_get_member_name(xr_value* mem);

/** Get value of the struct member from the @ref XRV_MEMBER node.
 *
 * @param mem Member node.
 *
 * @return Value of the member. Owned by the mem node.
 */
xr_value* xr_value_get_member_value(xr_value* mem);

/** Check if given node is stadard XML-RPC error struct. And store
 * faultCode and faultString into errcode and errmsg.
 *
 * @param val Value node.
 * @param errcode Pointer to the variable where the faultCode should be stored.
 * @param errmsg Pointer to the variable where the faultString should be stored.
 *
 * @return Function returns 1 if node is XML-RPC error node, 0 Otherwise.
 *
 * @warning Caller must free returned errmsg using g_free.
 */
int xr_value_is_error_retval(xr_value* val, int* errcode, char** errmsg);

/** Debugging function that dumps node tree to the string.
 *
 * @param v Value node.
 * @param string GString to which value will be dumped.
 * @param indent Indent level, usually 0.
 */
void xr_value_dump(xr_value* v, GString* string, int indent);

#endif
