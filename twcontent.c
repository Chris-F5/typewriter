/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdlib.h>
#include <stdarg.h>

#include "utils.h"
#include "twpdf.h"
#include "twcontent.h"

void
pdf_content_init(struct pdf_content *content)
{
  content->allocated = 1024;
  content->length = 0;
  content->bytes = xmalloc(content->allocated);
}

void
pdf_content_free(struct pdf_content *content)
{
  free(content->bytes);
}

void
pdf_content_write(struct pdf_content *content, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  revsprintf(&content->bytes, &content->allocated, &content->length, format, args);
  va_end(args);
}

void
pdf_content_define(struct pdf *pdf, struct pdf_obj_indirect *ref,
    struct pdf_content *content)
{
  pdf_define_stream(pdf, ref, content->length, content->bytes);
  content->bytes = NULL; /* Pass ownership to pdf. */
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
