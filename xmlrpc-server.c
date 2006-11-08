#include <stdio.h>
#include <memory.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <strings.h>

#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "xmlrpc-server.h"
//#define DEBUG

struct _xr_servlet_method_desc
{
  char* name;
  servlet_method_t cb;
};

struct _xr_servlet_desc
{
  char* name;
  GHashTable* methods;
  servlet_construct_t construct;
  servlet_destruct_t destruct;
};

/* server */

typedef struct _xr_servlet xr_servlet;

struct _xr_servlet
{
  BIO* bio;
  int running;
};

struct _xr_server
{
  SSL_CTX* ctx;
  BIO* bio_in;
  BIO* bio_ssl;
  GThreadPool* pool;
  int secure;
  int running;
};

void xr_server_init()
{
  SSL_library_init();
  ERR_load_crypto_strings();
  SSL_load_error_strings();
  ERR_load_SSL_strings();
}

int xr_server_servlet_run(xr_servlet* servlet, xr_server* server)
{
  char buf[1024];
  servlet->running = 1;
  while (servlet->running)
  {
    int i = BIO_read(server->bio_in, buf, sizeof(buf));
    if (i <= 0)
      goto end;
    fwrite(buf, i, 1, stdout);
    fflush(stdout);
  }

/*

  header = g_strdup_printf(
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/xml\r\n"
    "Content-Length: %d\r\n\r\n",
    len_req
  );

*/

 end:
  BIO_ssl_shutdown(servlet->bio);
  BIO_free_all(servlet->bio);
  g_free(servlet);
}

xr_server* xr_server_new(const char* certfile, const char* port)
{
  GError* err = NULL;

  xr_server* server = g_new0(xr_server, 1);

  server->ctx = SSL_CTX_new(SSLv3_server_method());
  if (server->ctx == NULL)
    goto err1;

  if (certfile)
  {
    if (!SSL_CTX_use_certificate_file(server->ctx, certfile, SSL_FILETYPE_PEM))
      goto err2;
    if (!SSL_CTX_use_PrivateKey_file(server->ctx, certfile, SSL_FILETYPE_PEM))
      goto err2;
    if (!SSL_CTX_check_private_key(server->ctx))
      goto err2;
  }

  server->pool = g_thread_pool_new((GFunc)xr_server_servlet_run, server, 50, TRUE, &err);
  if (err)
    goto err2;

  server->bio_in = BIO_new_accept(port);
  if (server->bio_in == NULL)
    goto err3;

  server->bio_ssl = BIO_new_ssl(server->ctx, 0);
  BIO_set_accept_bios(server->bio_in, server->bio_ssl);

  if (BIO_do_accept(server->bio_in) <= 0)
    goto err4;

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
  BIO_free_all(server->bio_ssl);
  SSL_CTX_free(server->ctx);
  g_free(server);
}

void xr_server_stop(xr_server* server)
{
  g_assert(server != NULL);
  server->running = 0;
}

int xr_server_run(xr_server* server)
{
  GError* err;
  g_assert(server != NULL);

  server->running = 1;
  while (server->running)
  {
    if (BIO_do_accept(server->bio_in) <= 0)
      goto err;
    // new connection accepted
    xr_servlet* servlet = g_new0(xr_servlet, 1);
    servlet->bio = BIO_pop(server->bio_in);
    g_thread_pool_push(server->pool, servlet, &err);
  }

  return 0;
 err:
  return -1;
}


static int _xr_server_parse_headers(xr_server* server, char* buf, int len)
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
