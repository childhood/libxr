#include "tests.h"
#include "xr-call.h"

#define REQUEST(method, params) \
  "<methodCall><methodName>" method "</methodName><params>" params "</params></methodCall>\n"
#define PARAM(value) \
  "<param>" value "</param>\n"
#define VALUE(type, value) \
  "<value><" G_STRINGIFY(type) ">" value "</" G_STRINGIFY(type) "></value>"
#define ARRAY(value) \
  "<value><array><data>" value "</data></array></value>"
#define MEMBER(name, value) \
  "<member><name>" G_STRINGIFY(name) "</name>" value "</member>"

/* tests */

static int constructCall()
{
  xr_call* call = xr_call_new("test.test");
  const char* method = xr_call_get_method(call);
  TEST_ASSERT(method != 0);
  int method_match = !strcmp(method, "test");
  TEST_ASSERT(method_match);
  xr_call_free(call);
  return TRUE;
}

static int constructCallNoMethod()
{
  xr_call* call = xr_call_new(0);
  const char* method = xr_call_get_method(call);
  TEST_ASSERT(method == 0);
  xr_call_free(call);
  return TRUE;
}

static int requestUnserialize1()
{
  xr_call* call = xr_call_new(0);
  int rs = xr_call_unserialize_request(call, "<->", -1);
  TEST_ASSERT(!rs);
  xr_call_free(call);
  return TRUE;
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
  TEST_ASSERT(!rs);
  xr_call_free(call);
  return TRUE;
}

static int requestUnserialize3()
{
  xr_call* call = xr_call_new(0);
  char* call_value =
  REQUEST("test.test",
    PARAM(VALUE(string, ""))
    PARAM(VALUE(string, "s2"))
  );

  int rs = xr_call_unserialize_request(call, call_value, -1);
  TEST_ASSERT(rs);

  const char* method = xr_call_get_method(call);
  TEST_ASSERT(method != 0);
  int method_match = !strcmp(method, "test");
  TEST_ASSERT(method_match);

  xr_value* val = xr_call_get_param(call, 0);
  TEST_ASSERT(val != 0);
  TEST_ASSERT(xr_value_get_type(val) == XRV_STRING);
  char* str_val = 0;
  rs = xr_value_to_string(val, &str_val);
  TEST_ASSERT(rs);
  TEST_ASSERT(str_val != 0);
  int param_match = !strcmp(str_val, "");
  TEST_ASSERT(param_match);
  g_free(str_val);

  val = xr_call_get_param(call, 1);
  TEST_ASSERT(val != 0);
  TEST_ASSERT(xr_value_get_type(val) == XRV_STRING);
  str_val = 0;
  rs = xr_value_to_string(val, &str_val);
  TEST_ASSERT(rs);
  TEST_ASSERT(str_val != 0);
  param_match = !strcmp(str_val, "s2");
  TEST_ASSERT(param_match);
  g_free(str_val);

  xr_call_free(call);
  return TRUE;
}

static int _assert_param_type(xr_call* call, int no, int type)
{
  xr_value* val = xr_call_get_param(call, no);
  TEST_ASSERT(val != 0);
  TEST_ASSERT(xr_value_get_type(val) == type);
  return xr_value_get_type(val) == type;
}

static int requestUnserialize4()
{
  xr_call* call = xr_call_new(0);
  char* call_value =
  REQUEST("test.test",
    PARAM(VALUE(int, "1"))
    PARAM(VALUE(int, "-1"))
    PARAM(VALUE(string, "some string"))
    PARAM(VALUE(boolean, "1"))
    PARAM(VALUE(double, "1.2323"))
    PARAM(VALUE(dateTime.iso8601, "2006-04-04"))
    PARAM(VALUE(base64, "bGlieHJpbmNsdWRlZGlyID0gJChpbmNsdWRlZGlyKS9saWJ4cgoKbGlieHJpbmNsdWRlX0hF\n"
                        "QURFUlMgPSBcCiAgeHItdmFsdWUuaCBcCiAgeHItY2FsbC5oIFwKICB4ci1jbGllbnQuaCBc\n"
                        "CiAgeHItc2VydmVyLmgK"))
    PARAM(ARRAY(
      VALUE(string, "f1")
      VALUE(string, "f2")
    ))
    PARAM(VALUE(struct,
      MEMBER(m1, VALUE(string, "s1"))
      MEMBER(m2, VALUE(string, ""))
    ))
  );
//  g_print(call_value);
  int rs = xr_call_unserialize_request(call, call_value, -1);
//  xr_call_dump(call, 0);
  TEST_ASSERT(rs);
  TEST_ASSERT(_assert_param_type(call, 0, XRV_INT));
  TEST_ASSERT(_assert_param_type(call, 1, XRV_INT));
  TEST_ASSERT(_assert_param_type(call, 2, XRV_STRING));
  TEST_ASSERT(_assert_param_type(call, 3, XRV_BOOLEAN));
  TEST_ASSERT(_assert_param_type(call, 4, XRV_DOUBLE));
  TEST_ASSERT(_assert_param_type(call, 5, XRV_TIME));
  TEST_ASSERT(_assert_param_type(call, 6, XRV_BLOB));
  TEST_ASSERT(_assert_param_type(call, 7, XRV_ARRAY));
  TEST_ASSERT(_assert_param_type(call, 8, XRV_STRUCT));
  xr_call_free(call);
  return TRUE;
}

/* testsuite */

int main()
{
  xr_debug_enabled = 0;
  int failed = FALSE;
  RUN_TEST(constructCall);
  RUN_TEST(constructCallNoMethod);
  RUN_TEST(requestUnserialize1);
  RUN_TEST(requestUnserialize2);
  RUN_TEST(requestUnserialize3);
  RUN_TEST(requestUnserialize4);
  return failed ? 1 : 0;
}
