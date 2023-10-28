/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

/*
 * The following must be included before this file:
#include "twpdf.h"
 */

struct pdf_jpeg_info {
  int width, height;
  unsigned char components;
};

struct pdf_obj_indirect *pdf_jpeg_define(struct pdf *pdf, const char *fname,
    struct pdf_jpeg_info *info);
