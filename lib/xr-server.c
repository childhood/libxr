#include <stdio.h>
#include <memory.h>
#include <errno.h>
#include <regex.h>
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

#include "xr-server.h"
#define DEBUG

/* server */

struct _xr_servlet
{
  void* priv;
  BIO* bio;
  int running;
  char* resource;
  xr_servlet_def* def;
};

struct _xr_server
{
  SSL_CTX* ctx;
  BIO* bio_in;
  BIO* bio_ssl;
  GThreadPool* pool;
  int secure;
  int running;
  GSList* servlet_types;
};

void* xr_servlet_get_priv(xr_servlet* servlet)
{
  return servlet->priv;
}

void xr_servlet_return_error(xr_servlet* servlet, int code, char* msg)
{
}

char* _parse_http_request(char* header)
{
  regex_t r;
  regmatch_t m[4];
  int rs = regcomp(&r, "^([^ ]+)[ ]+([^ ]+)[ ]+(.+)$", REG_EXTENDED);
  if (rs)
    return NULL;
  rs = regexec(&r, header, 4, m, 0);
  regfree(&r);
  if (rs != 0)
    return NULL;
  char* res = g_strndup(header+m[2].rm_so, m[2].rm_eo-m[2].rm_so);
  return res;
}

static int _xr_server_servlet_method_call(xr_server* server, xr_servlet* servlet, xr_call* call)
{
  g_assert(server != NULL);
  g_assert(servlet != NULL);
  g_assert(call != NULL);

  // initialize servlet if necessary
  // perform a call

  xr_call_set_error(call, 100, "Method not implemented.");
  return 0;
}

static int _xr_server_parse_headers(xr_servlet* servlet, char* buf, int len)
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

#define match_iprefix(str) !strncasecmp(header, str, sizeof(str)-1)
#define match_prefix(str) !strncmp(header, str, sizeof(str)-1)

  for (it=headers; it; it=it->next)
  {
    char* header = it->data;
    if (match_prefix("POST ") && it == headers)
    {
      g_free(servlet->resource);
//      servlet->resource = _parse_http_request(header);
    }
    else if (match_iprefix("Content-Length:"))
    {
      content_length = atoi(g_strstrip(header + sizeof("Content-Length:")-1));
    }
    g_free(header);
  }
  g_slist_free(headers);

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

static int _xr_server_servlet_call(xr_server* server, xr_servlet* servlet)
{
  g_assert(server != NULL);
  g_assert(servlet != NULL);

  // read request
#define READ_STEP 128
  char request_header[1025];
  unsigned int request_header_length = 0;
  char* request_buffer_preread;
  int request_length_preread;
  char* request_buffer;
  int request_length;

  while (1)
  {
    if (request_header_length + READ_STEP > sizeof(request_header)-1)
      goto err; //ERR: headers too long
    int bytes_read = BIO_read(servlet->bio, request_header + request_header_length, READ_STEP);
    if (bytes_read <= 0)
      goto err; //ERR: err or eof in headers?
    char* eoh = _find_eoh(request_header + request_header_length, bytes_read); // search for block for \r\n\r\n (EOH)
    request_header_length += bytes_read;
    if (eoh) // end of headers in the current block
    {
      request_buffer_preread = eoh + 4;
      request_length_preread = request_header_length - (eoh - request_header + 4);
      request_header_length = eoh - request_header + 4;
      *(eoh+2) = '\0';
      break;
    }
  }

#ifdef DEBUG
  fwrite(request_header, request_header_length, 1, stdout);
  fflush(stdout);
#endif

  request_length = _xr_server_parse_headers(servlet, request_header, request_header_length);
  if (request_length <= 0 || request_length > 1024*1024)
    return -1;

  request_buffer = g_malloc(request_length);  
  memcpy(request_buffer, request_buffer_preread, request_length_preread);

  if (BIO_read(servlet->bio, request_buffer + request_length_preread, request_length - request_length_preread) 
      != request_length - request_length_preread)
  {
    g_free(request_buffer);
    return -1;
  }

#ifdef DEBUG
  fwrite(request_buffer, request_length, 1, stdout);
  fflush(stdout);
#endif

  xr_call* call = xr_call_new(NULL);
  if (xr_call_unserialize_request(call, request_buffer, request_length))
  {
    xr_call_set_error(call, 100, "Unserialize request failure.");
  }
  else
  {
    xr_call_set_error(call, 100, "Unserialize request failure.");
//    _xr_server_servlet_method_call(server, servlet, call);
  }
  g_free(request_buffer);

  // send response

  char* response_buffer;
  int response_length;
  char* response_header;
  int response_header_length;

  xr_call_serialize_response(call, &response_buffer, &response_length);
  xr_call_free(call);

  response_header = g_strdup_printf(
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/xml\r\n"
    "Content-Length: %d\r\n\r\n",
    response_length
  );
  response_header_length = strlen(response_header);
  if (BIO_write(servlet->bio, response_header, response_header_length) != response_header_length)
  {
    g_free(response_header);
    xr_call_free_buffer(response_buffer);
    return -1;
  }
  if (BIO_write(servlet->bio, response_buffer, response_length) != response_length)
  {
    g_free(response_header);
    xr_call_free_buffer(response_buffer);
    return -1;
  }
#ifdef DEBUG
  printf("%s", response_header);
  fwrite(response_buffer, response_length, 1, stdout);
  fflush(stdout);
#endif
  g_free(response_header);
  xr_call_free_buffer(response_buffer);

  return 0;
 err:
  return -1;
}

