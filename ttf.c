/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

/*
 * See Apple's TrueType reference manual for file format info:
 * https://developer.apple.com/fonts/TrueType-Reference-Manual/
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "tw.h"

#define REQUIRED_TABLE_COUNT \
  (sizeof(required_tables) / sizeof(required_tables[0]))

static int16_t read_int16(const char *ptr);
static int32_t read_int32(const char *ptr);
static int read_head_table(
    const char *table, long table_size, struct font_info *info);
static int read_format4_cmap_subtable(
    const char *table, long table_size, struct font_info *info);
static int read_cmap_table(
    const char *table, long table_size, struct font_info *info);
static int read_hhea_table(
    const char *table, long table_size, struct font_info *info);
static int read_hmtx_table(
    const char *table, long table_size, struct font_info *info);

static const char *required_tables[] = {"head", "cmap", "hhea", "hmtx"};
static int (*required_table_parsers[REQUIRED_TABLE_COUNT])
  (const char *table, long table_size, struct font_info *font) = {
  read_head_table,
  read_cmap_table,
  read_hhea_table,
  read_hmtx_table,
};

/* Convert big-edian to little-endian. Assume this machine is little-endian. */
static int16_t
read_int16(const char *ptr)
{
  uint16_t ret;
  ((char *)&ret)[0] = ptr[1];
  ((char *)&ret)[1] = ptr[0];
  return ret;
}

static int32_t
read_int32(const char *ptr)
{
  uint32_t ret;
  ((char *)&ret)[0] = ptr[3];
  ((char *)&ret)[1] = ptr[2];
  ((char *)&ret)[2] = ptr[1];
  ((char *)&ret)[3] = ptr[0];
  return ret;
}

static int
read_head_table(const char *table, long table_size, struct font_info *info)
{
  if (table_size != 54)
    return 1;
  info->units_per_em = read_int16(table + 18);
  info->x_min = read_int16(table + 36) * 1000 / info->units_per_em;
  info->y_min = read_int16(table + 38) * 1000 / info->units_per_em;
  info->x_max = read_int16(table + 40) * 1000 / info->units_per_em;
  info->y_max = read_int16(table + 42) * 1000 / info->units_per_em;
  return 0;
}

static int
read_format4_cmap_subtable(
    const char *table,
    long table_size,
    struct font_info *info)
{
  unsigned int range_index, char_index;
  uint16_t seg_count;
  const char *end_codes, *start_codes, *deltas, *offsets;
  if (table_size < 16)
    return 1;
  seg_count = read_int16(table + 6) / 2;
  end_codes = table + 14;
  start_codes = end_codes + seg_count * 2 + 2;
  deltas = start_codes + seg_count * 2;
  offsets = deltas + seg_count * 2;
  if (table_size < 16 + seg_count * 8)
    return 1;
  for (char_index = 0; char_index < 256; char_index++)
    info->cmap[char_index] = 0;
  for (range_index = 0; range_index < seg_count; range_index++) {
    uint16_t start, end, glyph_delta, glyph_offset;
    end = read_int16(end_codes + range_index * 2);
    start = read_int16(start_codes + range_index * 2);
    if (end > 255) end = 255;
    if (start > 255) break;
    glyph_delta = read_int16(deltas + range_index * 2);
    glyph_offset = read_int16(offsets + range_index * 2);
    if (glyph_offset == 0) {
      for (char_index = start; char_index <= end; char_index++)
        info->cmap[char_index] = char_index + glyph_delta;
    } else {
      if(offsets + range_index * 2 + glyph_offset 
          + 2 * (end - start) + 2
          > table + table_size)
        return 1;
      for (char_index = start; char_index <= end; char_index++)
        info->cmap[char_index] = read_int16(
          offsets + range_index * 2
          + glyph_offset
          + 2 * (char_index - start));
    }
  }
  return 0;
}

