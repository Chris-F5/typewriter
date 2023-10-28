/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdlib.h>
#include <stdarg.h>

#include "utils.h"
#include "twpdf.h"
#include "twcontent.h"

static void write_content(struct pdf_content *content, const char *format, ...);

static void
write_content(struct pdf_content *content, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  revsprintf(&content->bytes, &content->allocated, &content->length, format, args);
  va_end(args);
}

static void
switch_mode(struct pdf_content *content, int mode)
{
  if (content->mode == PDF_CONTENT_MODE_PAGE && mode == PDF_CONTENT_MODE_TEXT) {
    write_content(content, "BT\n");
    write_content(content, "/F0 %d Tf\n", content->font_size);
  } else if (content->mode == PDF_CONTENT_MODE_TEXT && mode == PDF_CONTENT_MODE_PAGE) {
    write_content(content, "ET\n");
  }
  content->mode = mode;
}

static void
switch_font_size(struct pdf_content *content, int size) {
  if (content->font_size != size) {
    content->font_size = size;
    if (content->mode == PDF_CONTENT_MODE_TEXT)
      write_content(content, "/F0 %d Tf\n", content->font_size);
  }
}

static void
escaped_string(struct pdf_content *content, const char *string)
{
  write_content(content, "(");
  for (; *string; string++) switch (*string) {
  case '(':
  case ')':
  case '\\':
    write_content(content, "\\");
  default:
    write_content(content, "%c", *string);
  }
  write_content(content, ")");
}

void
pdf_content_init(struct pdf_content *content)
{
  content->allocated = 1024;
  content->length = 0;
  content->bytes = xmalloc(content->allocated);
  content->mode = PDF_CONTENT_MODE_PAGE;
}

void
pdf_content_reset_page(struct pdf_content *content)
{
  free(content->bytes);
  content->allocated = 1024;
  content->length = 0;
  content->bytes = xmalloc(content->allocated);
  content->mode = PDF_CONTENT_MODE_PAGE;
}

void
pdf_content_free(struct pdf_content *content)
{
  free(content->bytes);
}

void
pdf_content_write_text(struct pdf_content *content, const char *string, int x,
    int y, int size)
{
  switch_mode(content, PDF_CONTENT_MODE_TEXT);
  switch_font_size(content, size);
  write_content(content, "1 0 0 1 %d %d Tm\n", x, y);
  escaped_string(content, string);
  write_content(content, " Tj\n");
}

void
pdf_content_write_image(struct pdf_content *content, const char *name, int x,
    int y, int w, int h)
{
  switch_mode(content, PDF_CONTENT_MODE_PAGE);
  write_content(content, "q\n");
  write_content(content, "%d 0 0 %d %d %d cm\n", w, h, x, y);
  /* TODO: escape name. */
  write_content(content, "/%s Do\n", name);
  write_content(content, "Q\n");
}

void
pdf_content_define(struct pdf *pdf, struct pdf_obj_indirect *ref,
    struct pdf_content *content)
{
  switch_mode(content, PDF_CONTENT_MODE_PAGE);
  pdf_define_stream(pdf, ref, pdf_create_dictionary(pdf),
      pdf_create_array(pdf), content->length, content->bytes);
  content->bytes = NULL; /* Pass ownership to pdf. */
}

struct pdf_obj *
pdf_content_create_resources(struct pdf *pdf,
    struct pdf_obj_dictionary *xobjects)
{
  struct pdf_obj_dictionary *resources, *font_resources, *helvetica;
  helvetica = pdf_create_dictionary(pdf);
  helvetica = pdf_prepend_dictionary(pdf, helvetica, "Type",
      (struct pdf_obj *)pdf_create_name(pdf, "Font"));
  helvetica = pdf_prepend_dictionary(pdf, helvetica, "Subtype",
      (struct pdf_obj *)pdf_create_name(pdf, "Type1"));
  helvetica = pdf_prepend_dictionary(pdf, helvetica, "BaseFont",
      (struct pdf_obj *)pdf_create_name(pdf, "Courier"));
  font_resources = pdf_create_dictionary(pdf);
  font_resources = pdf_prepend_dictionary(pdf, font_resources, "F0",
      (struct pdf_obj *)helvetica);
  resources = pdf_create_dictionary(pdf);
  resources = pdf_prepend_dictionary(pdf, resources, "Font",
      (struct pdf_obj *)font_resources);
  resources = pdf_prepend_dictionary(pdf, resources, "XObject",
      (struct pdf_obj *)xobjects);
  return (struct pdf_obj *)resources;
}
