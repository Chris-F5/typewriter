/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

/*
 * The following must be included before this file:
#include "twpdf.h"
 */

struct pdf_pages {
  int page_allocated, page_count;
  struct pdf_obj **page_objs;
  struct pdf_obj_indirect *pages_parent_ref;
};

void pdf_pages_init(struct pdf *pdf, struct pdf_pages *pages);
void pdf_pages_free(struct pdf_pages *pages);

void pdf_pages_add_page(struct pdf *pdf, struct pdf_pages *pages,
    struct pdf_obj_indirect *content);
void pdf_pages_define_catalogue(struct pdf *pdf, struct pdf_obj_indirect *ref,
    struct pdf_pages *pages);
