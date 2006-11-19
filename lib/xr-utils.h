#ifndef __XR_UTILS_H__
#define __XR_UTILS_H__

#include <glib.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

/** @file xr_utils Header
 */

void xr_set_nodelay(BIO* bio);
void xr_ssl_init();
void xr_ssl_fini();

#endif
