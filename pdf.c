/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "tw.h"

void
pdf_write_header(FILE *file)
{
  fprintf(file, "%%PDF-1.7\n");
}

void
pdf_start_indirect_obj(FILE *file, struct pdf_xref_table *xref, int obj)
{
  xref->obj_offsets[obj] = ftell(file);
  fprintf(file, "%d 0 obj\n", obj);
}

void
pdf_end_indirect_obj(FILE *file)
{
  fprintf(file, "endobj\n");
}

void
pdf_write_file_stream(FILE *pdf_file, FILE *data_file)
{
  long size, i;
  fseek(data_file, 0, SEEK_END);
  size = ftell(data_file);
  fseek(data_file, 0, SEEK_SET);
  fprintf(pdf_file, "<<\n\
  /Filter /ASCIIHexDecode\n\
  /Length %ld\n\
  /Length1 %ld\n\
  >>\nstream\n", size * 2, size);
  for (i = 0; i < size; i++)
    fprintf(pdf_file, "%02x", (unsigned char)fgetc(data_file));
  fprintf(pdf_file, "\nendstream\n");
}

void
pdf_write_text_stream(FILE *file, const char *data, long size)
{
  fprintf(file, "<< /Length %ld >> stream\n", size);
  fwrite(data, 1, size, file);
  fprintf(file, "\nendstream\n");
}

void
pdf_write_int_array(FILE *file, const int *values, int count)
{
  int i;
  fprintf(file, "[\n ");
  for (i = 0; i < count; i++)
    fprintf(file, " %d", values[i]);
  fprintf(file, "\n]\n");
}

void
pdf_write_font_descriptor(FILE *file, int font_file, const char *font_name,
    int italic_angle, int ascent, int descent, int cap_height,
    int stem_vertical, int min_x, int min_y, int max_x, int max_y)
{
  fprintf(file, "<<\n\
  /Type /FontDescriptor\n\
  /FontName /%s\n\
  /FontFile2 %d 0 R\n\
  /Flags 6\n\
  /FontBBox [%d, %d, %d, %d]\n\
  /ItalicAngle %d\n\
  /Ascent %d\n\
  /Descent %d\n\
  /CapHeight %d\n\
  /StemV %d\n\
>>\n", font_name, font_file, min_x, min_y, max_x, max_y, italic_angle,
      ascent, descent, cap_height, stem_vertical);
}

void
pdf_write_page(FILE *file, int parent, int content)
{
  fprintf(file, "<<\n\
  /Type /Page\n\
  /Parent %d 0 R\n\
  /Contents %d 0 R\n\
>>\n", parent, content);
}

void
pdf_write_font(FILE *file, const char *font_name, int font_descriptor,
    int font_widths)
{
  fprintf(file, "<<\n\
  /Type /Font\n\
  /Subtype /TrueType\n\
  /BaseFont /%s\n\
  /FirstChar 0\n\
  /LastChar 255\n\
  /Widths %d 0 R\n\
  /FontDescriptor %d 0 R\n\
>>\n", font_name, font_widths, font_descriptor);
}

void
pdf_write_pages(FILE *file, int resources, int page_count, const int *page_objs)
{
  int i;
  fprintf(file, "<<\n\
  /Type /Pages\n\
  /Resources %d 0 R\n\
  /Kids [\n", resources);
  for (i = 0; i < page_count; i++)
    fprintf(file, "    %d 0 R\n", page_objs[i]);
  /* 595x842 is a portrait A4 page. */
  fprintf(file, "    ]\n\
  /Count %d\n\
  /MediaBox [0 0 595 842]\n\
>>\n", page_count);
}

void
pdf_write_catalog(FILE *file, int page_list)
{
  fprintf(file, "<<\n\
  /Type /Catalog\n\
  /Pages %d 0 R\n\
>>\n", page_list);
}

void
init_pdf_xref_table(struct pdf_xref_table *xref)
{
  xref->obj_count = 1;
  xref->allocated = 100;
  xref->obj_offsets = xmalloc(xref->allocated * sizeof(long));
  memset(xref->obj_offsets, 0, xref->allocated * sizeof(long));
}

int
allocate_pdf_obj(struct pdf_xref_table *xref)
{
  if (xref->obj_count == xref->allocated) {
    xref->obj_offsets
      = xrealloc(xref->obj_offsets, (xref->allocated + 100) * sizeof(long));
    memset(xref->obj_offsets + xref->allocated, 0,
        (xref->allocated + 100) * sizeof(long));
    xref->allocated += 100;
  }
  return xref->obj_count++;
}

void
pdf_add_footer(FILE *file, const struct pdf_xref_table *xref, int root_obj)
{
  int xref_offset, i;
  xref_offset = ftell(file);
  fprintf(file, "xref\n\
0 %d\n\
000000000 65535 f \n", xref->obj_count);
  for (i = 1; i < xref->obj_count; i++)
    fprintf(file, "%09ld 00000 n \n", xref->obj_offsets[i]);
  fprintf(file, "trailer << /Size %d /Root %d 0 R >>\n\
startxref\n\
%d\n\
%%%%EOF", xref->obj_count, root_obj, xref_offset);
}

void
free_pdf_xref_table(struct pdf_xref_table *xref)
{
  free(xref->obj_offsets);
}

