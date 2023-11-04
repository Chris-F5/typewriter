/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

struct str {
  struct str *next;
  char str[];
};

struct stralloc {
  struct str *strs;
};

void stralloc_init(struct stralloc *stralloc);
void stralloc_free(struct stralloc *stralloc);
char *stralloc_alloc(struct stralloc *stralloc, const char *str);
