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
  pdf_init_empty(&pdf);
  content_ref = pdf_allocate_indirect_obj(&pdf);
  catalogue_ref = pdf_allocate_indirect_obj(&pdf);
  pdf_pages_init(&pdf, &pages);
  pdf_content_init(&content);
  pdf_content_define(&pdf, content_ref, &content);
  pdf_content_free(&content);
  pdf_pages_add_page(&pdf, &pages, content_ref);
  pdf_pages_define_catalogue(&pdf, catalogue_ref, &pages);
  pdf_write(&pdf, "output.pdf");
  pdf_free(&pdf);
  pdf_pages_free(&pages);
  return 0;
}
