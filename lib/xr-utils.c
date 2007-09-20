#ifdef WIN32
  #include <winsock2.h>
#else
  #include <arpa/inet.h>
  #include <netinet/tcp.h>
#endif

#include "xr-utils.h"

void xr_set_nodelay(BIO* bio)
{
  int flag = 1;
  int sock = -1;
  BIO_get_fd(bio, &sock);
  if (sock >= 0)
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
  else
    fprintf(stderr, "Error disabling Nagle.\n");
}

const char* xr_get_bio_error_string()
{
  const char* str;
  if ((str = ERR_reason_error_string(ERR_get_error())))
    return str;
#ifndef WIN32
  if ((str = g_strerror(errno)))
    return str;
#else
  if (WSAGetLastError())
    return "WSA ERROR.";
#endif
  return "Unknown or no error.";
}
