/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

/*
 * tw.c
 * tw is short for typewriter. This file reads _pages_ from stdin and converts
 * this to PDF.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "tw.h"

/* A page's entire text content and its current state. */
struct text_content {
  int x, y, font_size, word_spacing;
  struct dbuffer buffer;
  char font_name[256];
};

static void add_page(FILE *pdf_file, int obj_parent,
    struct pdf_xref_table *xref, struct pdf_page_list *page_list,
    struct dbuffer *text_content, struct dbuffer *graphic_content);
static void write_pdf_escaped_string(struct dbuffer *buffer,
    const char *string);
static int parse_text(FILE *input, int x, int y,
    struct text_content *text_content, struct pdf_resources *resources);
static int parse_graphic(FILE *input, int origin_x, int origin_y,
    struct text_content *text_content, struct dbuffer *graphic_content,
    struct pdf_resources *resources);

/* 
 * Records are parsed in multiple functions, in order to avoid reallocating
 * record memory each time, _record_ is kept as a static global variable.
 * It is reset before each use so its persistent state does not matter.
 * Use of record is not thread-safe.
 */
static struct record record;

/* Add a page to _pdf_file_ with _text_content_ and _graphic_content_. */
static void
add_page(FILE *pdf_file, int obj_parent, struct pdf_xref_table *xref,
    struct pdf_page_list *page_list, struct dbuffer *text_content,
    struct dbuffer *graphic_content)
{
  int obj_content, obj_page;
  long length;
  /* Allocate pdf indirecto objects for the page and its conetnt. */
  obj_content = allocate_pdf_obj(xref);
  obj_page = allocate_pdf_obj(xref);

  /* ET is the PDF control sequence for ending text content. */
  dbuffer_printf(text_content, "ET\n");
  /* Write a PDF stream indirect object containing the page content. */
  length = text_content->size + graphic_content->size;
  pdf_start_indirect_obj(pdf_file, xref, obj_content);
  fprintf(pdf_file, "<< /Length %ld >> stream\n", length);
  fwrite(text_content->data, 1, text_content->size, pdf_file);
  fwrite(graphic_content->data, 1, graphic_content->size, pdf_file);
  fprintf(pdf_file, "\nendstream\n");
  pdf_end_indirect_obj(pdf_file);

  /*
   * The PDF page object holds a reference to the PDF page content stream
   * object.
   */
  pdf_start_indirect_obj(pdf_file, xref, obj_page);
  pdf_write_page(pdf_file, obj_parent, obj_content);
  pdf_end_indirect_obj(pdf_file);

  /* Save a reference to the PDF page object. */
  add_pdf_page(page_list, obj_page);
}

