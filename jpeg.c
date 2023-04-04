/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdio.h>
#include <stdint.h>
#include "tw.h"

static uint16_t read_uint16(FILE *file);
static void skip_segment_payload(FILE *file);

static uint16_t
read_uint16(FILE *file)
{
  uint16_t n;
  n = 0;
  /* only words on little-edian machines */
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

int
read_jpeg(FILE *file, struct jpeg_info *info)
{
  unsigned char magic_bytes[2];
  unsigned char segment_code[2];

  fread(magic_bytes, 1, 2, file);
  if (magic_bytes[0] != 0xff || magic_bytes[1] != 0xd8) {
    fprintf(stderr, "File not JPEG.\n");
    return 1;
  }

  for(;;) {
    fread(segment_code, 1, 2, file);
    if (segment_code[0] != 0xFF) {
      fprintf(stderr, "JPEG file invalid.\n");
      return 1;
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
        fprintf(stderr, "Progressive DCT JPEGs are unsupported.\n");
        return 1;
      case 0xda: /* compressed image data */
        /*
         * If we get to the image data and have not found width and height yet
         * then assume we will not find it.
         */
        fprintf(stderr, "JPEG image data reached and no DCT segment found.\n");
        return 1;
      case 0xd9: /* end of image marker */
        fprintf(stderr, "End of JPEG reached and no DCT segment found.\n");
        return 1;
      default:
        fprintf(stderr, "Unrecognised JPEG segment code '%02x %02x'\n",
            segment_code[0], segment_code[1]);
        return 1;
      }
    }
    if (ferror(file) || feof(file)) {
      fprintf(stderr, "Error reading JPEG file.\n");
      return 1;
    }
  }
end_scan:
  if (ferror(file) || feof(file)) {
    fprintf(stderr, "Error reading JPEG file.\n");
    return 1;
  }
  if (info->components != 1 && info->components != 3) {
    fprintf(stderr, "JPEG file has unsupported color component count '%d'.\n",
        info->components);
    return 1;
  }
  return 0;
}
