/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdio.h>
#include <string.h>

#include "tw.h"

struct text_content {
  int x, y, font_size, word_spacing;
  struct dbuffer buffer;
  char font_name[256];
};

static void add_page(FILE *pdf_file, int obj_parent,
    struct pdf_xref_table *xref, struct pdf_page_list *page_list,
    const struct dbuffer *text_content);
static void write_pdf_escaped_string(struct dbuffer *buffer,
    const char *string);
static int parse_text(FILE *input, int x, int y,
    struct text_content *text_content, struct pdf_resources *resources);
static int parse_graphic(FILE *input, int origin_x, int origin_y,
    struct text_content *text_content, struct pdf_resources *resources);

static struct record record;

static void
add_page(FILE *pdf_file, int obj_parent, struct pdf_xref_table *xref,
    struct pdf_page_list *page_list, const struct dbuffer *text_content)
{
  int obj_content, obj_page;
  obj_content = allocate_pdf_obj(xref);
  obj_page = allocate_pdf_obj(xref);

  pdf_start_indirect_obj(pdf_file, xref, obj_content);
  pdf_write_text_stream(pdf_file, text_content->data, text_content->size);
  pdf_end_indirect_obj(pdf_file);

  pdf_start_indirect_obj(pdf_file, xref, obj_page);
  pdf_write_page(pdf_file, obj_parent, obj_content);
  pdf_end_indirect_obj(pdf_file);

  add_pdf_page(page_list, obj_page);
}

static void
write_pdf_escaped_string(struct dbuffer *buffer, const char *string)
{
  dbuffer_putc(buffer, '(');
  while (*string) {
    switch (*string) {
      case '(':
      case ')':
      case '\\':
        dbuffer_putc(buffer, '\\');
      default:
        dbuffer_putc(buffer, *string);
    }
    string++;
  }
  dbuffer_putc(buffer, ')');
}

static int
parse_text(FILE *input, int x, int y, struct text_content *text_content,
    struct pdf_resources *resources)
{
  int parse_result, arg1, font_size, word_spacing;
  char font_name[256];
  font_size = 0;
  font_name[0] = '\0';
  word_spacing = 0;
  dbuffer_printf(&text_content->buffer, "1 0 0 1 %d %d Tm", x, y);
  for (;;) {
    parse_result = parse_record(input, &record);
    if (parse_result == EOF) {
      fprintf(stderr, "Text not ended before end of file.\n");
      return 1;
    }
    if (parse_result)
      continue;
    if (strcmp(record.fields[0], "END") == 0)
      break;
    if (strcmp(record.fields[0], "FONT") == 0) {
      if (record.field_count != 3) {
        fprintf(stderr, "Text FONT command must take 2 arguments.\n");
        continue;
      }
      if (strlen(record.fields[1]) >= 256) {
        fprintf(stderr, "Font name too long: '%s'\n", record.fields[1]);
        continue;
      }
      if (!is_font_name_valid(record.fields[1])) {
        fprintf(stderr, "Font name contains illegal characters: '%s'\n",
            record.fields[1]);
        continue;
      }
      if (str_to_int(record.fields[2], &arg1)) {
        fprintf(stderr, "Text FONT command's 2nd argument must be integer.\n");
        continue;
      }
      strcpy(font_name, record.fields[1]);
      font_size = arg1;
      include_font_resource(resources, font_name);
      continue;
    }
    if (strcmp(record.fields[0], "SPACE") == 0) {
      if (record.field_count != 2) {
        fprintf(stderr, "Text SPACE command must take 1 arguments.\n");
        continue;
      }
      if (str_to_int(record.fields[1], &arg1)) {
        fprintf(stderr, "Text SPACE command's argument must be integer.\n");
        continue;
      }
      word_spacing = arg1;
      continue;
    }
    if (strcmp(record.fields[0], "STRING") == 0) {
      if (record.field_count != 2) {
        fprintf(stderr, "Text STRING command must take 1 argument.\n");
        continue;
      }
      if (*font_name == '\0') {
        fprintf(stderr, "Text must specify font.\n");
        continue;
      }
      if (strcmp(text_content->font_name, font_name)
          || text_content->font_size != font_size) {
        dbuffer_printf(&text_content->buffer, " /%s %d Tf", font_name,
            font_size);
        strcpy(text_content->font_name, font_name);
        text_content->font_size = font_size;
      }
      if (word_spacing != text_content->word_spacing) {
        dbuffer_printf(&text_content->buffer, " %f Tw",
            (float)word_spacing / 1000);
        text_content->word_spacing = word_spacing;
      }
      dbuffer_putc(&text_content->buffer, ' ');
      write_pdf_escaped_string(&text_content->buffer, record.fields[1]);
      dbuffer_printf(&text_content->buffer, " Tj");
      continue;
    }
    fprintf(stderr, "Invalid text command: '%s'\n", record.fields[0]);
  }
  dbuffer_putc(&text_content->buffer, '\n');
  return 0;
}