static int _xr_server_servlet_run(xr_servlet* servlet, xr_server* server)
{
  g_assert(server != NULL);
  g_assert(servlet != NULL);

  if (server->secure)
  {
    if (BIO_do_handshake(servlet->bio) <= 0)
      goto end;
  }

  servlet->running = 1;
  while (servlet->running)
  {
    if (_xr_server_servlet_call(server, servlet) < 0)
      goto end;
  }

 end:
  BIO_free_all(servlet->bio);
  g_free(servlet);
  return 0;
}

void xr_server_stop(xr_server* server)
{
  g_assert(server != NULL);
  server->running = 0;
}

int xr_server_run(xr_server* server)
{
  g_assert(server != NULL);
  GError* err = NULL;

  server->running = 1;
  while (server->running)
  {
    if (BIO_do_accept(server->bio_in) <= 0)
      goto err;
    // new connection accepted
    xr_servlet* servlet = g_new0(xr_servlet, 1);
    servlet->bio = BIO_pop(server->bio_in);
    g_thread_pool_push(server->pool, servlet, &err);
    if (err)
    {
      // reject connection if thread pool is full
      BIO_free_all(servlet->bio);
      g_free(servlet);
      g_error_free(err);
      err = NULL;
    }
  }

  return 0;
 err:
  return -1;
}

int xr_server_register_servlet(xr_server* server, xr_servlet_def* servlet)
{
  g_assert(server != NULL);
  g_assert(servlet != NULL);
  server->servlet_types = g_slist_append(server->servlet_types, servlet);
}

xr_server* xr_server_new(const char* port, const char* cert)
{
  g_assert(port != NULL);
  GError* err = NULL;

  xr_server* server = g_new0(xr_server, 1);
  server->secure = !!cert;

  server->ctx = SSL_CTX_new(SSLv3_server_method());
  if (server->ctx == NULL)
    goto err1;

  if (server->secure)
  {
    if (!SSL_CTX_use_certificate_file(server->ctx, cert, SSL_FILETYPE_PEM))
      goto err2;
    if (!SSL_CTX_use_PrivateKey_file(server->ctx, cert, SSL_FILETYPE_PEM))
      goto err2;
    if (!SSL_CTX_check_private_key(server->ctx))
      goto err2;
  }

  server->pool = g_thread_pool_new((GFunc)_xr_server_servlet_run, server, 5, TRUE, &err);
  if (err)
    goto err2;

  server->bio_in = BIO_new_accept((char*)port);
  if (server->bio_in == NULL)
    goto err3;
#ifdef DEBUG
  BIO_set_callback(server->bio_in, BIO_debug_callback);
#endif

  if (server->secure)
  {
    SSL* ssl;
    server->bio_ssl = BIO_new_ssl(server->ctx, 0);
#ifdef DEBUG
    BIO_set_callback(server->bio_ssl, BIO_debug_callback);
#endif
    BIO_get_ssl(server->bio_ssl, &ssl);
    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
    BIO_set_accept_bios(server->bio_in, server->bio_ssl);
  }

  if (BIO_do_accept(server->bio_in) <= 0)
    goto err4;

  // disable John Nagle's algo
  int flag = 1;
  int sock = -1;
  BIO_get_fd(server->bio_in, &sock);
  if (sock >= 0)
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));

  return server;
 err4:
  BIO_free(server->bio_in);
 err3:
  g_thread_pool_free(server->pool, TRUE, FALSE);
 err2:
  SSL_CTX_free(server->ctx);
 err1:
  g_free(server);
  return NULL;
}

void xr_server_free(xr_server* server)
{
  g_assert(server != NULL);
  g_thread_pool_free(server->pool, TRUE, FALSE);
  BIO_free_all(server->bio_in);
  SSL_CTX_free(server->ctx);
  g_free(server);
}

void xr_server_init()
{
  if (!g_thread_supported())
    g_thread_init(NULL);
  SSL_library_init();
  ERR_load_crypto_strings();
  SSL_load_error_strings();
  ERR_load_SSL_strings();
}
