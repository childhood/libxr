#include <stdlib.h>
#include <string.h>
#include <regex.h>

#include "xr-http.h"

struct _xr_http
{
  // io
  BIO* bio;
  // request
  char* req_method;
  char* req_resource;
  char* req_host;
  // response
  int res_code;
  char* res_reason;
  // common
  gchar** headers;
  int content_length;
};

#define READ_STEP 128
//#define DEBUG

static int _xr_http_parse_status_line(xr_http* http, char* line)
{
  regex_t r;
  regmatch_t m[4];
  int rs;
  
  g_assert(http != NULL);
  g_assert(line != NULL);

  rs = regcomp(&r, "^HTTP/([0-9]+\\.[0-9]+) ([0-9]+) (.+)$", REG_EXTENDED);
  if (rs)
    return -1;
  rs = regexec(&r, line, 4, m, 0);
  regfree(&r);
  if (rs != 0)
    return -1;

  gchar* code = g_strndup(line+m[2].rm_so, m[2].rm_eo-m[2].rm_so);
  http->res_code = atoi(code);
  g_free(code);

  g_free(http->res_reason);
  http->res_reason = g_strndup(line+m[3].rm_so, m[3].rm_eo-m[3].rm_so);

  return 0;
}

static int _xr_http_parse_request_line(xr_http* http, char* line)
{
  regex_t r;
  regmatch_t m[4];
  int rs;
  
  g_assert(http != NULL);
  g_assert(line != NULL);

  rs = regcomp(&r, "^([A-Z]+) ([^ ]+) HTTP/([0-9]+\\.[0-9]+)$", REG_EXTENDED);
  if (rs)
    return -1;
  rs = regexec(&r, line, 4, m, 0);
  regfree(&r);
  if (rs != 0)
    return -1;

  g_free(http->req_method);
  http->req_method = g_strndup(line+m[1].rm_so, m[1].rm_eo-m[1].rm_so);

  g_free(http->req_resource);
  http->req_resource = g_strndup(line+m[2].rm_so, m[2].rm_eo-m[2].rm_so);

  return 0;
}

static int _xr_http_parse_headers(xr_http* http, int message_type, char* buffer)
{
  int headers_count, i, rs;

  g_assert(http != NULL);
  g_assert(buffer != NULL);

  g_strfreev(http->headers);
  http->headers = g_strsplit(buffer, "\r\n", 100);
  headers_count = g_strv_length(http->headers);

  if (headers_count < 1)
    return -1;

  if (message_type == XR_HTTP_REQUEST)
    rs = _xr_http_parse_request_line(http, http->headers[0]);
  else
    rs = _xr_http_parse_status_line(http, http->headers[0]);

  if (rs < 0)
    return -1;

  char* clen = xr_http_get_header(http, "Content-Length:");
  http->content_length = clen ? atoi(clen) : -1;

  if (http->content_length < 0 || http->content_length > 1024*1024*8)
    return -1;

  return 0;
}

static char* _find_eoh(char* buf, int len)
{
  int i;
  for (i = 0; i < len-3; i++)
    if (buf[i] == '\r' && !memcmp(buf+i, "\r\n\r\n", 4))
      return buf+i;
  return NULL;
}

xr_http* xr_http_new(BIO* bio)
{
  g_assert(bio != NULL);

  xr_http* http = g_new0(xr_http, 1);
  http->bio = bio;
  return http;
}

void xr_http_free(xr_http* http)
{
  g_strfreev(http->headers);
  g_free(http->req_resource);
  g_free(http->req_method);
  g_free(http->req_host);
  g_free(http->res_reason);
  memset(http, 0, sizeof(*http));
  g_free(http);
}

