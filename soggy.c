/* soggy.c */

#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"
#include "hls.h"
#include "segname.h"
#include "ogg/ogg.h"
#include "vorbis/codec.h"

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

struct work {
  FILE *out;

  segname *seg;

  ogg_sync_state ss;
  ogg_stream_state is, os;
  int sn;

  ogg_packet op;
  ogg_packet hdr[HDR_PKT];
  int pos;

  vorbis_info vi;
  vorbis_comment vc;
  vorbis_dsp_state vds;
};

static int new_sn(struct work *w) {
  int nsn = rand();
  while (nsn == w->sn) nsn = rand();
  w->sn = nsn;
  return w->sn;
}

static void write_bytes(FILE *fl, void *b, size_t len) {
  size_t done = fwrite(b, 1, len, fl);
  if (done != len) die("Write failed: %m");
}

static void write_page(struct work *w, ogg_page *p) {
  write_bytes(w->out, p->header, p->header_len);
  write_bytes(w->out, p->body, p->body_len);
}

static void flush_out(struct work *w) {
  ogg_page page;
  while (ogg_stream_flush(&w->os, &page)) {
    write_page(w, &page);
  }
}

static void close_out(struct work *w) {
  if (w->out) {
    flush_out(w);
    fclose(w->out);
    w->out = NULL;
    segname_rename(w->seg);
    segname_inc(w->seg);
  }
}

static void next_file(struct work *w) {
  close_out(w);

  char *tmp = segname_temp(w->seg);
  printf("writing %s\n", tmp);
  if (w->out = fopen(tmp, "wb"), !w->out)
    die("Can't write %s: %m", tmp);

  if (ogg_stream_init(&w->os, new_sn(w)) < 0)
    die("Can't init output stream");

  for (int i = 0; i < w->pos; i++) {
    ogg_page page;
    if (ogg_stream_packetin(&w->os, &w->hdr[i]) < 0)
      die("Can't write header packet");
    if (ogg_stream_pageout(&w->os, &page))
      write_page(w, &page);
  }
  flush_out(w);
}

static void send_packet(struct work *w, ogg_packet *op) {
  ogg_page page;
  if (ogg_stream_packetin(&w->os, op) < 0)
    die("Can't write header packet");
  while (ogg_stream_pageout(&w->os, &page))
    write_page(w, &page);
}

static void segment(FILE *in, segname *seg, double gop) {
  struct work w;
  ogg_page page;
  vorbis_block vb;
  int inited = 0;
  double sent = 0;

  w.pos = 0;
  w.seg = seg;
  w.out = NULL;

  ogg_sync_init(&w.ss);
  vorbis_info_init(&w.vi);
  vorbis_comment_init(&w.vc);

  while (!feof(in)) {
    char *buf = ogg_sync_buffer(&w.ss, BUFSIZE);
    if (!buf) die("Buffer allocation failed");
    size_t got = fread(buf, 1, BUFSIZE, in);
    if (ferror(in)) die("Read error: %m");
    if (got == 0) continue;
    ogg_sync_wrote(&w.ss, got);

    if (ogg_sync_pageout(&w.ss, &page) != 1) continue;

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
      if (ogg_stream_init(&w.is, ogg_page_serialno(&page)) < 0)
        die("Can't init stream");
      inited = 1;
    }

    if (ogg_stream_pagein(&w.is, &page) < 0)
      die("Can't decode page");

    while (ogg_stream_packetout(&w.is, &w.op) == 1) {

      /* Stash header packets for replay */
      if (w.pos < HDR_PKT) {
        vorbis_synthesis_headerin(&w.vi, &w.vc, &w.op);
        packet_dup(&w.hdr[w.pos++], &w.op);
        if (w.pos == HDR_PKT) {
          if (vorbis_synthesis_init(&w.vds, &w.vi) < 0)
            die("Can't init Vorbis decoder");
          if (vorbis_block_init(&w.vds, &vb) < 0)
            die("Can't init block");
        }
        continue;
      }

      vorbis_synthesis_trackonly(&vb, &w.op);
      vorbis_synthesis_blockin(&w.vds, &vb);

      double tm = vorbis_granule_time(&w.vds, ogg_page_granulepos(&page));

      if (!w.out) next_file(&w);

      if (tm >= sent + gop) {
        w.op.e_o_s = 1;
        send_packet(&w, &w.op);
        close_out(&w);
        sent = tm;
      }
      else {
        send_packet(&w, &w.op);
      }
    }
  }

  close_out(&w);
}

int main(int argc, char *argv[]) {
  parse_options(&argc, &argv);
  if (argc != 1) die("soggy <outfile%%08d.ogg>");
  segname *seg = segname_new(argv[0]);
  segment(stdin, seg, 8);
  segname_free(seg);
  return 0;
}

/* vim:is=2:sw=2:sts=2:et:ft=c
 */


















