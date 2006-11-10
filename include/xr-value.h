#ifndef __XR_VALUE_H__
#define __XR_VALUE_H__

#include <glib.h>

/** @file xmlrpc_value Header
 */

enum {
  XRV_ARRAY,   /**< Array type. */
  XRV_STRUCT,  /**< Struct type. */
  XRV_MEMBER,  /**< Struct type. */
  XRV_INT,     /**< Integer type. */
  XRV_STRING,  /**< String type. */
  XRV_BOOLEAN, /**< Boolean type. */
  XRV_DOUBLE,  /**< Double type. */
  XRV_TIME,    /**< Time type. */
  XRV_BLOB     /**< Blob type. */
};

typedef struct _xr_value xr_value;
typedef struct _xr_call xr_call;

struct _xr_value
{
  int type;                      /**< Type of the value. */

  // values
  char* str_val;
  int int_val;
  double dbl_val;
  int buf_len;

  // array
  GSList* children;       /**< Members or array items list. */

  // struct member fields
  char* member_name;             /**< Struct member name. */
  xr_value* member_value; /**< Struct member value. */
};

xr_value* xr_value_struct_new();
xr_value* xr_value_array_new();
xr_value* xr_value_string_new(char* val);
xr_value* xr_value_int_new(int val);
xr_value* xr_value_bool_new(int val);
xr_value* xr_value_double_new(double val);
xr_value* xr_value_time_new(char* val);
xr_value* xr_value_blob_new(char* val, int len);
void xr_value_struct_set_member(xr_value* str, char* name, xr_value* val);
void xr_value_array_append(xr_value* arr, xr_value* val);
void xr_value_free(xr_value* val);

#endif