static int
parse_graphic(FILE *input, int origin_x, int origin_y,
    struct text_content *text_content, struct pdf_resources *resources)
{
  int parse_result;
  int x, y, arg1, arg2;
  x = origin_x;
  y = origin_y;
  for (;;) {
    parse_result = parse_record(input, &record);
    if (parse_result == EOF) {
      fprintf(stderr, "Graphic not ended before end of file.\n");
      return 1;
    }
    if (parse_result)
      continue;
    if (strcmp(record.fields[0], "END") == 0)
      break;
    if (strcmp(record.fields[0], "MOVE") == 0) {
      if (record.field_count != 3) {
        fprintf(stderr, "Graphic MOVE command must take 2 arguments.\n");
        continue;
      }
      if (str_to_int(record.fields[1], &arg1)
          || str_to_int(record.fields[2], &arg2)) {
        fprintf(stderr, "Graphic MOVE command takes only integer arguments.\n");
        continue;
      }
      x = origin_x + arg1;
      y = origin_y + arg2;
      continue;
    }
    if (strcmp(record.fields[0], "START") == 0) {
      if (record.field_count != 2) {
        fprintf(stderr, "START command must take one argument.\n");
        return 1;
      }
      if (strcmp(record.fields[1], "GRAPHIC") == 0) {
        if (parse_graphic(input, x, y, text_content, resources))
          return 1;
        continue;
      }
      if (strcmp(record.fields[1], "TEXT") == 0) {
        if (parse_text(input, x, y, text_content, resources))
          return 1;
        continue;
      }
      fprintf(stderr, "Invalid graphic START command argument: '%s'\n",
          record.fields[1]);
      return 1;
    }
    fprintf(stderr, "Invalid graphic command: '%s'\n", record.fields[0]);
  }
  return 0;
}

int
print_pages(FILE *pages_file, FILE *typeface_file, FILE *pdf_file)
{
  int ret, parse_result;
  int obj_resources, obj_page_list, obj_catalog;
  struct pdf_xref_table xref_table;
  struct pdf_page_list page_list;
  struct pdf_resources resources;
  struct text_content text_content;
  ret = 0;
  init_pdf_xref_table(&xref_table);
  init_pdf_page_list(&page_list);
  init_pdf_resources(&resources);
  obj_resources = allocate_pdf_obj(&xref_table);
  obj_page_list = allocate_pdf_obj(&xref_table);
  obj_catalog = allocate_pdf_obj(&xref_table);
  pdf_write_header(pdf_file);

  text_content.x = 0;
  text_content.y = 0;
  text_content.font_size = 0;
  text_content.font_name[0] = '\0';
  dbuffer_init(&text_content.buffer, 1024 * 32, 1024 * 32);
  init_record(&record);
  for (;;) {
    parse_result = parse_record(pages_file, &record);
    if (parse_result == EOF)
      break;
    if (parse_result)
      continue;
    if (strcmp(record.fields[0], "START") == 0) {
      if (record.field_count != 2) {
        fprintf(stderr, "Pages START command must take 1 argument.\n");
        ret = 1;
        break;
      }
      if (strcmp(record.fields[1], "PAGE") == 0) {
        if (parse_graphic(pages_file, 0, 0, &text_content, &resources)) {
          ret = 1;
          break;
        }
        add_page(pdf_file, obj_page_list, &xref_table, &page_list,
            &text_content.buffer);
        text_content.x = 0;
        text_content.y = 0;
        text_content.font_size = 0;
        text_content.font_name[0] = '\0';
        text_content.buffer.size = 0;
        continue;
      }
      fprintf(stderr, "Invalid document START command argument: '%s'\n",
          record.fields[1]);
      ret = 1;
      break;
    }
  }
  dbuffer_free(&text_content.buffer);
  free_record(&record);

  pdf_add_resources(pdf_file, typeface_file, obj_resources, &resources,
      &xref_table);

  pdf_start_indirect_obj(pdf_file, &xref_table, obj_page_list);
  pdf_write_pages(pdf_file, obj_resources, page_list.page_count,
      page_list.page_objs);
  pdf_end_indirect_obj(pdf_file);

  pdf_start_indirect_obj(pdf_file, &xref_table, obj_catalog);
  pdf_write_catalog(pdf_file, obj_page_list);
  pdf_end_indirect_obj(pdf_file);

  pdf_add_footer(pdf_file, &xref_table, obj_catalog);
  free_pdf_page_list(&page_list);
  free_pdf_resources(&resources);
  free_pdf_xref_table(&xref_table);
  return ret;
}
