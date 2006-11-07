#include <stdio.h>
#include <string.h>
#include <glib.h>

#include "xmlrpc-server.h"

static GOptionEntry entries[] = 
{
//  { "", '\0', 0, G_OPTION_ARG_STRING, &, "", "" },
//  { "", '\0', 0, G_OPTION_ARG_NONE, &, "", 0 },
  { NULL }
};

int main(int ac, char* av[])
{
  GOptionContext* ctx = g_option_context_new("- XML-RPC SSL Spike Server.");
  g_option_context_add_main_entries(ctx, entries, NULL);
  g_option_context_parse(ctx, &ac, &av, NULL);
  g_option_context_set_help_enabled(ctx, TRUE);

  return 0;
}
