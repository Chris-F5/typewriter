/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "tw.h"

#define OBJ_FLAG_PAGE 1

struct pdf_obj {
  long offset;
  int flags;
  struct pdf_obj *next;
};

struct pdf_ctx {
	int obj_count;
  struct pdf_obj *base;
	struct pdf_obj *head;
};

static void pdf_write_file_stream(FILE *pdf_file, FILE *data_file);
static void add_page(FILE *output_file, struct pdf_ctx *pdf, const char *content,
    long length, int pages_obj);
static void print_pages(FILE *input_file, FILE *output_file, FILE *font_file,
    int cwidth, int cheight, const struct font_info* font_info, int font_size);

static int page_width = 595;
static int page_height = 842;
static int vertical_margin = 50;
static int horizontal_margin = 50;

static void
pdf_write_file_stream(FILE *pdf_file, FILE *data_file)
{
  long size, i;
  fseek(data_file, 0, SEEK_END);
  size = ftell(data_file);
  fseek(data_file, 0, SEEK_SET);
  fprintf(pdf_file, "\
<<\n\
  /Filter /ASCIIHexDecode\n\
  /Length %ld\n\
  /Length1 %ld\n\
>>\n\
stream\n", size * 2, size);
  for (i = 0; i < size; i++)
    fprintf(pdf_file, "%02x", (unsigned char)fgetc(data_file));
  fprintf(pdf_file, "\nendstream\n");
}

static void
add_page(FILE *output_file, struct pdf_ctx *pdf, const char *content,
    long length, int pages_obj)
{
  int content_obj;
  content_obj = pdf->obj_count++;
  pdf->head = pdf->head->next = xmalloc(sizeof(struct pdf_obj));
  pdf->head->offset = ftell(output_file);
  pdf->head->flags = 0;
  pdf->head->next = NULL;
  fprintf(output_file, "%d 0 obj\n", content_obj);
  fprintf(output_file, "<< /Length %ld >> stream\n", length);
  fwrite(content, 1, length, output_file);
  fprintf(output_file, "\nendstream\n");
  fprintf(output_file, "endobj\n");

  pdf->head = pdf->head->next = xmalloc(sizeof(struct pdf_obj));
  pdf->head->offset = ftell(output_file);
  pdf->head->flags = OBJ_FLAG_PAGE;
  pdf->head->next = NULL;
  fprintf(output_file, "%d 0 obj\n", pdf->obj_count++);
  fprintf(output_file, "\
<<\n\
  /Type /Page\n\
  /Parent %d 0 R\n\
  /Contents %d 0 R\n\
>>\n", pages_obj, content_obj);
  fprintf(output_file, "endobj\n");
}


