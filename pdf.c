#include "pdf.h"

#include <stdlib.h>
#include <string.h>

#include "utils.h"

#define PDF_HEADER "%PDF-1.7\n"

static void hex_encode(const unsigned char *data, long data_size, FILE *file);
static int set_offset(struct pdf_ctx *pdf, int obj);

static void hex_encode(const unsigned char *data, long data_size, FILE *file)
{
  long i;
  for (i = 0; i < data_size; i++)
    fprintf(file, "%02x", data[i]);
}

static int
set_offset(struct pdf_ctx *pdf, int obj)
{
  if ( (pdf->obj_offsets[obj] = ftell(pdf->file)) < 0) {
    perror("ftell failed");
    return 1;
  }
  return 0;
}

int
pdf_allocate_obj(struct pdf_ctx *pdf)
{
  int obj;
  obj = pdf->obj_count++;
  if (obj >= pdf->obj_allocated) {
    pdf->obj_allocated += 100;
    pdf->obj_offsets
      = realloc(pdf->obj_offsets, sizeof(long) * pdf->obj_allocated);
    if (pdf->obj_offsets == NULL) {
      perror("malloc failed\n");
      return -1;
    }
  }
  pdf->obj_offsets[obj] = 0;
  return obj;
}

int
pdf_init(struct pdf_ctx *pdf, FILE *file)
{
  size_t written;

  pdf->file = file;
  pdf->obj_count = 1;
  pdf->obj_allocated = 100;
  (pdf->obj_offsets = malloc(sizeof(long) * pdf->obj_allocated))
    OR goto memory_error;

  fprintf(pdf->file, "%%PDF-1.7\n");
  return 0;
memory_error:
  perror("memory error");
  return 1;
}

int
pdf_add_stream(struct pdf_ctx *pdf, int obj, const char *stream,
    long stream_length)
{
  set_offset(pdf, obj) == 0 OR return 1;

  fprintf(pdf->file, "%d 0 obj << /Length %ld >>\nstream\n", obj,
      stream_length);
  fwrite(stream, 1, stream_length, pdf->file);
  fprintf(pdf->file, "\nendstream\nendobj\n");

  return 0;
}

int
pdf_add_true_type_program(struct pdf_ctx *pdf, int obj, const char *ttf,
    long ttf_size)
{
  set_offset(pdf, obj) == 0 OR return 1;

  fprintf(pdf->file, "\
%d 0 obj <<\n\
  /Filter /ASCIIHexDecode\n\
  /Length %ld\n\
  /Length1 %ld\n\
  >>\nstream\n", obj, ttf_size * 2, ttf_size);
  hex_encode((unsigned char *)ttf, ttf_size, pdf->file);
  fprintf(pdf->file, "\nendstream\nendobj\n");

  return 0;
}

int
pdf_add_int_array(struct pdf_ctx *pdf, int obj, const int *values, int count)
{
  int i;
  set_offset(pdf, obj) == 0 OR return 1;

  fprintf(pdf->file, "%d 0 obj [\n ", obj);
  for (i = 0; i < count; i++)
    fprintf(pdf->file, " %d", values[i]);
  fprintf(pdf->file, "\n]\nendobj\n");
  return 0;
}

int
pdf_add_resources(struct pdf_ctx *pdf, int obj, int font_widths,
    int font_descriptor, const char *font_name)
{
  set_offset(pdf, obj) == 0 OR return 1;

  fprintf(pdf->file, "\
%d 0 obj << /Font << /F1 <<\n\
  /Type /Font\n\
  /Subtype /TrueType\n\
  /BaseFont /%s\n\
  /FirstChar 0\n\
  /LastChar 255\n\
  /Widths %d 0 R\n\
  /FontDescriptor %d 0 R\n\
  >> >> >> endobj\n", obj, font_name, font_widths, font_descriptor);
  return 0;
}

int
pdf_add_font_descriptor(struct pdf_ctx *pdf, int obj, int font_file,
    const char *font_name, int flags, int italic_angle, int ascent, int descent,
    int cap_height, int stem_vertical, int min_x, int min_y, int max_x,
    int max_y)
{
  set_offset(pdf, obj) == 0 OR return 1;

  fprintf(pdf->file, "\
%d 0 obj <<\n\
  /Type /FontDescriptor\n\
  /FontName /%s\n\
  /FontFile2 %d 0 R\n\
  /Flags %d\n\
  /FontBBox [%d, %d, %d, %d]\n\
  /ItalicAngle %d\n\
  /Ascent %d\n\
  /Descent %d\n\
  /CapHeight %d\n\
  /StemV %d\n\
  >> endobj\n", obj, font_name, font_file, flags, min_x, min_y, max_x, max_y,
      italic_angle, ascent, descent, cap_height, stem_vertical);
  return 0;
}

int
pdf_add_page(struct pdf_ctx *pdf, int obj, int parent, int resources,
    int content)
{
  set_offset(pdf, obj) == 0 OR return 1;
  fprintf(pdf->file, "\
%d 0 obj << /Type /Page\n\
  /Resources %d 0 R\n\
  /Parent %d 0 R\n\
  /Contents %d 0 R\n\
  >> endobj\n", obj, resources, parent, content);
  return 0;
}

int
pdf_add_page_list(struct pdf_ctx *pdf, int obj, const int *pages,
    int page_count)
{
  int i;
  set_offset(pdf, obj) == 0 OR return 1;
  /* 595x842 is a portrait A4 page. */
  fprintf(pdf->file, "\
%d 0 obj << /Type /Pages\n\
  /Kids [\n", obj);
  for (i = 0; i < page_count; i++)
    fprintf(pdf->file, "    %d 0 R\n", pages[i]);
  fprintf(pdf->file, "\
    ]\n\
  /Count %d\n\
  /MediaBox [0 0 595 842]\n\
  >> endobj\n", page_count);
  return 0;
}

int
pdf_add_catalog(struct pdf_ctx *pdf, int obj, int page_list)
{
  int i;
  set_offset(pdf, obj) == 0 OR return 1;
  fprintf(pdf->file, "\
%d 0 obj << /Type /Catalog\n\
  /Pages %d 0 R\n\
  >> endobj\n", obj, page_list);
  return 0;
}

int
pdf_end(struct pdf_ctx *pdf, int root_obj)
{
  int xref_offset, obj;
  (xref_offset = ftell(pdf->file)) >= 0 OR goto file_error;
  fprintf(pdf->file, "xref\n");
  fprintf(pdf->file, "0 %d\n", pdf->obj_count);
  fprintf(pdf->file, "000000000 65535 f \n");
  for (obj = 1; obj < pdf->obj_count; obj++)
    fprintf(pdf->file, "%09ld 00000 n \n", pdf->obj_offsets[obj]);
  fprintf(pdf->file, "trailer << /Size %d /Root %d 0 R >>\n",
      pdf->obj_count, root_obj);
  fprintf(pdf->file, "startxref\n%d\n", xref_offset);
  fprintf(pdf->file, "%%%%EOF");
  return 0;
file_error:
  perror("file error");
  return 1;
}
