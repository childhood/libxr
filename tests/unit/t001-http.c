#include "tests.h"
#include "xr-http.h"

/* phony functions */

static char* _read_expect_buffer;
static int _read_expect_length;
static int _read_expect_position;

static void _read_expect(char* buf, int length)
{
  g_assert(buf != NULL);
  if (length < 0)
    length = strlen(buf);
  _read_expect_buffer = buf;
  _read_expect_length = length;
  _read_expect_position = 0;
}

int BIO_read(BIO *b, void *data, int len)
{
  int read_len = MIN(len, _read_expect_length-_read_expect_position);
  memcpy(data, _read_expect_buffer, read_len);
  _read_expect_position += read_len;
  return read_len;
}

int BIO_write(BIO *b, const void *data, int len)
{
  return 0;
}

/* tests */

static int receiveSimpleRequest()
{
  int retval = -1;
  xr_http* http = xr_http_new();

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

  if (rs < 0)
    goto err;

  if (strcmp(xr_http_get_resource(http), "/test"))
    goto err;

  gchar* header = xr_http_get_header(http, "My-Header:");
  if (!header || strcmp(header, "Hello"))
    goto err;

  header = xr_http_get_header(http, "my-header:");
  if (!header || strcmp(header, "Hello"))
    goto err;

  retval = 0;
 err:
  xr_http_free(http);
  return retval;
}

static int receiveSimpleRequestWithoutContentLength()
{
  int retval = -1;
  xr_http* http = xr_http_new();

  _read_expect(
    "GET /test HTTP/1.1\r\n"
    "\r\n",
    -1
  );

  char* buffer;
  int length;
  int rs = xr_http_receive(http, XR_HTTP_REQUEST, &buffer, &length);

  if (rs == 0)
    goto err;

  retval = 0;
 err:
  xr_http_free(http);
  return retval;
}

/* testsuite */

int main()
{
  RUN_TEST(receiveSimpleRequest);
  RUN_TEST(receiveSimpleRequestWithoutContentLength);
  return 0;
}