static void
print_pages(FILE *input_file, FILE *output_file, FILE *font_file, int cwidth,
    int cheight, const struct font_info* font_info, int font_size)
{
  struct pdf_ctx pdf;
  int i, c;
  struct pdf_obj *o;
  int font_program_obj, font_widths_obj, font_descriptor_obj, font_obj;
  int resources_obj, pages_obj, catalog_obj;
  struct pdf_obj *pages_obj_ref;
  struct dbuffer content;
  int row, colunm, line, page;
  long xref_offset;

  pdf.obj_count = 1;
  pdf.base = pdf.head = xmalloc(sizeof(struct pdf_obj));
  pdf.base->offset = 0;
  pdf.base->flags = 0;
  pdf.base->next = NULL;

  fprintf(output_file, "%%PDF-1.7\n");

  font_program_obj = pdf.obj_count++;
  pdf.head = pdf.head->next = xmalloc(sizeof(struct pdf_obj));
  pdf.head->offset = ftell(output_file);
  pdf.head->flags = 0;
  pdf.head->next = NULL;
  fprintf(output_file, "%d 0 obj\n", font_program_obj);
  pdf_write_file_stream(output_file, font_file);
  fprintf(output_file, "endobj\n");

  font_widths_obj = pdf.obj_count++;
  pdf.head = pdf.head->next = xmalloc(sizeof(struct pdf_obj));
  pdf.head->offset = ftell(output_file);
  pdf.head->flags = 0;
  pdf.head->next = NULL;
  fprintf(output_file, "%d 0 obj\n", font_widths_obj);
  fprintf(output_file, "[\n ");
  for (i = 0; i < 256; i++)
    fprintf(output_file, " %d", font_info->char_widths[i]);
  fprintf(output_file, "\n]\n");
  fprintf(output_file, "endobj\n");

  font_descriptor_obj = pdf.obj_count++;
  pdf.head = pdf.head->next = xmalloc(sizeof(struct pdf_obj));
  pdf.head->offset = ftell(output_file);
  pdf.head->flags = 0;
  pdf.head->next = NULL;
  fprintf(output_file, "%d 0 obj\n", font_descriptor_obj);
  fprintf(output_file, "\
<<\n\
  /Type /FontDescriptor\n\
  /FontName /MonoFont\n\
  /FontFile2 %d 0 R\n\
  /Flags 6\n\
  /FontBBox [%d, %d, %d, %d]\n\
  /ItalicAngle -10\n\
  /Ascent 255\n\
  /Descent 255\n\
  /CapHeight 255\n\
  /StemV 10\n\
>>\n", font_program_obj, font_info->x_min, font_info->y_min, font_info->x_max,
      font_info->y_max);
  fprintf(output_file, "endobj\n");

  font_obj = pdf.obj_count++;
  pdf.head = pdf.head->next = xmalloc(sizeof(struct pdf_obj));
  pdf.head->offset = ftell(output_file);
  pdf.head->flags = 0;
  pdf.head->next = NULL;
  fprintf(output_file, "%d 0 obj\n", font_obj);
  fprintf(output_file, "\
<<\n\
  /Type /Font\n\
  /Subtype /TrueType\n\
  /BaseFont /MonoFont\n\
  /FirstChar 0\n\
  /LastChar 255\n\
  /Widths %d 0 R\n\
  /FontDescriptor %d 0 R\n\
>>\n", font_widths_obj, font_descriptor_obj);
  fprintf(output_file, "endobj\n");

  resources_obj = pdf.obj_count++;
  pdf.head = pdf.head->next = xmalloc(sizeof(struct pdf_obj));
  pdf.head->offset = ftell(output_file);
  pdf.head->flags = 0;
  pdf.head->next = NULL;
  fprintf(output_file, "%d 0 obj\n", resources_obj);
  fprintf(output_file, "\
<<\n\
  /Font <<\n\
    /MonoFont %d 0 R\n\
  >>\n\
>>\n", font_obj);
  fprintf(output_file, "endobj\n");

  pages_obj = pdf.obj_count++;
  pages_obj_ref = pdf.head = pdf.head->next = xmalloc(sizeof(struct pdf_obj));
  pdf.head->offset = 0;
  pdf.head->flags = 0;
  pdf.head->next = NULL;

  /* TODO: ADD PAGES. */
  dbuffer_init(&content, 1024 * 8, 1024 * 4);
  dbuffer_printf(&content, "BT\n");
  dbuffer_printf(&content, "/MonoFont %d Tf\n", font_size);
  page = row = colunm = 0;
  line = 1;
  while ( (c = fgetc(input_file)) != EOF) {
    if (c == '\n') {
      if (colunm > 0)
        dbuffer_printf(&content, ") Tj\n");
      line++;
      row++;
      colunm = 0;
      if (row >= cheight) {
        row = 0;
        page++;
        dbuffer_printf(&content, "ET");
        add_page(output_file, &pdf, content.data, content.size, pages_obj);
        content.size = 0;
        dbuffer_printf(&content, "BT\n");
        dbuffer_printf(&content, "/MonoFont %d Tf\n", font_size);
      }
      continue;
    }
    if (colunm == 0) {
      dbuffer_printf(&content, "1 0 0 1 %d %d Tm\n(", horizontal_margin,
          842 - vertical_margin - row * font_size);
    }
    if (colunm >= cwidth) {
      fprintf(stderr, "Warning: character width exceeded on line %d\n", line);
    }
    switch (c) {
      case '(':
      case ')':
      case '\\':
        dbuffer_putc(&content, '\\');
      default:
        dbuffer_putc(&content, c);
    }
    colunm++;
  }
  if (page == 0 || row != 0 || colunm != 0) {
    dbuffer_printf(&content, "ET");
    add_page(output_file, &pdf, content.data, content.size, pages_obj);
  }
  dbuffer_free(&content);

  pages_obj_ref->offset = ftell(output_file);
  fprintf(output_file, "%d 0 obj\n", pages_obj);
  fprintf(output_file, "\
<<\n\
  /Type /Pages\n\
  /Resources %d 0 R\n\
  /Kids [\n", resources_obj);
  c = 0;
  for (o = pdf.base, i = 0; o; o = o->next, i++) {
    if (o->flags & OBJ_FLAG_PAGE) {
      fprintf(output_file, "    %d 0 R\n", i);
      c++;
    }
  }
  /* A4 portrait is 595x842. */
  fprintf(output_file, "\
  ]\n\
  /Count %d\n\
  /MediaBox [0 0 595 842]\n\
>>\n", c);
  fprintf(output_file, "endobj\n");

  catalog_obj = pdf.obj_count++;
  pdf.head = pdf.head->next = xmalloc(sizeof(struct pdf_obj));
  pdf.head->offset = ftell(output_file);
  pdf.head->flags = 0;
  pdf.head->next = NULL;
  fprintf(output_file, "%d 0 obj\n", catalog_obj);
  fprintf(output_file, "\
<<\n\
  /Type /Catalog\n\
  /Pages %d 0- R\n\
>>\n", pages_obj);
  fprintf(output_file, "endobj\n");

  xref_offset = ftell(output_file);
  fprintf(output_file, "\
xref\n\
0 %d\n\
000000000 65535 f \n", pdf.obj_count);
  for (o = pdf.base->next; o; o = o->next)
    fprintf(output_file, "%09ld 00000 n \n", o->offset);
  fprintf(output_file, "\
trailer << /Size %d /Root %d 0 R >>\n\
startxref\n\
%ld\n\
%%%%EOF", pdf.obj_count, catalog_obj, xref_offset);

  for (o = pdf.base; o;) {
    pdf.base = o->next;
    free(o);
    o = pdf.base;
  }
}

