/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

/*
 * The following must be included before this file:
#include "twpdf.h"
 */

enum pdf_content_mode {
  PDF_CONTENT_MODE_PAGE = 0,
  PDF_CONTENT_MODE_TEXT = 1,
};

struct pdf_content {
  long allocated, length;
  char *bytes;
  int mode;
  int font_size;
};

void pdf_content_init(struct pdf_content *content);
void pdf_content_reset_page(struct pdf_content *content);
void pdf_content_free(struct pdf_content *content);

void pdf_content_write_text(struct pdf_content *content, const char *string,
    int x, int y, int size);
void pdf_content_write_image(struct pdf_content *content, const char *name,
    int x, int y, int w, int h);

void pdf_content_define(struct pdf *pdf, struct pdf_obj_indirect *ref,
    struct pdf_content *content);
struct pdf_obj *pdf_content_create_resources(struct pdf *pdf, struct pdf_obj_dictionary *xobjects);
