#include <stdio.h>

#include "tw.h"

int
main(int argc, char **argv)
{
  FILE *pages_file, *font_file, *pdf_file;
  pages_file = fopen("./pages", "r");
  font_file = fopen("./cmu.serif-roman.ttf", "rb");
  pdf_file = fopen("./output.pdf", "w");
  parse_pages(pages_file, font_file, pdf_file);
  fclose(pages_file);
  fclose(font_file);
  fclose(pdf_file);
  printf("DONE\n");
  return 0;
}
