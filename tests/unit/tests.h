#include <string.h>

#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define NORMAL "\033[0;39m"
#define WHITE "\033[1;37m"

#define RUN_TEST(func) { \
  g_print("Running " WHITE "%-70s\n" NORMAL, __FILE__ ": " G_STRINGIFY(func)); \
  gboolean rs = func(); if (!rs) failed = TRUE; \
  g_print("Done    " WHITE "%-70s[%s]\n" NORMAL, __FILE__ ": " G_STRINGIFY(func), !rs ? RED "FAILED" NORMAL : GREEN "PASSED" NORMAL); }

#define TEST_ASSERT(cond) { \
  int result = (cond); \
  g_print("        " WHITE "%-70s[%s]\n" NORMAL, __FILE__ "[" G_STRINGIFY(__LINE__) "]: " G_STRINGIFY(cond), !(result) ? RED "FAILED" NORMAL : GREEN "PASSED" NORMAL); \
  if (!result) \
    return FALSE; }