int xr_http_receive(xr_http* http, int message_type, gchar** buffer, gint* length)
{
  g_assert(http != NULL);
  g_assert(buffer != NULL);
  g_assert(length != NULL);

  char header[2049];
  unsigned int header_length = 0;
  char* buffer_preread;
  int length_preread;
  char* _buffer;
  int _length;

  while (1)
  {
    if (header_length + READ_STEP > sizeof(header)-1)
      return -1;
    int bytes_read = BIO_read(http->bio, header + header_length, READ_STEP);
    if (bytes_read <= 0)
      return -1;
    char* eoh = _find_eoh(header + header_length, bytes_read);
    header_length += bytes_read;
    if (eoh)
    {
      buffer_preread = eoh + 4;
      length_preread = header_length - (eoh - header + 4);
      header_length = eoh - header + 4;
      *(eoh+2) = '\0';
      break;
    }
  }

#ifdef DEBUG
  fwrite(header, header_length, 1, stdout);
  fflush(stdout);
#endif

  if (_xr_http_parse_headers(http, message_type, header) < 0)
    return -1;

  _length = http->content_length;
  _buffer = g_malloc(_length);  
  memcpy(_buffer, buffer_preread, length_preread);

  if (BIO_read(http->bio, _buffer + length_preread, _length - length_preread) 
      != _length - length_preread)
  {
    g_free(_buffer);
    return -1;
  }

  *buffer = _buffer;
  *length = _length;

#ifdef DEBUG
  fwrite(_buffer, _length, 1, stdout);
  fflush(stdout);
#endif

  return 0;
}

int xr_http_send(xr_http* http, int message_type, gchar* buffer, gint length)
{
  g_assert(http != NULL);
  g_assert(buffer != NULL);

  char* header;
  int header_length;
  int retval = -1;

  if (message_type == XR_HTTP_REQUEST)
  {
    header = g_strdup_printf(
      "%s %s HTTP/1.1\r\n"
      "Host: %s\r\n"
      "Connection: keep-alive\r\n"
      "Content-Length: %d\r\n"
      "\r\n",
      http->req_method,
      http->req_resource,
      http->req_host,
      length
    );
  }
  else
  {
    header = g_strdup_printf(
      "HTTP/1.1 %d %s\r\n"
      "Content-Type: text/xml\r\n"
      "Content-Length: %d\r\n\r\n",
      http->res_code,
      http->res_reason,
      length
    );
  }

  header_length = strlen(header);
  if (BIO_write(http->bio, header, header_length) != header_length)
    goto err;
  if (BIO_write(http->bio, buffer, length) != length)
    goto err;

#ifdef DEBUG
  printf("%s", header);
  fwrite(buffer, length, 1, stdout);
  fflush(stdout);
#endif

  retval = 0;
 err:
  g_free(header);
  return retval;
}

gchar* xr_http_get_header(xr_http* http, const gchar* name)
{
  int headers_count, i;

  g_assert(http != NULL);
  g_assert(http->headers != NULL);
  g_assert(name != NULL);

  headers_count = g_strv_length(http->headers);
  for (i=1; i<headers_count; i++)
  {
    if (http->headers[i] && strlen(http->headers[i]) > 2
        && !g_ascii_strncasecmp(http->headers[i], name, strlen(name)))
    {
      return g_strstrip(g_strdup(http->headers[i]+strlen(name)));
    }
  }
  return NULL;
}

void xr_http_setup_request(xr_http* http, gchar* method, gchar* resource, gchar* host)
{
  g_assert(http != NULL);
  g_assert(method != NULL);
  g_assert(resource != NULL);

  g_free(http->req_method);
  http->req_method = g_strdup(method);
  g_free(http->req_resource);
  http->req_resource = g_strdup(resource);
  g_free(http->req_host);
  http->req_host = g_strdup(host);
}

void xr_http_setup_response(xr_http* http, gint code)
{
  g_assert(http != NULL);
  http->res_code = code;
  switch (code)
  {
    case 200: http->res_reason = g_strdup("OK"); break;
    case 501: http->res_reason = g_strdup("Not Implemented"); break;
    case 500: http->res_reason = g_strdup("Internal Error"); break;
    default:
      g_assert_not_reached();
  }
}

char* xr_http_get_resource(xr_http* http)
{
  g_assert(http != NULL);
  return g_strdup(http->req_resource);
}
