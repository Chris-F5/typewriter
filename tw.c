/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdio.h>

#include "tw.h"

int
main(int argc, char **argv)
{
  FILE *pages_file, *typeface_file, *pdf_file;
  pages_file = stdin;
  typeface_file = fopen("./typeface", "r");
  pdf_file = fopen("./output.pdf", "w");
  print_pages(pages_file, typeface_file, pdf_file);
  fclose(pages_file);
  fclose(typeface_file);
  fclose(pdf_file);
  return 0;
}
