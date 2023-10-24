/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "utils.h"

void *
xmalloc(size_t len)
{
  void *p;
  if ( (p = malloc(len)) == NULL)
    perror("malloc");
  return p;
}

void *
xrealloc(void *p, size_t len)
{
  if ( (p = realloc(p, len)) == NULL)
    perror("realloc");
  return p;
}

void
revsprintf(char **stream, long *allocated, long *length, const char *format, va_list args)
{
  va_list args_copy;
  int written;
  va_copy(args_copy, args);
  written = vsnprintf(*stream + *length, *allocated - *length, format, args);
  if (written >= *allocated - *length) {
    *allocated += written + 1024 * 4;
    *stream = xrealloc(*stream, *allocated);
    vsprintf(*stream + *length, format, args_copy);
  }
  *length += written;
  va_end(args_copy);
}

void
resprintf(char **stream, long *allocated, long *length, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  revsprintf(stream, allocated, length, format, args);
  va_end(args);
}