void
init_pdf_resources(struct pdf_resources *resources)
{
  init_record(&resources->fonts_used);
}

void
include_font_resource(struct pdf_resources *resources, const char *font)
{
  int i;
  for (i = 0; i < resources->fonts_used.field_count; i++)
    if (strcmp(resources->fonts_used.fields[i], font) == 0)
      return;
  begin_field(&resources->fonts_used);
  dbuffer_printf(&resources->fonts_used.string, "%s", font);
  dbuffer_putc(&resources->fonts_used.string, '\0');
}

static int
pdf_add_font(FILE *pdf_file, FILE *font_file, struct pdf_xref_table *xref,
    const char *name)
{
  struct font_info font_info;
  int font_program, font_widths, font_descriptor, font;
  font_program = allocate_pdf_obj(xref);
  font_widths = allocate_pdf_obj(xref);
  font_descriptor = allocate_pdf_obj(xref);
  font = allocate_pdf_obj(xref);

  fseek(font_file, 0, SEEK_SET);
  if (read_ttf(font_file, &font_info))
    return -1;
  fseek(font_file, 0, SEEK_SET);
  pdf_start_indirect_obj(pdf_file, xref, font_program);
  pdf_write_file_stream(pdf_file, font_file);;
  pdf_end_indirect_obj(pdf_file);

  pdf_start_indirect_obj(pdf_file, xref, font_widths);
  pdf_write_int_array(pdf_file, font_info.char_widths, 256);
  pdf_end_indirect_obj(pdf_file);

  pdf_start_indirect_obj(pdf_file, xref, font_descriptor);
  pdf_write_font_descriptor(pdf_file, font_program, name, -10, 255,
      255, 255, 10, font_info.x_min, font_info.y_min, font_info.x_max,
      font_info.y_max);
  pdf_end_indirect_obj(pdf_file);

  pdf_start_indirect_obj(pdf_file, xref, font);
  pdf_write_font(pdf_file, name, font_descriptor, font_widths);
  pdf_end_indirect_obj(pdf_file);
  return font;
}

void
pdf_add_resources(FILE *pdf_file, FILE *typeface_file, int resources_obj,
    const struct pdf_resources *resources, struct pdf_xref_table *xref)
{
  FILE *font_file;
  struct record typeface_record;
  int i, parse_result;
  int *font_objs;
  init_record(&typeface_record);
  font_objs = xmalloc(resources->fonts_used.field_count * sizeof(int));
  for (i = 0; i < resources->fonts_used.field_count; i++)
    font_objs[i] = -1;
  while ( (parse_result = parse_record(typeface_file, &typeface_record))
      != EOF ) {
    if (parse_result)
      continue;
    if (typeface_record.field_count != 2) {
      fprintf(stderr, "Typeface records must have exactly 2 fields.");
      continue;
    }
    if (strlen(typeface_record.fields[0]) >= 256) {
      fprintf(stderr,
          "Typeface file contains font name that is too long '%s'.\n",
          typeface_record.fields[0]);
      continue;
    }
    if (!is_font_name_valid(typeface_record.fields[0])) {
      fprintf(stderr, "Typeface file contains invalid font name '%s'.\n",
          typeface_record.fields[0]);
      continue;
    }
    i = find_field(&resources->fonts_used, typeface_record.fields[0]);
    if (i == -1)
      continue;
    font_file = fopen(typeface_record.fields[1], "r");
    if (font_file == NULL) {
      fprintf(stderr, "Failed to open ttf file '%s': %s\n",
          typeface_record.fields[1], strerror(errno));
      continue;
    }
    font_objs[i] = pdf_add_font(pdf_file, font_file, xref,
        typeface_record.fields[0]);
    if (font_objs[i] == -1) {
      fprintf(stderr, "Failed to parse ttf file '%s'\n",
          typeface_record.fields[1]);
    }
    fclose(font_file);
  }
  free_record(&typeface_record);

  pdf_start_indirect_obj(pdf_file, xref, resources_obj);
  fprintf(pdf_file, "<<\n  /Font <<\n");
  for (i = 0; i < resources->fonts_used.field_count; i++) {
    if (font_objs[i] == -1) {
      fprintf(stderr, "Typeface file does not include '%s' font.\n",
          resources->fonts_used.fields[i]);
    }
    fprintf(pdf_file, "    /%s %d 0 R\n", resources->fonts_used.fields[i],
        font_objs[i]);
  }
  fprintf(pdf_file, "  >>\n>>\n");
  pdf_end_indirect_obj(pdf_file);
}

void
free_pdf_resources(struct pdf_resources *resources)
{
  free_record(&resources->fonts_used);
}

void
init_pdf_page_list(struct pdf_page_list *page_list)
{
  page_list->page_count = 0;
  page_list->pages_allocated = 100;
  page_list->page_objs = xmalloc(page_list->pages_allocated * sizeof(int));
}

void
add_pdf_page(struct pdf_page_list *page_list, int page)
{
  if (page_list->page_count == page_list->pages_allocated) {
    page_list->pages_allocated += 100;
    page_list->page_objs = xrealloc(page_list->page_objs,
        page_list->pages_allocated);
  }
  page_list->page_objs[page_list->page_count++] = page;
}

void
free_pdf_page_list(struct pdf_page_list *page_list)
{
  free(page_list->page_objs);
}