/* Write a PDF specification compliant string to _buffer_. */
static void
write_pdf_escaped_string(struct dbuffer *buffer, const char *string)
{
  /* PDF string is enclosed in brackets. */
  dbuffer_putc(buffer, '(');
  while (*string) {
    switch (*string) {
      /* Brackets and backshalses need to be escaped. */
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

/* Parse text mode _pages_ content. */
static int
parse_text(FILE *input, int x, int y, struct text_content *text_content,
    struct pdf_resources *resources)
{
  int parse_result, arg1, font_size, word_spacing;
  char font_name[256];
  font_size = 0;
  font_name[0] = '\0';
  word_spacing = 0;
  /* Set the text transformation matrix to the (x,y) transformation. */
  dbuffer_printf(&text_content->buffer, "1 0 0 1 %d %d Tm", x, y);
  for (;;) {
    parse_result = parse_record(input, &record);
    if (parse_result == EOF) {
      fprintf(stderr, "Text not ended before end of file.\n");
      return 1;
    }
    if (parse_result) /* If failed to parse record then skip it. */
      continue;
    if (strcmp(record.fields[0], "END") == 0)
      /* Exit text mode. */
      break;
    if (strcmp(record.fields[0], "FONT") == 0) {
      /* FONT [FONT_NAME] [FONT_SIZE] */
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
      /* Update the current font name and size. */
      strcpy(font_name, record.fields[1]);
      font_size = arg1;
      /* Tell _resources_ that this font is being used. */
      include_font_resource(resources, font_name);
      continue;
    }
    if (strcmp(record.fields[0], "SPACE") == 0) {
      /* SPACE [WORD_SPACING] */
      if (record.field_count != 2) {
        fprintf(stderr, "Text SPACE command must take 1 arguments.\n");
        continue;
      }
      if (str_to_int(record.fields[1], &arg1)) {
        fprintf(stderr, "Text SPACE command's argument must be integer.\n");
        continue;
      }
      /* Update current word spacing. */
      word_spacing = arg1;
      continue;
    }
    if (strcmp(record.fields[0], "STRING") == 0) {
      /* STRING [STRING] */
      if (record.field_count != 2) {
        fprintf(stderr, "Text STRING command must take 1 argument.\n");
        continue;
      }
      /* If the font has not been set. */
      if (*font_name == '\0') {
        fprintf(stderr, "Text must specify font.\n");
        continue;
      }
      /* If a different font is used for this string. */
      if (strcmp(text_content->font_name, font_name)
          || text_content->font_size != font_size) {
        /* Change the active font. */
        dbuffer_printf(&text_content->buffer, " /%s %d Tf", font_name,
            font_size);
        strcpy(text_content->font_name, font_name);
        text_content->font_size = font_size;
      }
      /* If a different word spacing is used for this string. */
      if (word_spacing != text_content->word_spacing) {
        /*
         * Change the current word spacing. Divide by 1000 to convert to point.
         */
        dbuffer_printf(&text_content->buffer, " %f Tw",
            (float)word_spacing / 1000);
        text_content->word_spacing = word_spacing;
      }
      /* Separate this text command from the last. */
      dbuffer_putc(&text_content->buffer, ' ');
      /* Write the string command. */
      write_pdf_escaped_string(&text_content->buffer, record.fields[1]);
      dbuffer_printf(&text_content->buffer, " Tj");
      continue; /* Next record. */
    }
    /* If the first field was not recognised. */
    fprintf(stderr, "Invalid text command: '%s'\n", record.fields[0]);
  }
  dbuffer_putc(&text_content->buffer, '\n');
  return 0;
}

/* Parse graphic mode _pages_ content. */
static int
parse_graphic(FILE *input, int origin_x, int origin_y,
    struct text_content *text_content, struct dbuffer *graphic_content,
    struct pdf_resources *resources)
{
  int parse_result, x, y, arg1, arg2, image_id;
  x = origin_x;
  y = origin_y;
  for (;;) {
    parse_result = parse_record(input, &record);
    if (parse_result == EOF) {
      fprintf(stderr, "Graphic not ended before end of file.\n");
      return 1;
    }
    if (parse_result) /* If failed to parse record then skip it. */
      continue;
    if (strcmp(record.fields[0], "END") == 0)
      /* Exit graphic mode. */
      break;
    if (strcmp(record.fields[0], "MOVE") == 0) {
      /* MOVE [X_OFFSET] [Y_OFFSET] */
      if (record.field_count != 3) {
        fprintf(stderr, "Graphic MOVE command must take 2 arguments.\n");
        continue;
      }
      /* Try to convert the offset arguments to integers. */
      if (str_to_int(record.fields[1], &arg1)
          || str_to_int(record.fields[2], &arg2)) {
        fprintf(stderr, "Graphic MOVE command takes only integer arguments.\n");
        continue;
      }
      /* Update the current position. */
      x = origin_x + arg1;
      y = origin_y + arg2;
      continue;
    }
    if (strcmp(record.fields[0], "START") == 0) {
      /* START [GRAPHIC/TEXT] */
      /* A graphic may contain text content or another graphic. */
      if (record.field_count != 2) {
        fprintf(stderr, "START command must take one argument.\n");
        return 1;
      }
      if (strcmp(record.fields[1], "GRAPHIC") == 0) {
        if (parse_graphic(input, x, y, text_content, graphic_content,
              resources))
          return 1;
        continue;
      }
      if (strcmp(record.fields[1], "TEXT") == 0) {
        if (parse_text(input, x, y, text_content, resources))
          return 1;
        continue;
      }
      /* The first argument was not recognised. */
      fprintf(stderr, "Invalid graphic START command argument: '%s'\n",
          record.fields[1]);
      return 1; /* Error. */
    }
    if (strcmp(record.fields[0], "IMAGE") == 0) {
      /* IMAGE [WIDTH] [HEIGHT] [IMAGE_FILE_NAME] */
      if (record.field_count != 4) {
        fprintf(stderr, "IMAGE command must take 3 arguments.\n");
        continue;
      }
      /* Convert width and height to integers. */
      if (str_to_int(record.fields[1], &arg1)
          || str_to_int(record.fields[2], &arg2)) {
        fprintf(stderr,
            "IMAGE command's 1st and 2nd arguments must be integer.\n");
        continue;
      }
      /* Tell _resources_ that we are using this image. */
      image_id = include_image_resource(resources, record.fields[3]);
      /* Push graphic state. */
      dbuffer_printf(graphic_content, "q\n");
      /* Set graphic transformation matrix and draw image. */
      dbuffer_printf(graphic_content, "  %d 0 0 %d %d %d cm\n", arg1, arg2, x,
          y);
      dbuffer_printf(graphic_content, "  /Img%d Do\n", image_id);
      /* Pop graphic state. */
      dbuffer_printf(graphic_content, "Q\n");
      continue;
    }
    fprintf(stderr, "Invalid graphic command: '%s'\n", record.fields[0]);
  }
  return 0;
}

/* Read _pages_ format from _pages_file_ and write PDF to _pdf_file_. */
int
print_pages(FILE *pages_file, FILE *typeface_file, FILE *pdf_file)
{
  int ret, parse_result;
  int obj_resources, obj_page_list, obj_catalog;
  struct pdf_xref_table xref_table;
  struct pdf_page_list page_list;
  struct pdf_resources resources;
  struct text_content text_content;
  struct dbuffer graphic_content;
  ret = 0;
  /* Initialise resources. */
  init_pdf_xref_table(&xref_table);
  init_pdf_page_list(&page_list);
  init_pdf_resources(&resources);
  /* Allocate essential indirect objects. */
  obj_resources = allocate_pdf_obj(&xref_table);
  obj_page_list = allocate_pdf_obj(&xref_table);
  obj_catalog = allocate_pdf_obj(&xref_table);
  /* Write PDF header. */
  pdf_write_header(pdf_file);

  text_content.x = 0;
  text_content.y = 0;
  text_content.font_size = 0;
  text_content.font_name[0] = '\0';
  dbuffer_init(&text_content.buffer, 1024 * 32, 1024 * 32);
  /* Begin Text. */
  dbuffer_printf(&text_content.buffer, "BT\n");
  dbuffer_init(&graphic_content, 1024 * 4, 1024 * 4);
  init_record(&record);
  /* Parse each record in _pages_ document mode. */
  for (;;) {
    parse_result = parse_record(pages_file, &record);
    if (parse_result == EOF) /* Stop at end of file. */
      break;
    if (parse_result) /* If failed to parse record then skip it. */
      continue;
    if (strcmp(record.fields[0], "START") == 0) {
      if (record.field_count != 2) {
        fprintf(stderr, "Pages START command must take 1 argument.\n");
        /* ret=1 because this is an error and we will return 1. */
        ret = 1;
        break;
      }
      if (strcmp(record.fields[1], "PAGE") == 0) {
        /* Try to parse this PAGE's graphic. */
        if (parse_graphic(pages_file, 0, 0, &text_content, &graphic_content,
              &resources)) {
          ret = 1;
          break;
        }
        /* Add the page to the pdf file. */
        add_page(pdf_file, obj_page_list, &xref_table, &page_list,
            &text_content.buffer, &graphic_content);
        /* Reset the text and graphic for the next page. */
        text_content.x = 0;
        text_content.y = 0;
        text_content.font_size = 0;
        text_content.font_name[0] = '\0';
        text_content.buffer.size = 0;
        dbuffer_printf(&text_content.buffer, "BT\n");
        graphic_content.size = 0;
        graphic_content.data[0] = '\0';
        continue;
      }
      fprintf(stderr, "Invalid document START command argument: '%s'\n",
          record.fields[1]);
      ret = 1;
      break;
    }
  }
  /* Cleanup resources. */
  dbuffer_free(&text_content.buffer);
  free_record(&record);

  /* Add resources to PDF file and close it. */
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

int
main(int argc, char **argv)
{
  int opt;
  const char *output_file_name;
  FILE *pages_file, *typeface_file, *pdf_file;
  /* Default output to ./output.pdf */
  output_file_name = "./output.pdf";
  /* Parse command line arguments. */
  while ( (opt = getopt(argc, argv, "o:")) != -1) {
    switch (opt) {
      case 'o':
        output_file_name = optarg;
        break;
      default:
        fprintf(stderr, "Usage: %s [-o OUTPUT_FILE]\n", argv[0]);
        return 1;
    }
  }
  pages_file = stdin;
  /*
   * Typeface file is hardcoded to be called 'typeface'. This is a design
   * decision: if we could call the typeface file anything we wanted, then
   * other programs part of the typesetting process would struggle to locate
   * it. Just like 'Makefile', 'typeface' is a hard-coded file name.
   */
  typeface_file = fopen("./typeface", "r");
  pdf_file = fopen(output_file_name, "w");
  if (typeface_file == NULL) {
    fprintf(stderr, "tw: Failed to open typeface file.\n");
    return 1;
  }
  if (pdf_file == NULL) {
    fprintf(stderr, "tw: Failed to open output file '%s'.\n", output_file_name);
    return 1;
  }
  print_pages(pages_file, typeface_file, pdf_file);
  fclose(pages_file);
  fclose(typeface_file);
  fclose(pdf_file);
  return 0;
}
