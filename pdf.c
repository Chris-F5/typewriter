#include "tw.h"

#include <stdlib.h>
#include <string.h>

static void start_obj(struct pdf_ctx *pdf, int obj);

static void
start_obj(struct pdf_ctx *pdf, int obj)
{
  pdf->obj_offsets[obj] = ftell(pdf->file);
  fprintf(pdf->file, "%d 0 obj ", obj);
}

void
pdf_init(struct pdf_ctx *pdf, FILE *file)
{
  size_t written;
  pdf->file = file;
  pdf->obj_count = 0;
  pdf->obj_allocated = 100;
  pdf->obj_offsets = xmalloc(sizeof(long) * pdf->obj_allocated);
  memset(pdf->obj_offsets, 0, sizeof(long) * pdf->obj_allocated);
  /* Allocate the 'zero' object. */
  pdf_allocate_obj(pdf);
  fprintf(pdf->file, "%%PDF-1.7\n");
}

int
pdf_allocate_obj(struct pdf_ctx *pdf)
{
  int obj;
  obj = pdf->obj_count++;
  while (obj >= pdf->obj_allocated) {
    pdf->obj_offsets
      = xrealloc(pdf->obj_offsets, sizeof(long) * (pdf->obj_allocated + 100));
    memset(pdf->obj_offsets + pdf->obj_allocated * sizeof(long), 0,
        sizeof(long) * (pdf->obj_allocated + 100));
    pdf->obj_allocated += 100;
  }
  return obj;
}

void
pdf_add_true_type_program(struct pdf_ctx *pdf, int obj, const char *ttf,
    long ttf_size)
{
  long i;
  start_obj(pdf, obj);
  fprintf(pdf->file, "<<\n\
  /Filter /ASCIIHexDecode\n\
  /Length %ld\n\
  /Length1 %ld\n\
  >>\nstream\n", ttf_size * 2, ttf_size);
  for (i = 0; i < ttf_size; i++)
    fprintf(pdf->file, "%02x", (unsigned char)ttf[i]);
  fprintf(pdf->file, "\nendstream\nendobj\n");
}

void
pdf_add_stream(struct pdf_ctx *pdf, int obj, const char *bytes,
    long bytes_count)
{
  start_obj(pdf, obj);
  fprintf(pdf->file, "<< /Length %ld >> stream\n", bytes_count);
  fwrite(bytes, 1, bytes_count, pdf->file);
  fprintf(pdf->file, "\nendstream\nendobj\n");
}

void
pdf_add_int_array(struct pdf_ctx *pdf, int obj, const int *values, int count)
{
  int i;
  start_obj(pdf, obj);
  fprintf(pdf->file, "[\n ");
  for (i = 0; i < count; i++)
    fprintf(pdf->file, " %d", values[i]);
  fprintf(pdf->file, "\n]\nendobj\n");
}

void
pdf_add_resources(struct pdf_ctx *pdf, int obj, int font_widths,
    int font_descriptor, const char *font_name)
{
  start_obj(pdf, obj);
  fprintf(pdf->file, "<</Font << /F1 <<\n\
  /Type /Font\n\
  /Subtype /TrueType\n\
  /BaseFont /%s\n\
  /FirstChar 0\n\
  /LastChar 255\n\
  /Widths %d 0 R\n\
  /FontDescriptor %d 0 R\n\
  >> >> >> endobj\n", font_name, font_widths, font_descriptor);
}

void
pdf_add_font_descriptor(struct pdf_ctx *pdf, int obj, int font_file,
    const char *font_name, int flags, int italic_angle, int ascent, int descent,
    int cap_height, int stem_vertical, int min_x, int min_y, int max_x,
    int max_y)
{
  start_obj(pdf, obj);
  fprintf(pdf->file, "<<\n\
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
  >> endobj\n", font_name, font_file, flags, min_x, min_y, max_x, max_y,
      italic_angle, ascent, descent, cap_height, stem_vertical);
}

void
pdf_add_page(struct pdf_ctx *pdf, int obj, int parent, int resources,
    int content)
{
  start_obj(pdf, obj);
  fprintf(pdf->file, "<<\n\
  /Type /Page\n\
  /Resources %d 0 R\n\
  /Parent %d 0 R\n\
  /Contents %d 0 R\n\
  >> endobj\n", resources, parent, content);
}

void
pdf_add_page_list(struct pdf_ctx *pdf, int obj, const int *pages,
    int page_count)
{
  int i;
  start_obj(pdf, obj);
  /* 595x842 is a portrait A4 page. */
  fprintf(pdf->file, "<<\n\
  /Type /Pages\n\
  /Kids [\n");
  for (i = 0; i < page_count; i++)
    fprintf(pdf->file, "    %d 0 R\n", pages[i]);
  fprintf(pdf->file, "\
    ]\n\
  /Count %d\n\
  /MediaBox [0 0 595 842]\n\
  >> endobj\n", page_count);
}

void
pdf_add_catalog(struct pdf_ctx *pdf, int obj, int page_list)
{
  start_obj(pdf, obj);
  fprintf(pdf->file, "<<\n\
  /Type /Catalog\n\
  /Pages %d 0 R\n\
  >> endobj\n", page_list);
}

void
pdf_end(struct pdf_ctx *pdf, int root_obj)
{
  int xref_offset, obj;
  xref_offset = ftell(pdf->file);
  fprintf(pdf->file, "xref\n");
  fprintf(pdf->file, "0 %d\n", pdf->obj_count);
  fprintf(pdf->file, "000000000 65535 f \n");
  for (obj = 1; obj < pdf->obj_count; obj++)
    fprintf(pdf->file, "%09ld 00000 n \n", pdf->obj_offsets[obj]);
  fprintf(pdf->file, "trailer << /Size %d /Root %d 0 R >>\n",
      pdf->obj_count, root_obj);
  fprintf(pdf->file, "startxref\n%d\n", xref_offset);
  fprintf(pdf->file, "%%%%EOF");
}
