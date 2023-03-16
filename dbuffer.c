/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "tw.h"

void
dbuffer_init(struct dbuffer *buf, int initial, int increment)
{
  buf->size = 0;
  buf->allocated = initial;
  buf->increment = increment;
  buf->data = xmalloc(buf->allocated);
}

void
dbuffer_putc(struct dbuffer *buf, char c)
{
  if (buf->allocated == buf->size) {
    buf->allocated += buf->increment;
    buf->data = xrealloc(buf->data, buf->allocated);
  }
  buf->data[buf->size++] = c;
}

void
dbuffer_printf(struct dbuffer *buf, const char *format, ...)
{
  va_list args;
  int len;
  va_start(args, format);
  len = vsnprintf(buf->data + buf->size, buf->allocated - buf->size, format,
      args);
  if (len >= buf->allocated - buf->size) {
    buf->allocated += buf->increment * (1 + len / buf->increment);
    buf->data = xrealloc(buf->data, buf->allocated);
    vsprintf(buf->data + buf->size, format, args);
  }
  buf->size += len;
  va_end(args);
}

void
dbuffer_free(struct dbuffer *buf)
{
  free(buf->data);
}
