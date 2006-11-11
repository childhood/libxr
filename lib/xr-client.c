#include <stdio.h>
#include <memory.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <strings.h>

#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "xr-client.h"
//#define DEBUG

struct _xr_client_conn
{
  SSL_CTX* ctx;
  SSL* ssl;
  BIO* bio;
  char* resource;
  char* host;
  int secure;
  int is_open;

  int errcode;    /* this must be > 0 for errors */
  char* errmsg;   /* Non-NULL on error. */
};

static void _xr_client_set_error(xr_client_conn* conn, int code, char* msg)
{
  g_assert(conn != NULL);
  conn->errcode = code;
  g_free(conn->errmsg);
  conn->errmsg = g_strdup(msg);
}

void xr_client_init()
{
  SSL_library_init();
  ERR_load_crypto_strings();
  SSL_load_error_strings();
  ERR_load_SSL_strings();
}

xr_client_conn* xr_client_new()
{
  xr_client_conn* conn = g_new0(xr_client_conn, 1);
  conn->ctx = SSL_CTX_new(SSLv3_client_method());
  //XXX: setup certificates
  if (conn->ctx == NULL)
  {
    g_free(conn);
    return NULL;
  }
  return conn;
}

int xr_client_get_error_code(xr_client_conn* conn)
{
  g_assert(conn != NULL);
  return conn->errcode;
}

char* xr_client_get_error_message(xr_client_conn* conn)
{
  g_assert(conn != NULL);
  return conn->errmsg;
}

void xr_client_reset_error(xr_client_conn* conn)
{
  _xr_client_set_error(conn, 0, NULL);
}

int xr_client_open(xr_client_conn* conn, char* uri)
{
  g_assert(conn != NULL);
  g_assert(uri != NULL);
  g_assert(!conn->is_open);

  // parse URI format: http://host:8080/RES
  conn->secure = 1;
  conn->resource = "/CLIENT";
  conn->host = "127.0.0.1";
  if (conn->secure)
  {
    conn->bio = BIO_new_ssl_connect(conn->ctx);
    BIO_get_ssl(conn->bio, &conn->ssl);
    SSL_set_mode(conn->ssl, SSL_MODE_AUTO_RETRY);
    BIO_set_conn_hostname(conn->bio, uri);
  }
  else
  {
    conn->bio = BIO_new_connect(uri);
  }

#ifdef DEBUG
//  BIO_set_callback(conn->bio, BIO_debug_callback);
#endif

  if (BIO_do_connect(conn->bio) <= 0)
  {
    fprintf(stderr, "Error connecting to server\n");
    ERR_print_errors_fp(stderr);
    BIO_free_all(conn->bio);
    return -1;
  }
  
  int flag = 1;
  int sock = -1;
  BIO_get_fd(conn->bio, &sock);
  if (sock >= 0)
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
  else
    fprintf(stderr, "Error disabling Nagle.\n");

  if (conn->secure)
  {
    if (BIO_do_handshake(conn->bio) <= 0)
    {
      fprintf(stderr, "Error establishing SSL connection\n");
      ERR_print_errors_fp(stderr);
      BIO_free_all(conn->bio);
      return -1;
    }
  }
  conn->is_open = 1;
  return 0;
}

int xr_client_close(xr_client_conn* conn)
{
  g_assert(conn != NULL);
  g_assert(conn->is_open);

  if (conn->secure)
    BIO_ssl_shutdown(conn->bio);
  BIO_free_all(conn->bio);
  conn->ssl = NULL;
  conn->bio = NULL;
  conn->is_open = 0;
  return 0;
}

static int _xr_client_parse_headers(xr_client_conn* conn, char* buf, int len)
{
  int i, content_length = -1;
  GSList* headers = NULL, *it;
  char* line = buf;
  for (i = 0; i < len; i++)
  {
    if (buf[i] == '\r' && buf[i+1] == '\n')
    {
      if (line < buf + i)
        headers = g_slist_append(headers, g_strndup(line, buf + i - line));
      line = buf + i + 2;
    }
  }
  if (line < buf + len)
    headers = g_slist_append(headers, g_strndup(line, buf + len - line));

#define match_prefix(str) !strncasecmp(header, str, sizeof(str)-1)
  for (it=headers; it; it=it->next)
  {
    char* header = it->data;
    if (match_prefix("Content-Length:"))
    {
      content_length = atoi(g_strstrip(header + sizeof("Content-Length:")-1));
    }
    g_free(header);
  }

  return content_length;
}

