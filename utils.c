/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdlib.h>
#include <stdio.h>

#include "tw.h"

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

int
is_font_name_valid(const char *font_name)
{
  unsigned char c;
  if (*font_name == '\0')
    return 1;
  while (*font_name) {
    c = *font_name;
    if (c == 45 || (c >= 48 && c <= 57) || c == 95 || (c >= 65 && c <= 90)
        || (c >= 97 && c <= 122)) {
      font_name++;
    } else {
      return 0;
    }
  }
  return 1;
}

int
str_to_int(const char *str, int *n)
{
  char *endptr;
  *n = strtol(str, &endptr, 10);
  if (*str == '\0' || *endptr != '\0')
    return 1;
  return 0;
}
