/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "twpdf.h"
#include "document.h"
#include "stralloc.h"
#include "arg.h"

static int read_line(FILE *file, char **line, int *allocated);
static void read_file(struct document *doc, FILE *file);

struct stralloc stralloc;

static int font_size;
static int top_margin;
static int bot_margin;
static int left_margin;
static int right_margin;
static const char *tab_expand;

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
  int line_allocated, image_size;
  image_size = 595 - left_margin - right_margin;

  line_allocated = 256;
  line = xmalloc(line_allocated);
  while (read_line(stdin, &line, &line_allocated)) {
    str = stralloc_alloc(&stralloc, line);
    if (str[0] == '\0') {
      put_glue(doc, font_size * 30, font_size);
    } else if (strcmp(str, "---") == 0) {
      put_glue(doc, 0, 0);
    } else if (strncmp(str, "!IMAGE_SIZE ", strlen("!IMAGE_SIZE ")) == 0) {
      str += strlen("!IMAGE_SIZE ");
      image_size = atoi(str);
    } else if (strncmp(str, "!IMAGE ", strlen("!IMAGE ")) == 0) {
      str += strlen("!IMAGE ");
      put_glue(doc, font_size * 40, font_size / 2);
      put_image(doc, str, image_size);
    } else {
      put_glue(doc, font_size * 40, font_size / 2);
      put_text(doc, str, font_size);
    }
  }
  free(line);
}

int
main(int argc, char **argv)
{
  struct document doc;
  const char *output_fname;
  int c;

  font_size = 9;
  top_margin = 40;
  bot_margin = 40;
  left_margin = 80;
  right_margin = 80;
  tab_expand = "    ";
  output_fname = "output.pdf";
  while ( (c = next_opt(argc, argv, "s#v#h#t*o*")) != -1) {
    switch (c) {
    case 's':
      font_size = opt_arg_int;
      break;
    case 'v':
      top_margin = opt_arg_int;
      bot_margin = opt_arg_int;
      break;
    case 'h':
      left_margin = opt_arg_int;
      right_margin = opt_arg_int;
      break;
    case 't':
      tab_expand = opt_arg_string;
      if (strlen(tab_expand) > 32) {
        fprintf(stderr, "Tab expand too long.\n");
        exit(1);
      }
      break;
    case 'o':
      output_fname = opt_arg_string;
      break;
    }
  }

  stralloc_init(&stralloc);
  init_document(&doc, top_margin, bot_margin, left_margin);

  read_file(&doc, stdin);

  optimise_breaks(&doc);
  build_document(&doc);
  pdf_write(&doc.pdf, output_fname);

  free_document(&doc);
  stralloc_free(&stralloc);
  return 0;
}