static int
read_cmap_table(const char *table, long table_size, struct font_info *info)
{
  uint16_t subtable_count, format, subtable_size;
  const char *subtable;
  if (table_size < 4)
    goto invalid;
  subtable_count = read_int16(table + 2);
  if (table_size < 4 + subtable_count * 8)
    goto invalid;
  for (subtable = table + 4;
      subtable < table + 4 + subtable_count * 8;
      subtable += 8) {
    uint16_t platform_id, platform_specific_id;
    uint32_t offset;
    platform_id = read_int16(subtable);
    platform_specific_id = read_int16(subtable + 2);
    offset = read_int32(subtable + 4);
    if (platform_id == 0 && platform_specific_id <= 4) {
      subtable = table + offset;
      goto found_cmap;
    }
  }
  goto unsupported;
found_cmap:
  if (subtable + 4 > table + table_size)
    goto invalid;
  format = read_int16(subtable);
  subtable_size = read_int16(subtable + 2);
  if (subtable + subtable_size > table + table_size)
    goto invalid;
  if (format != 4)
    goto unsupported;
  if (read_format4_cmap_subtable(subtable, subtable_size, info))
    goto subtable_error;
  return 0;
invalid:
  fprintf(stderr, "ttf cmap table invalid\n");
  return 1;
unsupported:
  fprintf(stderr, "ttf cmap table not supported\n");
  return 1;
subtable_error:
  fprintf(stderr, "failed to read ttf cmap subtable\n");
  return 1;
}

static int
read_hhea_table(const char *table, long table_size, struct font_info *info)
{
  if (table_size != 36)
    return 1;
  info->long_hor_metrics_count = read_int16(table + 34);
  return 0;
}

static int
read_hmtx_table(const char *table, long table_size, struct font_info *info)
{
  int i;
  uint16_t fallback_tt_width;
  if (info->long_hor_metrics_count < 1)
    return 1;
  if (table_size < info->long_hor_metrics_count * 4)
    return 1;
  fallback_tt_width 
    = read_int16(table + 4 * (info->long_hor_metrics_count - 1));
  for (i = 0; i < 256; i++) {
    uint16_t glyph, tt_width;
    glyph = info->cmap[i];
    tt_width 
      = glyph >= info->long_hor_metrics_count
      ? fallback_tt_width
      : read_int16(table + 4 * glyph);
    info->char_widths[i] = tt_width * 1000 / info->units_per_em;
  }
  return 0;
}

int
read_ttf(FILE *file, struct font_info *info)
{
  /*
   * This file was originally written to parse the ttf data from a buffer. I
   * later decided to change the function definition to read straight from a
   * file descriptor. I couldn't be bothered to rewrite all the ttf parsing
   * code to read right from the file, so I added the following hack that reads
   * the file into a sufficiently large buffer and then the rest of the code
   * works as it did before.
   */
  char *ttf;
  long ttf_size;
  fseek(file, 0, SEEK_END);
  ttf_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  ttf = xmalloc(ttf_size);
  fread(ttf, 1, ttf_size, file);

  int i;
  uint32_t magic_bytes;
  uint16_t table_count;
  uint32_t required_table_offsets[REQUIRED_TABLE_COUNT];
  uint32_t required_table_lengths[REQUIRED_TABLE_COUNT];
  const char *table_directory;
  const char *table;

  if (ttf_size < 12)
    goto table_directory_invalid;
  magic_bytes = read_int32(ttf);
  if (magic_bytes != 0x00010000)
    goto not_ttf;
  table_count = read_int16(ttf + 4);
  table_directory = ttf + 12;
  if (ttf_size < 12 + table_count * 16)
    goto table_directory_invalid;
  for (i = 0; i < REQUIRED_TABLE_COUNT; i++)
    required_table_lengths[i] = 0;
  for (table = table_directory;
      table < table_directory + table_count * 16;
      table += 16) {
    char tag[4];
    uint32_t offset, length;
    memcpy(tag, table, 4);
    offset = read_int32(table + 8);
    length = read_int32(table + 12);
    for (i = 0; i < REQUIRED_TABLE_COUNT; i++)
      if (strcmp(tag, required_tables[i]) == 0) {
        required_table_offsets[i] = offset;
        required_table_lengths[i] = length;
      }
  }
  for (i = 0; i < REQUIRED_TABLE_COUNT; i++) {
    if (required_table_lengths[i] == 0)
      goto table_no_exist;
    if (required_table_parsers[i](ttf + required_table_offsets[i],
          required_table_lengths[i], info))
      goto table_error;
  }
  free(ttf);
  return 0;
not_ttf:
  fprintf(stderr, "failed to read ttf: not a ttf\n");
  free(ttf);
  return 1;
table_directory_invalid:
  fprintf(stderr, "ttf table directory invalid\n");
  free(ttf);
  return 1;
table_no_exist:
  fprintf(stderr, "ttf does not have %s table\n", required_tables[i]);
  free(ttf);
  return 1;
table_error:
  fprintf(stderr, "failed to read ttf %s table\n", required_tables[i]);
  free(ttf);
  return 1;
}
