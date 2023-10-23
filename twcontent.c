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

struct pdf_obj *
pdf_content_create_resources(struct pdf *pdf)
{
  struct pdf_obj_dictionary *resources, *font_resources, *helvetica;
  helvetica = pdf_create_dictionary(pdf);
  helvetica = pdf_prepend_dictionary(pdf, helvetica, "Type",
      (struct pdf_obj *)pdf_create_name(pdf, "Font"));
  helvetica = pdf_prepend_dictionary(pdf, helvetica, "Subtype",
      (struct pdf_obj *)pdf_create_name(pdf, "Type1"));
  helvetica = pdf_prepend_dictionary(pdf, helvetica, "BaseFont",
      (struct pdf_obj *)pdf_create_name(pdf, "Helvetica"));
  font_resources = pdf_create_dictionary(pdf);
  font_resources = pdf_prepend_dictionary(pdf, font_resources, "F0",
      (struct pdf_obj *)helvetica);
  resources = pdf_create_dictionary(pdf);
  resources = pdf_prepend_dictionary(pdf, resources, "Font",
      (struct pdf_obj *)font_resources);
  return (struct pdf_obj *)resources;
}
