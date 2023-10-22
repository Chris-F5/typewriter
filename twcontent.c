/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdlib.h>

#include "utils.h"
#include "twpdf.h"
#include "twcontent.h"

void
pdf_content_init(struct pdf_content *content)
{
}

void
pdf_content_free(struct pdf_content *content)
{
}

void
pdf_content_define(struct pdf *pdf, struct pdf_obj_indirect *ref,
    struct pdf_content *content)
{
  long allocated, size;
  char *bytes;
  allocated = 1024;
  size = 0;
  bytes = xmalloc(allocated);
  resprintf(&bytes, &allocated, &size, "BT\n");
  resprintf(&bytes, &allocated, &size, "/F0 12 Tf\n");
  resprintf(&bytes, &allocated, &size, "1 0 0 1 %d %d Tm\n", 100, 100);
  resprintf(&bytes, &allocated, &size, "(hello world) Tj\n");
  resprintf(&bytes, &allocated, &size, "ET\n");
  pdf_define_stream(pdf, ref, size, bytes);
}
