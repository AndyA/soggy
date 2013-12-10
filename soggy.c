/* soggy.c */

#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "utils.h"
#include "ogg/ogg.h"

#define BUFSIZE 4096
#define HDR_PKT 3

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

static ogg_packet *packet_dup(ogg_packet *out, const ogg_packet *op) {
  *out = *op;
  out->packet = alloc(out->bytes);
  memcpy(out->packet, op->packet, op->bytes);
  return out;
}

static void segment(FILE *in, const char *out) {
  FILE *out = NULL;
  unsigned long seq = 0;

  ogg_sync_state ss;
  ogg_stream_state instream;
  ogg_page page;

  ogg_packet op;
  ogg_packet hdr[HDR_PKT];
  int pos = 0;
  int inited = 0;

  ogg_sync_init(&ss);

  while (!feof(in)) {
    char *buf = ogg_sync_buffer(&ss, BUFSIZE);
    if (!buf) die("Buffer allocation failed");
    size_t got = fread(buf, 1, BUFSIZE, in);
    if (ferror(in)) die("Read error: %m");
    if (got == 0) continue;
    ogg_sync_wrote(&ss, got);

    if (ogg_sync_pageout(&ss, &page) != 1) continue;

    printf("version: %d, continued: %d, bos: %d, eos: %d, "
           "granulepos: %12lld, serialno: %08x, pageno: %8ld, packets: %8d\n",
           ogg_page_version(&page),
           ogg_page_continued(&page),
           ogg_page_bos(&page),
           ogg_page_eos(&page),
           ogg_page_granulepos(&page),
           ogg_page_serialno(&page),
           ogg_page_pageno(&page),
           ogg_page_packets(&page)
          );

    if (!inited) {
      ogg_stream_init(&instream, ogg_page_serialno(&page));
      inited = 1;
    }

    if (ogg_stream_pagein(&instream, &page) < 0)
      die("Can't decode page");

    if (ogg_stream_packetout(&instream, &op) != 1) continue;

    /* Stash header packets for replay */
    if (pos < HDR_PKT) {
      packet_dup(&hdr[pos++], &op);
      continue;
    }

    if (!out) {
    }
  }
}

int main(int argc, char *argv[]) {
  parse_options(&argc, &argv);
  if (argc != 1) die("soggy <outfile%%08d.ogg>");
  segment(stdin, argv[0]);
  return 0;
}

/* vim:instream=2:sw=2:sts=2:et:ft=c
 */








































