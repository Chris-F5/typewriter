/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "utils.h"
#include "stralloc.h"

void 
stralloc_init(struct stralloc *stralloc)
{
  stralloc->strs = NULL;
}

void 
stralloc_free(struct stralloc *stralloc)
{
  struct str *str, *next_str;
  for (str = stralloc->strs; str; str = next_str) {
    next_str = str->next;
    free(str);
  }
}

char *
stralloc_alloc(struct stralloc *stralloc, const char *str)
{
  struct str *s;
  long size;
  size = strlen(str) + 1;
  s = xmalloc(sizeof(struct str) + size);
  s->next = stralloc->strs;
  memcpy(s->str, str, size);
  return s->str;
}