static char* _find_eoh(char* buf, int len)
{
  int i;
  for (i = 0; i < len-3; i++)
  {
    if (buf[i] == '\r' && !memcmp(buf+i, "\r\n\r\n", 4))
      return buf+i;
  }
  return NULL;
}

int xr_client_call(xr_client_conn* conn, xr_call* call)
{
  g_assert(conn != NULL);
  g_assert(conn->is_open);
  g_assert(call != NULL);
  g_assert(conn->errcode == 0);

  char* request_buffer;
  int request_length;
  char* request_header;
  int request_header_length;

  // send request
  xr_call_serialize_request(call, &request_buffer, &request_length);
  request_header = g_strdup_printf(
    "POST %s HTTP/1.1\r\n"
    "Connection: keep-alive\r\n"
    "Host: %s\r\n"
    "Content-Length: %d\r\n\r\n",
    conn->resource,
    conn->host,
    request_length
  );
  request_header_length = strlen(request_header);
  if (BIO_write(conn->bio, request_header, request_header_length) != request_header_length)
  {
    g_free(request_header);
    xr_call_free_buffer(request_buffer);
    _xr_client_set_error(conn, -1, "");
    goto fatal_err;
  }
  if (BIO_write(conn->bio, request_buffer, request_length) != request_length)
  {
    g_free(request_header);
    xr_call_free_buffer(request_buffer);
    _xr_client_set_error(conn, -1, "");
    goto fatal_err;
  }
#ifdef DEBUG
  printf("%s", request_header);
  fwrite(request_buffer, request_length, 1, stdout);
  fflush(stdout);
#endif
  g_free(request_header);
  xr_call_free_buffer(request_buffer);

  // read response
#define READ_STEP 128
  char response_header[1025];
  int response_header_length = 0;
  char* response_buffer_preread;
  int response_length_preread;
  char* response_buffer;
  int response_length;

  while (1)
  {
    if (response_header_length + READ_STEP > sizeof(response_header)-1)
    {
      _xr_client_set_error(conn, -1, "Headers too long (limit is 1024 bytes)");
      goto fatal_err;
    }
    int bytes_read = BIO_read(conn->bio, response_header + response_header_length, READ_STEP);
    if (bytes_read <= 0)
    {
      _xr_client_set_error(conn, -1, "EOS in headers. Are we talking to HTTP server?");
      goto fatal_err;
    }
    char* eoh = _find_eoh(response_header + response_header_length, bytes_read); // search for block for \r\n\r\n (EOH)
    response_header_length += bytes_read;
    if (eoh) // end of headers in the current block
    {
      response_buffer_preread = eoh + 4;
      response_length_preread = response_header_length - (eoh - response_header + 4);
      response_header_length = eoh - response_header + 4;
      *(eoh+2) = '\0';
      break;
    }
  }

#ifdef DEBUG
  fwrite(response_header, response_header_length, 1, stdout);
  fflush(stdout);
#endif

  response_length = _xr_client_parse_headers(conn, response_header, response_header_length);
  if (response_length <= 0 || response_length > 1024*1024)
  {
    _xr_client_set_error(conn, -1, "Content length unknown or over limit (1MB).");
    goto fatal_err;
  }

  response_buffer = g_malloc(response_length);  
  memcpy(response_buffer, response_buffer_preread, response_length_preread);

  if (BIO_read(conn->bio, response_buffer, response_length - response_length_preread) 
      != response_length - response_length_preread)
  {
    g_free(response_buffer);
    _xr_client_set_error(conn, -1, "EOS in content. Server terminated connecton?");
    goto fatal_err;
  }

#ifdef DEBUG
  fwrite(response_buffer, response_length, 1, stdout);
  fflush(stdout);
#endif

  int retval = xr_call_unserialize_response(call, response_buffer, response_length);
  g_free(response_buffer);
  if (retval < 0)
  {
    _xr_client_set_error(conn, xr_call_get_error_code(call), xr_call_get_error_message(call));
    return 1;
  }

  xr_client_reset_error(conn);

  return 0;
 fatal_err:
  return -1;
}

int xr_client_call_ex(xr_client_conn* conn, xr_call* call, xr_demarchalizer_t dem, void** retval)
{
  if (xr_client_call(conn, call) == 0)
  {
    if (dem(xr_call_get_retval(call), retval) < 0)
      _xr_client_set_error(conn, 100, "getListList: Retval demarchalziation failed!");
  }
}

void xr_client_free(xr_client_conn* conn)
{
  g_assert(conn != NULL);
  if (conn->is_open)
    xr_client_close(conn);
  SSL_CTX_free(conn->ctx);
  g_free(conn);
}
