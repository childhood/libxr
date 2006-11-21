#include <string.h>

#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define NORMAL "\033[0;39m"
#define WHITE "\033[1;37m"

#define RUN_TEST(func) { \
  g_print("Running " WHITE "%-70s\n" NORMAL, __FILE__ ": " G_STRINGIFY(func)); \
  int rs = func(); \
  g_print("Done    " WHITE "%-70s[%s]\n" NORMAL, __FILE__ ": " G_STRINGIFY(func), rs ? RED "FAILED" NORMAL : GREEN "PASSED" NORMAL); }

#define TEST_ASSERT(cond) \
  g_print("        " WHITE "%-70s[%s]\n" NORMAL, __FILE__ "[" G_STRINGIFY(__LINE__) "]: " G_STRINGIFY(cond), !(cond) ? RED "FAILED" NORMAL : GREEN "PASSED" NORMAL);
