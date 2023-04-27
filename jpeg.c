/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

/*
 * This file parses JPEG files. See 'https://en.wikipedia.org/wiki/JPEG' for 
 * file format information.
 */

#include <stdio.h>
#include <stdint.h>
#include "tw.h"

static uint16_t read_uint16(FILE *file);
static void skip_segment_payload(FILE *file);

/* Read big-endian 16-bit unsigned integer from file. */
static uint16_t
read_uint16(FILE *file)
{
  uint16_t n;
  n = 0;
  /* Copy each byte individually and reverse order. */
  fread(1 + (char *)&n, 1, 1, file);
  fread((char *)&n, 1, 1, file);
  return n;
}

/*
 * Skips a jpeg segment by reading segment length and seeking forward that
 * amount.
 * */
static void
skip_segment_payload(FILE *file)
{
  uint16_t segment_length;
  segment_length = read_uint16(file);
  /* Subtract two because length does not include first two bytes. */
  fseek(file, segment_length - 2, SEEK_CUR);
}

/* Parse a jpeg file and write the file info to _info_ */
int
read_jpeg(FILE *file, struct jpeg_info *info)
{
  unsigned char magic_bytes[2];
  unsigned char segment_code[2];

  /* Read the first two bytes from the file. */
  fread(magic_bytes, 1, 2, file);
  /* jpegs always start with ffd8. The 'magic bytes'. */
  if (magic_bytes[0] != 0xff || magic_bytes[1] != 0xd8) {
    fprintf(stderr, "File not JPEG.\n");
    return 1;
  }

  /* For each segment in jpeg file. */
  for(;;) {
    /* Read the two byte segment code. */
    fread(segment_code, 1, 2, file);
    /* Segment codes always start with the byte 0xff. */
    if (segment_code[0] != 0xFF) {
      fprintf(stderr, "JPEG file invalid.\n");
      return 1;
    }
    /* These segments are reserved for application specific segments. */
    if (segment_code[1] >= 0xe0 && segment_code[1] <= 0xef) {
      /* Skip application specific segment. */
      skip_segment_payload(file);
    } else {
      /* Handle this segment accordingly. */
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
        /* When we find this segment we stop because this is all we want. */
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
        /* If we did not handle this segment code. */
        fprintf(stderr, "Unrecognised JPEG segment code '%02x %02x'\n",
            segment_code[0], segment_code[1]);
        return 1;
      }
    }
    /* If we are at the end of the file or if there was an error reading. */
    if (ferror(file) || feof(file)) {
      fprintf(stderr, "Error reading JPEG file.\n");
      return 1;
    }
  }
end_scan:
  /* If we are at the end of the file or if there was an error reading. */
  if (ferror(file) || feof(file)) {
    fprintf(stderr, "Error reading JPEG file.\n");
    return 1;
  }
  /* PDF only supports JPEGS with rgb (3) or greyscale (1) colorspace. */
  if (info->components != 1 && info->components != 3) {
    fprintf(stderr, "JPEG file has unsupported color component count '%d'.\n",
        info->components);
    return 1;
  }
  return 0;
}
