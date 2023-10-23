/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */


#include <stdlib.h>

#include "utils.h"
#include "twpdf.h"
#include "twpages.h"

void 
pdf_pages_init(struct pdf *pdf, struct pdf_pages *pages)
{
  pages->page_allocated = 64;
  pages->page_count = 0;
  pages->page_objs = xmalloc(pages->page_allocated * sizeof(void *));
  pages->pages_parent_ref = pdf_allocate_indirect_obj(pdf);
}

void 
pdf_pages_free(struct pdf_pages *pages)
{
  free(pages->page_objs);
}


void 
pdf_pages_add_page(struct pdf *pdf, struct pdf_pages *pages,
    struct pdf_obj_indirect *content)
{
  struct pdf_obj_dictionary *page;
  page = pdf_create_dictionary(pdf);
  page = pdf_prepend_dictionary(pdf, page, "Type",
      (struct pdf_obj *)pdf_create_name(pdf, "Page"));
  page = pdf_prepend_dictionary(pdf, page, "Parent",
      (struct pdf_obj *)pages->pages_parent_ref);
  page = pdf_prepend_dictionary(pdf, page, "Contents",
      (struct pdf_obj *)content);
  if (pages->page_count == pages->page_allocated) {
    pages->page_allocated += 64;
    pages->page_objs = xrealloc(pages->page_objs, pages->page_allocated * sizeof(void *));
  }
  pages->page_objs[pages->page_count++] = (struct pdf_obj *)page;
}

void
pdf_pages_define_catalogue(struct pdf *pdf, struct pdf_obj_indirect *ref,
    struct pdf_pages *pages, struct pdf_obj *resources)
{
  int i;
  struct pdf_obj_indirect *page_ref;
  struct pdf_obj_array *pages_array, *media_box;
  struct pdf_obj_dictionary *pages_parent, *catalogue;

  media_box = pdf_create_array(pdf);
  media_box = pdf_prepend_array(pdf, media_box,
      (struct pdf_obj *)pdf_create_integer(pdf, 842));
  media_box = pdf_prepend_array(pdf, media_box,
      (struct pdf_obj *)pdf_create_integer(pdf, 595));
  media_box = pdf_prepend_array(pdf, media_box,
      (struct pdf_obj *)pdf_create_integer(pdf, 0));
  media_box = pdf_prepend_array(pdf, media_box,
      (struct pdf_obj *)pdf_create_integer(pdf, 0));

  pages_array = pdf_create_array(pdf);
  for (i = pages->page_count - 1; i >= 0; i--) {
    page_ref = pdf_allocate_indirect_obj(pdf);
    pdf_define_obj(pdf, page_ref, pages->page_objs[i], 0);
    pages_array = pdf_prepend_array(pdf, pages_array, (struct pdf_obj *)page_ref);
  }

  pages_parent = pdf_create_dictionary(pdf);
  pages_parent = pdf_prepend_dictionary(pdf, pages_parent, "Type",
      (struct pdf_obj *)pdf_create_name(pdf, "Pages"));
  pages_parent = pdf_prepend_dictionary(pdf, pages_parent, "Kids",
      (struct pdf_obj *)pages_array);
  pages_parent = pdf_prepend_dictionary(pdf, pages_parent, "Count",
      (struct pdf_obj *)pdf_create_integer(pdf, pages->page_count));
  pages_parent = pdf_prepend_dictionary(pdf, pages_parent, "Resources",
      resources);
  pages_parent = pdf_prepend_dictionary(pdf, pages_parent, "MediaBox",
      (struct pdf_obj *)media_box);

  catalogue = pdf_create_dictionary(pdf);
  catalogue = pdf_prepend_dictionary(pdf, catalogue, "Type",
      (struct pdf_obj *)pdf_create_name(pdf, "Catalog"));
  catalogue = pdf_prepend_dictionary(pdf, catalogue, "Pages",
      (struct pdf_obj *)pages->pages_parent_ref);

  pdf_define_obj(pdf, pages->pages_parent_ref, (struct pdf_obj *)pages_parent, 0);
  pdf_define_obj(pdf, ref, (struct pdf_obj *)catalogue, 1);
}
