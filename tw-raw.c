/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "utils.h"
#include "twpdf.h"
#include "document.h"
#include "stralloc.h"

static int read_line(FILE *file, char **line, int *allocated);
static void read_file(struct document *doc, FILE *file);

struct stralloc stralloc;

static const int font_size = 9;
static const int top_margin = 40;
static const int bot_margin = 40;
static const int left_margin = 80;
static const char *tab_expand = "    ";

static int
read_line(FILE *file, char **line, int *allocated)
{
  int c, len, i;
  len = 0;
  while ( (c = fgetc(file)) != EOF) {
    if (len + 64 >= *allocated) {
      *allocated += 256;
      *line = xrealloc(*line, *allocated);
    }
    switch (c) {
    case '\r':
      break;
    case '\n':
      (*line)[len] = '\0';
      return 1;
    case '\t':
      for (i = 0; tab_expand[i] != '\0'; i++)
        (*line)[len++] = tab_expand[i];
      break;
    default:
      (*line)[len++] = c;
    }
  }
  (*line)[len] = '\0';
  return 0;
}

static void
read_file(struct document *doc, FILE *file)
{
  char *line, *str;
  int line_allocated;

  line_allocated = 256;
  line = xmalloc(line_allocated);
  while (read_line(stdin, &line, &line_allocated)) {
    str = stralloc_alloc(&stralloc, line);
    put_text(doc, str, font_size);
    put_glue(doc, 0, 0);
  }
  free(line);
}

int
main(int argc, char **argv)
{
  struct document doc;
  stralloc_init(&stralloc);
  init_document(&doc, top_margin, bot_margin, left_margin);

  read_file(&doc, stdin);

  optimise_breaks(&doc);
  build_document(&doc);
  pdf_write(&doc.pdf, "output.pdf");

  free_document(&doc);
  stralloc_free(&stralloc);
  return 0;
}