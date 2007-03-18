#include "tests.h"
#include "xr-call.h"

#define REQUEST(method, params) \
  "<methodCall><methodName>" method "</methodName><params>" params "</params></methodCall>"
#define PARAM(value) \
  "<param>" value "</param>"
#define VALUE(type, value) \
  "<value><" G_STRINGIFY(type) ">" value "</" G_STRINGIFY(type) "></value>"
#define MEMBER(name, value) \
  "<member><name>" G_STRINGIFY(name) "</name>" value "</member>"

/* tests */

static int constructCall()
{
  xr_call* call = xr_call_new("test");
  char* method = xr_call_get_method(call);
  TEST_ASSERT(method != 0);
  int method_match = !strcmp(method, "test");
  TEST_ASSERT(method_match);
  return 0;
}

static int constructCallNoMethod()
{
  xr_call* call = xr_call_new(0);
  char* method = xr_call_get_method(call);
  TEST_ASSERT(method == 0);
  return 0;
}

static int requestUnserialize1()
{
  xr_call* call = xr_call_new(0);
  int rs = xr_call_unserialize_request(call, "<->", -1);
  TEST_ASSERT(rs < 0);
  return 0;
}

static int requestUnserialize2()
{
  xr_call* call = xr_call_new(0);
  char* call_value =
  REQUEST("",
    PARAM(VALUE(string, "s1"))
    PARAM(VALUE(string, "s2"))
  );
  int rs = xr_call_unserialize_request(call, call_value, -1);
  TEST_ASSERT(rs < 0);
  return 0;
}

static int requestUnserialize3()
{
  xr_call* call = xr_call_new(0);
  char* call_value =
  REQUEST("test",
    PARAM(VALUE(string, ""))
    PARAM(VALUE(string, "s2"))
  );

  int rs = xr_call_unserialize_request(call, call_value, -1);
  TEST_ASSERT(rs == 0);

  char* method = xr_call_get_method(call);
  TEST_ASSERT(method != 0);
  int method_match = !strcmp(method, "test");
  TEST_ASSERT(method_match);

  xr_value* val = xr_call_get_param(call, 0);
  TEST_ASSERT(val != 0);
  TEST_ASSERT(xr_value_get_type(val) == XRV_STRING);
  char* str_val = 0;
  rs = xr_value_to_string(val, &str_val);
  TEST_ASSERT(rs == 0);
  TEST_ASSERT(str_val != 0);
  int param_match = !strcmp(str_val, "");
  TEST_ASSERT(param_match);

  val = xr_call_get_param(call, 1);
  TEST_ASSERT(val != 0);
  TEST_ASSERT(xr_value_get_type(val) == XRV_STRING);
  str_val = 0;
  rs = xr_value_to_string(val, &str_val);
  TEST_ASSERT(rs == 0);
  TEST_ASSERT(str_val != 0);
  param_match = !strcmp(str_val, "s2");
  TEST_ASSERT(param_match);

  return 0;
}

static int _assert_param_type(xr_call* call, int no, int type)
{
  xr_value* val = xr_call_get_param(call, no);
  TEST_ASSERT(val != 0);
  TEST_ASSERT(xr_value_get_type(val) == type);
  return 0;
}

static int requestUnserialize4()
{
  xr_call* call = xr_call_new(0);
  char* call_value =
  REQUEST("test",
    PARAM(VALUE(int, "1"))
    PARAM(VALUE(int, "-1"))
    PARAM(VALUE(string, "some string"))
    PARAM(VALUE(boolean, "true"))
    PARAM(VALUE(double, "1.2323"))
    PARAM(VALUE(time, "2006-04-04"))
    PARAM(VALUE(base64, "bGlieHJpbmNsdWRlZGlyID0gJChpbmNsdWRlZGlyKS9saWJ4cgoKbGlieHJpbmNsdWRlX0hF\n"
                        "QURFUlMgPSBcCiAgeHItdmFsdWUuaCBcCiAgeHItY2FsbC5oIFwKICB4ci1jbGllbnQuaCBc\n"
                        "CiAgeHItc2VydmVyLmgK"))
    PARAM(VALUE(array,
      VALUE(string, "f1")
      VALUE(string, "f2")
    ))
    PARAM(VALUE(struct,
      MEMBER(m1, VALUE(string, "s1"))
      MEMBER(m2, VALUE(string, ""))
    ))
  );
  int rs = xr_call_unserialize_request(call, call_value, -1);
//  xr_call_dump(call, 0);
  TEST_ASSERT(rs == 0);
  TEST_ASSERT(_assert_param_type(call, 0, XRV_INT) == 0);
  TEST_ASSERT(_assert_param_type(call, 1, XRV_INT) == 0);
  TEST_ASSERT(_assert_param_type(call, 2, XRV_STRING) == 0);
  TEST_ASSERT(_assert_param_type(call, 3, XRV_BOOLEAN) == 0);
  TEST_ASSERT(_assert_param_type(call, 4, XRV_DOUBLE) == 0);
  TEST_ASSERT(_assert_param_type(call, 5, XRV_TIME) == 0);
  TEST_ASSERT(_assert_param_type(call, 6, XRV_BLOB) == 0);
  TEST_ASSERT(_assert_param_type(call, 7, XRV_ARRAY) == 0);
  TEST_ASSERT(_assert_param_type(call, 8, XRV_STRUCT) == 0);
  return 0;
}

/* testsuite */

int main()
{
  xr_debug_enabled = 0;
  int failed = 0;
  RUN_TEST(constructCall);
  RUN_TEST(constructCallNoMethod);
  RUN_TEST(requestUnserialize1);
  RUN_TEST(requestUnserialize2);
  RUN_TEST(requestUnserialize3);
  RUN_TEST(requestUnserialize4);
  return failed;
}
