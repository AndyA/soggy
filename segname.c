/* segname.c */

#include <inttypes.h>
#include <jd_pretty.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "segname.h"
#include "utils.h"

#define TMP_PREFIX_LEN 10

static void freep(char **cp) {
  if (*cp) {
    free(*cp);
    *cp = NULL;
  }
}

static void free_names(segname *sn) {
  freep(&sn->uri);
  freep(&sn->cur_name);
  freep(&sn->tmp_name);
}

static unsigned push_field(segname *sn, const char *frag,
                           unsigned frag_len, unsigned field_len, unsigned pos) {
  segname_field *snf = alloc(sizeof(*snf));

  char fb[frag_len + 1];
  memcpy(fb, frag, frag_len);
  fb[frag_len] = '\0';

  snf->frag = frag;
  snf->frag_len = frag_len;
  snf->field_len = field_len;
  snf->pos = pos;

  snf->next = sn->fld;
  sn->fld = snf;
  return pos + frag_len + field_len;
}

static unsigned parse_format(segname *sn) {
  unsigned pos = 0;
  const char *fmt = sn->fmt;
  const char *lp = fmt;
  while (*fmt) {
    if (*fmt++ == '%') {
      if (*fmt == '%') {
        fmt++;
        continue;
      }
      char *ep;
      unsigned long len = strtoul(fmt, &ep, 10);
      if (fmt == ep) jd_throw("Missing field width");
      if (len == 0) jd_throw("Field width must be non-zero");
      if (!*ep) jd_throw("Missing conversion character");
      if (*ep != 'd') jd_throw("The only legal conversion is %%Nd");

      pos = push_field(sn, lp, fmt - lp - 1, len, pos);
      fmt = lp = ep + 1;
    }
  }
  return push_field(sn, lp, fmt - lp, 0, pos);
}

segname *segname_new_prefixed(const char *fmt, const char *prefix) {
  segname *sn = alloc(sizeof(*sn));
  sn->fmt = fmt;
  sn->len = parse_format(sn);
  sn->prefix = sstrdup(prefix);
  return sn;
}

segname *segname_new(const char *fmt) {
  return segname_new_prefixed(fmt, NULL);
}

static void free_fields(segname_field *snf) {
  for (segname_field *next = snf; next; snf = next) {
    next = snf->next;
    free(snf);
  }
}

void segname_free(segname *sn) {
  if (sn) {
    free_fields(sn->fld);
    free_names(sn);
    free(sn->prefix);
    free(sn);
  }
}

int segname_parse(segname *sn, const char *name) {
  if (strlen(name) != sn->len) return 0;
  free_names(sn);
  for (segname_field *snf = sn->fld; snf; snf = snf->next) {
    if (snf->field_len) {
      char tmp[snf->field_len + 1];
      if (memcmp(name + snf->pos, snf->frag, snf->frag_len)) return 0;
      memcpy(tmp, name + snf->pos + snf->frag_len, snf->field_len);
      tmp[snf->field_len] = '\0';
      char *ep;
      snf->seq = strtoul(tmp, &ep, 10);
      if (ep - tmp != snf->field_len) return 0;
    }
  }

  return 1;
}

int segname_inc(segname *sn) {
  free_names(sn);
  for (segname_field *snf = sn->fld; snf; snf = snf->next) {
    uint64_t limit = 1;
    for (unsigned i = 0; i < snf->field_len; i++) limit *= 10;
    if (++snf->seq < limit) return 0;
    snf->seq = 0;
  }
  return 1;
}

char *segname_format(segname *sn) {
  char *buf = alloc(sn->len + 1);

  for (segname_field *snf = sn->fld; snf; snf = snf->next) {
    memcpy(buf + snf->pos, snf->frag, snf->frag_len);
    if (snf->field_len) {
      char fmt[20], tmp[snf->field_len + 1];
      sprintf(fmt, "%%0%u" PRIu64, snf->field_len);
      sprintf(tmp, fmt, snf->seq);
      memcpy(buf + snf->pos + snf->frag_len, tmp, snf->field_len);
    }
  }

  return buf;
}

char *segname_next(segname *sn) {
  char *next = segname_format(sn);
  segname_inc(sn);
  return next;
}

char *segname_uri(segname *sn) {
  if (!sn->uri)
    sn->uri = segname_format(sn);
  return sn->uri;
}

static char *prefix(const char *name, const char *prefix) {
  if (!prefix || !*prefix) return sstrdup(name);
  size_t len = strlen(prefix);
  size_t plen = len;
  if (prefix[plen - 1] != '/') plen++;
  char *fn = alloc(plen + strlen(name) + 1);
  memcpy(fn, prefix, len);
  if (len != plen) fn[len] = '/';
  strcpy(fn + plen, name);
  return fn;
}

static char *random_chars(char *s, size_t len) {
  for (unsigned i = 0; i < len;) {
    int cc  = rand() & 0x3f;
    if (cc < 26 + 10) s[i++] = cc < 26 ? 'a' + cc : '0' + cc - 26;
  }
  return s;
}

static char *tmp_name(const char *filename) {
  size_t len = strlen(filename);
  char *tmp = alloc(len + TMP_PREFIX_LEN + 2);
  const char *slash = strrchr(filename, '/');
  if (slash) slash++;
  else slash = filename;
  memcpy(tmp, filename, slash - filename);
  random_chars(tmp + (slash - filename), TMP_PREFIX_LEN);
  tmp[(slash - filename) + TMP_PREFIX_LEN] = '.';
  memcpy(tmp + (slash - filename) + TMP_PREFIX_LEN + 1, slash, len + filename - slash + 1);
  return tmp;
}

char *segname_prefix(segname *sn, const char *name) {
  return prefix(name, sn->prefix);
}

char *segname_name(segname *sn) {
  if (!sn->cur_name)
    sn->cur_name = segname_prefix(sn, segname_uri(sn));
  return sn->cur_name;
}

char *segname_temp(segname *sn) {
  if (!sn->tmp_name)
    sn->tmp_name = tmp_name(segname_name(sn));
  return sn->tmp_name;
}

void segname_rename(segname *sn) {
  char *temp = segname_temp(sn);
  char *name = segname_name(sn);
  if (rename(temp, name))
    jd_throw("Can't rename %s as %s: %m", temp, name);
}

/* vim:ts=2:sw=2:sts=2:et:ft=c
 */



