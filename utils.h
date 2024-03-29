/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

/*
 * The following must be included before this file:
#include <stdarg.h>
#include <stdlib.h>
 */

void *xmalloc(size_t len);
void *xrealloc(void *p, size_t len);
void revsprintf(char **stream, long *allocated, long *length, const char *format, va_list args);
void resprintf(char **stream, long *allocated, long *length, const char *format, ...);
