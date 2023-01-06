#include "tw.h"

#include <stdarg.h>
#include <stdlib.h>

void
bytes_init(struct bytes *bytes, int initial, int increment)
{
  bytes->count = 0;
  bytes->allocated = initial;
  bytes->increment = increment;
  bytes->data = xmalloc(bytes->allocated);
  bytes->data[0] = '\0';
}

void
bytes_printf(struct bytes *bytes, const char *format, ...)
{
  va_list args;
  int len;
  va_start(args, format);
  len = vsnprintf(bytes->data + bytes->count, bytes->allocated - bytes->count,
      format, args);
  if (len >= bytes->allocated - bytes->count) {
    bytes->allocated += bytes->increment * (1 + len / bytes->increment);
    bytes->data = xrealloc(bytes->data, bytes->allocated);
    vsprintf(bytes->data + bytes->count, format, args);
  }
  bytes->count += len;
  va_end(args);
}

void
bytes_free(struct bytes *bytes)
{
  free(bytes->data);
}
