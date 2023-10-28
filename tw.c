/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdio.h>
#include <stdlib.h>

#include "twpdf.h"
#include "twpages.h"
#include "twcontent.h"
#include "twjpeg.h"

static const int top_margin = 40;
static const int bot_margin = 40;
static const int left_margin = 80;

static const int font_size = 10;
static const char *tab_expand = "    ";

char line[256];

int
eat_page(FILE *in, struct pdf_content *content)
{
  int c, empty, row, col;
  empty = 1;
  row = col = 0;
  while ( (c = fgetc(in)) != EOF) {
    empty = 0;
    if (col >= sizeof(line) - 1) {
      fprintf(stderr, "line buffer full\n");
      exit(1);
    }
    if (c == '\n') {
      line[col] = '\0';
      pdf_content_write_text(content, line, left_margin,
          842 - top_margin - (row + 1) * font_size, 12);
      row++;
      col = 0;
      if ((row + 1) * font_size + top_margin + bot_margin > 842)
        break;
      continue;
    }
    line[col++] = c;
  }
  pdf_content_write_image(content, "Img0", 30, 30, 200, 100);
  return !empty;
}

int
main(int argc, char **argv)
{
  struct pdf pdf;
  struct pdf_pages pages;
  struct pdf_content content;
  struct pdf_obj_indirect *content_ref, *catalogue_ref;
  struct pdf_obj_dictionary *xobjects;
  struct pdf_obj *resources;
  struct pdf_jpeg_info img_info;
  pdf_init_empty(&pdf);
  catalogue_ref = pdf_allocate_indirect_obj(&pdf);
  pdf_pages_init(&pdf, &pages);
  pdf_content_init(&content);
  xobjects = pdf_create_dictionary(&pdf);
  xobjects = pdf_prepend_dictionary(&pdf, xobjects, "Img0",
      (struct pdf_obj *)pdf_jpeg_define(&pdf, "image1.jpg", &img_info));
  while (eat_page(stdin, &content)) {
    content_ref = pdf_allocate_indirect_obj(&pdf);
    pdf_content_define(&pdf, content_ref, &content);
    pdf_content_reset_page(&content);
    pdf_pages_add_page(&pdf, &pages, content_ref);
  }
  pdf_content_free(&content);
  resources = pdf_content_create_resources(&pdf, xobjects);
  pdf_pages_define_catalogue(&pdf, catalogue_ref, &pages, resources);
  pdf_write(&pdf, "output.pdf");
  pdf_free(&pdf);
  pdf_pages_free(&pages);
  return 0;
}
