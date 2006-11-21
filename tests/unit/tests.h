#include <string.h>

#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define NORMAL "\033[0;39m"
#define WHITE "\033[1;37m"

#define RUN_TEST(func) \
  g_print("Running " WHITE "%-70s", __FILE__ "[" G_STRINGIFY(__LINE__) "]:" G_STRINGIFY(func)); \
  g_print("%s\n" NORMAL, func() ? RED "[FAILED]" : GREEN "[PASSED]");
