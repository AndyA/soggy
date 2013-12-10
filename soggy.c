/* soggy.c */

#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "utils.h"

#define PROG "soggy"

static void usage() {
  fprintf(stderr, "Usage: " PROG " [options] <json>...\n\n"
          "Options:\n"
          "  -h, --help           See this text\n"
          "  -V, --version        Show version\n"
          "\n" PROG " %s\n", v_info);
  exit(1);
}

static void parse_options(int *argc, char ***argv) {
  int ch, oidx;

  static struct option opts[] = {
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'V'},
    {NULL, 0, NULL, 0}
  };

  while (ch = getopt_long(*argc, *argv, "hV", opts, &oidx), ch != -1) {
    switch (ch) {
    case 'V':
      version();
      break;
    case 'h':
    default:
      usage();
    }
  }

  *argc -= optind;
  *argv += optind;
}

int main(int argc, char *argv[]) {
  parse_options(&argc, &argv);
  return 0;
}

/* vim:ts=2:sw=2:sts=2:et:ft=c
 */
