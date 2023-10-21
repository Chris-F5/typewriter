#include <stdio.h>

#include "twpdf.h"

int
main(int argc, char **argv)
{
  struct pdf pdf;
  pdf_init_empty(&pdf);
  pdf_write(&pdf, "output.pdf");
  pdf_free(&pdf);
  return 0;
}
