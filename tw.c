/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdio.h>

#include "twpdf.h"
#include "twpages.h"

int
main(int argc, char **argv)
{
  struct pdf pdf;
  struct pdf_pages pages;
  pdf_init_empty(&pdf);
  pdf_pages_init(&pdf, &pages);
  pdf_pages_add_page(&pdf, &pages, NULL);
  pdf_pages_define_catalogue(&pdf, &pages);
  pdf_write(&pdf, "output.pdf");
  pdf_free(&pdf);
  pdf_pages_free(&pages);
  return 0;
}
