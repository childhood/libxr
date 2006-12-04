#include "tests.h"
#include "xr-http.h"

/* phony functions */

static char* _read_expect_buffer;
static int _read_expect_length;
static int _read_expect_position;

static void _read_expect(char* buf, int length)
{
  g_assert(buf != 0);
  if (length < 0)
    length = strlen(buf);
  _read_expect_buffer = buf;
  _read_expect_length = length;
  _read_expect_position = 0;
}

int BIO_read(BIO *b, void *data, int len)
{
  int read_len = MIN(len, _read_expect_length-_read_expect_position);
  memcpy(data, _read_expect_buffer+_read_expect_position, read_len);
  _read_expect_position += read_len;
  return read_len;
}

int BIO_write(BIO *b, const void *data, int len)
{
  return 0;
}

/* tests */

static int receiveRequestValid()
{
  int retval = -1;
  BIO* bio = (BIO*)1;
  xr_http* http = xr_http_new(bio);

  _read_expect(
    "GET /test HTTP/1.1\r\n"
    "Content-Length: 0\r\n"
    "My-Header: Hello\r\n"
    "\r\n",
    -1
  );

  char* buffer;
  int length;
  int rs = xr_http_receive(http, XR_HTTP_REQUEST, &buffer, &length);

  TEST_ASSERT(rs == 0)
  if (rs < 0)
    goto err;

  rs = strcmp(xr_http_get_resource(http), "/test");
  TEST_ASSERT(rs == 0)
  if (rs)
    goto err;

  gchar* header = xr_http_get_header(http, "My-Header:");
  TEST_ASSERT(header != 0)
  if (!header || strcmp(header, "Hello"))
    goto err;

  header = xr_http_get_header(http, "my-header:");
  TEST_ASSERT(header != 0)
  if (!header || strcmp(header, "Hello"))
    goto err;

  header = xr_http_get_header(http, "header:");
  TEST_ASSERT(header == 0)
  if (header)
    goto err;

  retval = 0;
 err:
  xr_http_free(http);
  return retval;
}

static int _expect_invalid_http(int message_type)
{
  int retval = -1;
  BIO* bio = (BIO*)1;
  xr_http* http = xr_http_new(bio);

  char* buffer = 0;
  int length;
  int rs = xr_http_receive(http, message_type, &buffer, &length);

  TEST_ASSERT(buffer == 0)
  TEST_ASSERT(rs < 0)
  if (rs == 0)
    goto err;

  retval = 0;
 err:
  xr_http_free(http);
  return retval;
}

static int receiveRequestWithoutContentLength()
{
  _read_expect(
    "GET /test HTTP/1.1\r\n"
    "\r\n",
    -1
  );
  return _expect_invalid_http(XR_HTTP_REQUEST);
}

static int receiveRequestInvalidStartLine()
{
  _read_expect(
    "GET /test sfgsdfg dsfg  HTTP/1.1\r\n"
    "Content-Length: 0\r\n"
    "\r\n",
    -1
  );
  return _expect_invalid_http(XR_HTTP_REQUEST);
}

static int receiveRequestUnterminatedHeader()
{
  _read_expect(
    "GET /test HTTP/1.1\r\n"
    "Content-Length: 0\r\n"
    "Some-Crap: cr...",
    -1
  );
  return _expect_invalid_http(XR_HTTP_REQUEST);
}

static int receiveRequestVeryLongHeader()
{
  _read_expect(
    "GET /test HTTP/1.1\r\n"
    "Content-Length: 0\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "Very-Long-Header: long long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\r\n"
    "\r\n",
    -1
  );
  return _expect_invalid_http(XR_HTTP_REQUEST);
}

#if 0
int xr_http_receive(xr_http* http, int message_type, gchar** buffer, gint* length);
int xr_http_send(xr_http* http, int message_type, gchar* buffer, gint length);
void xr_http_setup_request(xr_http* http, gchar* method, gchar* resource, gchar* host);
void xr_http_setup_response(xr_http* http, gint code);
#endif

/* testsuite */

int main()
{
  int failed = 0;
  RUN_TEST(receiveRequestValid);
  RUN_TEST(receiveRequestWithoutContentLength);
  RUN_TEST(receiveRequestInvalidStartLine);
  RUN_TEST(receiveRequestUnterminatedHeader);
  RUN_TEST(receiveRequestVeryLongHeader);
  return failed;
}
