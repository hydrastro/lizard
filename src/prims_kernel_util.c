/* src/prims_kernel_util.c -- shared helpers for kernel primitives. */
#include "prims_kernel_util.h"
#include "mem.h"

#include <stdio.h>
#include <string.h>

const char *lizard_kernel_term_to_string(lizard_heap_t *heap, kterm_t *t) {
  char buf[512];
  FILE *fp;
  long pos;
  size_t got;
  char *sbuf;

  fp = tmpfile();
  if (fp == NULL) {
    return "<kterm>";
  }
  kt_fprint(fp, t);
  fflush(fp);
  pos = ftell(fp);
  if (pos <= 0 || (size_t)pos >= sizeof(buf)) {
    fclose(fp);
    return "<kterm>";
  }
  rewind(fp);
  got = fread(buf, 1, (size_t)pos, fp);
  buf[got] = '\0';
  fclose(fp);
  sbuf = (char *)lizard_heap_alloc(strlen(buf) + 1U);
  strcpy(sbuf, buf);
  return sbuf;
}