int
main(int argc, char **argv)
{
  int opt;
  const char *output_file_name;
  const char *font_file_name;
  FILE *input_file, *output_file, *font_file;
  struct font_info font_info;
  int cwidth, cheight;
  int font_size = 10;
  output_file_name = "./output.pdf";
  font_file_name = "./fonts/cmu.typewriter-text-regular.ttf";
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
  input_file = stdin;
  output_file = fopen(output_file_name, "w");
  font_file = fopen(font_file_name, "r");
  if (output_file == NULL) {
    fprintf(stderr, "tw: Failed to open output file '%s'.\n", output_file_name);
    return 1;
  }
  if (font_file == NULL) {
    fprintf(stderr, "tw: Failed to open font file '%s'.\n", font_file_name);
    return 1;
  }
  if (read_ttf(font_file, &font_info)) {
    fprintf(stderr, "tw: Failed to parse font file '%s'.\n", font_file_name);
    return 1;
  }
  cwidth = ((page_width - horizontal_margin * 2) * 1000)
    / (font_info.char_widths[(unsigned char)'A'] * font_size);
  cheight = (page_height - vertical_margin * 2) / font_size;
  printf("%dx%d\n", cwidth, cheight);

  print_pages(input_file, output_file, font_file, cwidth, cheight, &font_info,
    font_size);

  fclose(input_file);
  fclose(output_file);
  fclose(font_file);
  return 0;
}
