#ifndef __XR_UTILS_H__
#define __XR_UTILS_H__

#include <glib.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

/** @file xr_utils Header
 */

void xr_set_nodelay(BIO* bio);
const char* xr_get_bio_error_string();

#endif
