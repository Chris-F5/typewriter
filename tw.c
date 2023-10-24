/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdio.h>
#include <stdlib.h>

#include "twpdf.h"
#include "twpages.h"
#include "twcontent.h"

char line[256];

void
write_line(struct pdf_content *content, const char *line, int y)
{
  pdf_content_write(content, "1 0 0 1 %d %d Tm\n", 40, y);
  pdf_content_write(content, "(");
  for (; *line; line++) switch (*line) {
  case '(':
  case ')':
  case '\\':
    pdf_content_write(content, "\\");
  default:
    pdf_content_write(content, "%c", *line);
  }
  pdf_content_write(content, ") Tj\n");
}

int
eat_page(FILE *in, struct pdf_content *content)
{
  int c, empty, row, col;
  empty = 1;
  row = col = 0;
  pdf_content_write(content, "BT\n");
  pdf_content_write(content, "/F0 12 Tf\n");
  while ( (c = fgetc(in)) != EOF) {
    empty = 0;
    if (col >= sizeof(line) - 1) {
      fprintf(stderr, "line buffer full\n");
      exit(1);
    }
    if (c == '\n') {
      line[col] = '\0';
      write_line(content, line, 842 - 32 - row * 12);
      row++;
      col = 0;
      if (row * 12 + 42 > 842 - 20)
        break;
      continue;
    }
    line[col++] = c;
  }
  pdf_content_write(content, "ET\n");
  return !empty;
}

int
main(int argc, char **argv)
{
  struct pdf pdf;
  struct pdf_pages pages;
  struct pdf_content content;
  struct pdf_obj_indirect *content_ref, *catalogue_ref;
  struct pdf_obj *resources;
  pdf_init_empty(&pdf);
  catalogue_ref = pdf_allocate_indirect_obj(&pdf);
  pdf_pages_init(&pdf, &pages);
  pdf_content_init(&content);
  while (eat_page(stdin, &content)) {
    content_ref = pdf_allocate_indirect_obj(&pdf);
    pdf_content_define(&pdf, content_ref, &content);
    pdf_content_reset(&content);
    pdf_pages_add_page(&pdf, &pages, content_ref);
  }
  pdf_content_free(&content);
  resources = pdf_content_create_resources(&pdf);
  pdf_pages_define_catalogue(&pdf, catalogue_ref, &pages, resources);
  pdf_write(&pdf, "output.pdf");
  pdf_free(&pdf);
  pdf_pages_free(&pages);
  return 0;
}
