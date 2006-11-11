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
typedef struct _xr_blob xr_blob;

struct _xr_blob
{
  char* buf;
  int len;
};

xr_blob* xr_blob_new(char* buf, int len);
void xr_blob_free(xr_blob* blob);

/* base types */
xr_value* xr_value_string_new(char* val);
xr_value* xr_value_int_new(int val);
xr_value* xr_value_bool_new(int val);
xr_value* xr_value_double_new(double val);
xr_value* xr_value_time_new(char* val);
xr_value* xr_value_blob_new(xr_blob* val);
int xr_value_to_int(xr_value* val, int* nval);
int xr_value_to_string(xr_value* val, char** nval);
int xr_value_to_bool(xr_value* val, int* nval);
int xr_value_to_double(xr_value* val, double* nval);
int xr_value_to_time(xr_value* val, char** nval);
int xr_value_to_blob(xr_value* val, xr_blob** nval);
int xr_value_get_type(xr_value* val);

xr_value* xr_value_array_new();
void xr_value_array_append(xr_value* arr, xr_value* val);
GSList* xr_value_get_items(xr_value* val);

xr_value* xr_value_struct_new();
void xr_value_struct_set_member(xr_value* str, char* name, xr_value* val);
xr_value* xr_value_get_member(xr_value* val, char* name);
GSList* xr_value_get_members(xr_value* val);
char* xr_value_get_member_name(xr_value* val);
xr_value* xr_value_get_member_value(xr_value* val);

void xr_value_free(xr_value* val);

int xr_value_is_error_retval(xr_value* v, int* errcode, char** errmsg);

#endif
