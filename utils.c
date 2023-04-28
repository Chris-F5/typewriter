/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdlib.h>
#include <stdio.h>

#include "tw.h"

/* Call _malloc_ and catch memory error. */
void *
xmalloc(size_t len)
{
  void *p;
  if ( (p = malloc(len)) == NULL)
    perror("malloc");
  return p;
}

/* Call _realloc_ and catch memory error. */
void *
xrealloc(void *p, size_t len)
{
  if ( (p = realloc(p, len)) == NULL)
    perror("realloc");
  return p;
}

/* Check that the _font_name_ does not include any illegel characters. */
int
is_font_name_valid(const char *font_name)
{
  unsigned char c;
  /* An empty font name is invalid. */
  if (*font_name == '\0')
    return 1;
  while (*font_name) {
    c = *font_name;
    /*
     * '-' or 0-9 or '_' or A-Z or a-z
     * these are the only legal characers.
     */
    if (c == 45 || (c >= 48 && c <= 57) || c == 95 || (c >= 65 && c <= 90)
        || (c >= 97 && c <= 122)) {
      font_name++;
    } else {
      return 0; /* Return 0 on success. */
    }
  }
  return 1; /* Return 1 on failure. */
}

/* Tyy to convert _str_ to an integer stored in n. */
int
str_to_int(const char *str, int *n)
{
  char *endptr;
  *n = strtol(str, &endptr, 10);
  if (*str == '\0' || *endptr != '\0')
    return 1; /* Return 1 on failure. */
  return 0; /* Return 0 on success. */
}
