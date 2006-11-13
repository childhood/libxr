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

#include <glib.h>

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
};

/** Create new blob.
 *
 * @param buf Buffer with blob data. Ownership of the buffer is transferred.
 *   Buffer will be freed using g_free when @ref xr_blob_free is called.
 * @param len Lenght of the data in the buffer.
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
void xr_blob_free(xr_blob* blob);

/** Create new @ref xr_value node of type @ref XRV_STRING.
 *
 * @param val Value to be contained in the node.
 *
 * @return New @ref xr_value node.
 */
xr_value* xr_value_string_new(char* val);

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
xr_value* xr_value_time_new(char* val);

/** Create new @ref xr_value node of type @ref XRV_BLOB.
 *
 * @param val Value to be contained in the node. Ownership of the val
 *   is transferred to the @ref xr_value. Val will be freed using
 *   xr_blob_free when @ref xr_value_free is called.
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
 * @warning Value is still owned by the xr_value node and will be freed
 *   whenever xr_value_free is called on the original node.
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
 * @warning Caller must free returned string using g_free.
 */
int xr_value_to_blob(xr_value* val, xr_blob** nval);

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
void xr_value_struct_set_member(xr_value* str, char* name, xr_value* val);

/** Get member value from the struct node.
 *
 * @param str Struct node.
 * @param name Member name.
 *
 * @return Membr value or NULL if struct does not have member
 *   with specified name. Returned value is owned by the str.
 *   Don't free it!
 */
xr_value* xr_value_get_member(xr_value* str, char* name);

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
char* xr_value_get_member_name(xr_value* mem);

/** Get value of the struct member from the @ref XRV_MEMBER node.
 *
 * @param mem Member node.
 *
 * @return Value of the member. Owned by the mem node.
 */
xr_value* xr_value_get_member_value(xr_value* mem);

/** Free @ref xr_value node and all its children.
 *
 * @param val Node to be freed.
 */
void xr_value_free(xr_value* val);

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

/** Debugging function that dumps node tree to the stdout.
 *
 * @param v Value node.
 * @param indent Indent level, usually 0.
 */
void xr_value_dump(xr_value* v, int indent);

#endif
