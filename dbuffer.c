/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "tw.h"

/* Initialise the dbuffer pointed to by _buf_. */
void
dbuffer_init(struct dbuffer *buf, int initial, int increment)
{
  buf->size = 0;
  buf->allocated = initial;
  buf->increment = increment;
  /* Allocate the initial heap memory. */
  buf->data = xmalloc(buf->allocated);
}

/* Write a character to the end of a dynamic buffer. */
void
dbuffer_putc(struct dbuffer *buf, char c)
{
  /* If there is insufficient space in the buffer then allocate more. */
  if (buf->allocated == buf->size) {
    buf->allocated += buf->increment;
    buf->data = xrealloc(buf->data, buf->allocated);
  }
  /* Add the character and increment _size_. */
  buf->data[buf->size++] = c;
}

/* Formatted print to the end of the buffer. */
void
dbuffer_printf(struct dbuffer *buf, const char *format, ...)
{
  va_list args;
  int len;
  va_start(args, format);
  /*
   * Write to data but don't overflow the buffer. _len_ is the number of bytes
   * that would have been written if the buffer were large enough.
   */
  len = vsnprintf(buf->data + buf->size, buf->allocated - buf->size, format,
      args);
  /* If there was not enough space allocated: */
  if (len >= buf->allocated - buf->size) {
    /* Allocate enough space for _len_ bytes. */
    buf->allocated += buf->increment * (1 + len / buf->increment);
    buf->data = xrealloc(buf->data, buf->allocated);
    /*
     * Write to data without checking overflow because we know there is enough
     * space allocated.
     */
    vsprintf(buf->data + buf->size, format, args);
  }
  buf->size += len;
  va_end(args);
}

/* Free dynamic buffer heap allocated memory. */
void
dbuffer_free(struct dbuffer *buf)
{
  free(buf->data);
}
