/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
#include "twpdf.h"
#include "twjpeg.h"

static uint16_t read_uint16(FILE *file);
static void skip_segment_payload(FILE *file);
static void read_pdf_info(FILE *file, const char *fname, struct pdf_jpeg_info *info);

static uint16_t
read_uint16(FILE *file)
{
  uint16_t n;
  n = 0;
  fread(1 + (char *)&n, 1, 1, file);
  fread((char *)&n, 1, 1, file);
  return n;
}

static void
skip_segment_payload(FILE *file)
{
  uint16_t segment_length;
  segment_length = read_uint16(file);
  fseek(file, segment_length - 2, SEEK_CUR);
}

static void
read_pdf_info(FILE *file, const char *fname, struct pdf_jpeg_info *info)
{
  unsigned char magic_bytes[2];
  unsigned char segment_code[2];

  fread(magic_bytes, 1, 2, file);
  if (magic_bytes[0] != 0xff || magic_bytes[1] != 0xd8) {
    fprintf(stderr, "twpdf: Not a JPEG file %s.\n", fname);
    exit(1);
  }

  for(;;) {
    fread(segment_code, 1, 2, file);
    if (segment_code[0] != 0xFF) {
      fprintf(stderr, "twpdf: JPEG file invalid %s.\n", fname);
      exit(1);
    }
    if (segment_code[1] >= 0xe0 && segment_code[1] <= 0xef) {
      /*
       * Application specific segment.
       * Just hope that the first two bytes specify length of this segment.
       */
      skip_segment_payload(file);
    } else {
      switch (segment_code[1]) {
      case 0xfe: /* comment marker */
      case 0xdb: /* quantization tables */
      case 0xc4: /* huffman tables */
        skip_segment_payload(file);
        break;
      case 0xc0: /* baseline DCT segment (contains image width and height) */
        fseek(file, 3, SEEK_CUR);
        info->height = read_uint16(file);
        info->width = read_uint16(file);
        fread(&info->components, 1, 1, file);
        goto end_scan;
      case 0xc2: /* progressive DCT segment (unsupported) */
        fprintf(stderr, "twpdf: Progressive DCT JPEGs are not supported %s.\n", fname);
        exit(1);
      case 0xda: /* compressed image data */
        /*
         * If we get to the image data and have not found width and height yet
         * then assume we will not find it.
         */
        fprintf(stderr, "twpdf: JPEG image data reached and no DCT segment found in %s.\n", fname);
        exit(1);
      case 0xd9: /* end of image marker */
        fprintf(stderr, "twpdf: End of JPEG reached and no DCT segment found in %s.\n", fname);
        exit(1);
      default:
        fprintf(stderr, "twpdf: Unrecognised JPEG segment code '%02x %02x' in %s.\n",
            segment_code[0], segment_code[1], fname);
        exit(1);
      }
    }
    if (ferror(file) || feof(file)) {
      fprintf(stderr, "twpdf: Error reading JPEG file %s.\n", fname);
      exit(1);
    }
  }
end_scan:
  if (ferror(file) || feof(file)) {
    fprintf(stderr, "twpdf: Error reading JPEG file %s.\n", fname);
    exit(1);
  }
  if (info->components != 1 && info->components != 3) {
    fprintf(stderr, "twpdf: JPEG file has unsupported color component count '%d' in %s.\n",
        info->components, fname);
    exit(1);
  }
}

struct pdf_obj_indirect *
pdf_jpeg_define(struct pdf *pdf, const char *fname, struct pdf_jpeg_info *info)
{
  FILE *file;
  struct pdf_obj_array *filters;
  struct pdf_obj_indirect *ref;
  struct pdf_obj_dictionary *dictionary;
  const char *color_space;
  long length;
  char *bytes;

  file = fopen(fname, "r");
  if (file == NULL) {
    fprintf(stderr, "twpdf: Failed to open JPEG file %s.\n", fname);
    exit(1);
  }
  read_pdf_info(file, fname, info);
  fseek(file, 0, SEEK_END);
  length = ftell(file);
  fseek(file, 0, SEEK_SET);
  bytes = xmalloc(length);
  fread(bytes, 1, length, file);

  color_space = info->components == 3 ? "DeviceRGB" : "DeviceGray";
  filters = pdf_create_array(pdf);
  filters = pdf_prepend_array(pdf, filters,
      (struct pdf_obj *)pdf_create_name(pdf, "DCTDecode"));
  dictionary = pdf_create_dictionary(pdf);
  dictionary = pdf_prepend_dictionary(pdf, dictionary, "Type",
      (struct pdf_obj *)pdf_create_name(pdf, "XObject"));
  dictionary = pdf_prepend_dictionary(pdf, dictionary, "Subtype",
      (struct pdf_obj *)pdf_create_name(pdf, "Image"));
  dictionary = pdf_prepend_dictionary(pdf, dictionary, "Width",
      (struct pdf_obj *)pdf_create_integer(pdf, info->width));
  dictionary = pdf_prepend_dictionary(pdf, dictionary, "Height",
      (struct pdf_obj *)pdf_create_integer(pdf, info->height));
  dictionary = pdf_prepend_dictionary(pdf, dictionary, "ColorSpace",
      (struct pdf_obj *)pdf_create_name(pdf, color_space));
  dictionary = pdf_prepend_dictionary(pdf, dictionary, "BitsPerComponent",
      (struct pdf_obj *)pdf_create_integer(pdf, 8));
  ref = pdf_allocate_indirect_obj(pdf);
  pdf_define_stream(pdf, ref, dictionary, filters, length, bytes);
  return ref;
}
