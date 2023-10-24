/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdio.h>

#include "twpdf.h"
#include "twpages.h"
#include "twcontent.h"

int
main(int argc, char **argv)
{
  struct pdf pdf;
  struct pdf_pages pages;
  struct pdf_content content;
  struct pdf_obj_indirect *content_ref, *catalogue_ref;
  struct pdf_obj *resources;
  pdf_init_empty(&pdf);
  content_ref = pdf_allocate_indirect_obj(&pdf);
  catalogue_ref = pdf_allocate_indirect_obj(&pdf);
  pdf_pages_init(&pdf, &pages);
  pdf_content_init(&content);
  pdf_content_write(&content, "BT\n");
  pdf_content_write(&content, "/F0 12 Tf\n");
  pdf_content_write(&content, "1 0 0 1 %d %d Tm\n", 100, 100);
  pdf_content_write(&content, "(hello world) Tj\n");
  pdf_content_write(&content, "ET\n");
  pdf_content_define(&pdf, content_ref, &content);
  resources = pdf_content_create_resources(&pdf);
  pdf_content_free(&content);
  pdf_pages_add_page(&pdf, &pages, content_ref);
  pdf_pages_define_catalogue(&pdf, catalogue_ref, &pages, resources);
  pdf_write(&pdf, "output.pdf");
  pdf_free(&pdf);
  pdf_pages_free(&pages);
  return 0;
}
