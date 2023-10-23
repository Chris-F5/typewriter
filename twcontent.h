/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

/*
 * The following must be included before this file:
#include "twpdf.h"
 */

struct pdf_content {
};

void pdf_content_init(struct pdf_content *content);
void pdf_content_free(struct pdf_content *content);

void pdf_content_define(struct pdf *pdf, struct pdf_obj_indirect *ref,
    struct pdf_content *content);
struct pdf_obj *pdf_content_create_resources(struct pdf *pdf);
