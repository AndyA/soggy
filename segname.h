/* segname.h */

#ifndef MC_SEGNAME_H_
#define MC_SEGNAME_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct segname_field {
  struct segname_field *next;
  const char *frag;
  unsigned frag_len, field_len;
  unsigned pos;
  uint64_t seq;
} segname_field;

typedef struct {
  segname_field *fld;
  const char *fmt;
  unsigned len;
  char *prefix;
  char *uri;
  char *cur_name;
  char *tmp_name;
} segname;

segname *segname_new_prefixed(const char *fmt, const char *prefix);
segname *segname_new(const char *fmt);
void segname_free(segname *sn);
int segname_parse(segname *sn, const char *name);
int segname_inc(segname *sn);
char *segname_format(segname *sn);
char *segname_next(segname *sn);
char *segname_uri(segname *sn);
char *segname_name(segname *sn);
char *segname_temp(segname *sn);
void segname_rename(segname *sn);
char *segname_prefix(segname *sn, const char *name);

#ifdef __cplusplus
}
#endif

#endif

/* vim:ts=2:sw=2:sts=2:et:ft=c
 */

