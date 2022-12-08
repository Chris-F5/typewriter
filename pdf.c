#include "pdf.h"

#include <stdlib.h>
#include <string.h>

#include "utils.h"

#define PDF_HEADER "%PDF-1.7\n"

static int start_obj(struct pdf_ctx *pdf, int obj);

static int
start_obj(struct pdf_ctx *pdf, int obj)
{
  if (obj == 0) {
    pdf->error_flags |= PDF_ERROR_FLAG_RESERVED_OBJ;
    return PDF_ERROR_FLAG_RESERVED_OBJ;
  }
  if (pdf->obj_offsets == NULL) {
    pdf->error_flags |= PDF_ERROR_FLAG_MEMORY;
    return PDF_ERROR_FLAG_MEMORY;
  }
  if (obj < 0 || obj >= pdf->obj_allocated) {
    pdf->error_flags |= PDF_ERROR_FLAG_INVALID_OBJ;
    return PDF_ERROR_FLAG_INVALID_OBJ;
  }
  if (pdf->obj_offsets[obj]) {
    pdf->error_flags |= PDF_ERROR_FLAG_REPEAT_OBJ;
    return PDF_ERROR_FLAG_REPEAT_OBJ;
  }
  pdf->obj_offsets[obj] = ftell(pdf->file);
  fprintf(pdf->file, "%d 0 obj ", obj);
  return 0;
}

int
pdf_init(struct pdf_ctx *pdf, FILE *file)
{
  int error_flags = 0;
  size_t written;
  pdf->error_flags = 0;
  pdf->file = file;
  pdf->obj_count = 0;
  pdf->obj_allocated = 100;
  pdf->obj_offsets = malloc(sizeof(long) * pdf->obj_allocated);
  if (pdf->obj_offsets) {
    memset(pdf->obj_offsets, 0, sizeof(long) * pdf->obj_allocated);
    /* allocate the 'zero' object with offset zero*/
    pdf_allocate_obj(pdf);
  } else {
    pdf->obj_allocated = 0;
    error_flags |= PDF_ERROR_FLAG_MEMORY;
  }
  fprintf(pdf->file, "%%PDF-1.7\n");

  if (ferror(pdf->file)) {
    error_flags |= PDF_ERROR_FLAG_FILE;
    clearerr(pdf->file);
  }
  pdf->error_flags |= error_flags;
  return error_flags;
}

int
pdf_allocate_obj(struct pdf_ctx *pdf)
{
  int obj;
  if (pdf->obj_offsets == NULL) {
    pdf->error_flags |= PDF_ERROR_FLAG_MEMORY;
    return -1;
  }
  obj = pdf->obj_count;
  while (pdf->obj_count >= pdf->obj_allocated) {
    pdf->obj_offsets
      = realloc(pdf->obj_offsets, sizeof(long) * (pdf->obj_allocated + 100));
    if (pdf->obj_offsets) {
      memset(pdf->obj_offsets + pdf->obj_allocated * sizeof(long), 0,
          sizeof(long) * (pdf->obj_allocated += 100));
    } else {
      pdf->error_flags |= PDF_ERROR_FLAG_MEMORY;
      return -1;
    }
  }
  pdf->obj_count++;
  return obj;
}

int
pdf_add_stream(struct pdf_ctx *pdf, int obj, FILE *stream)
{
  int error_flags;
  long stream_length;
  char c;
  if( (error_flags = start_obj(pdf, obj)) )
    return error_flags;
  fseek(stream, 0, SEEK_END);
  stream_length = ftell(stream);
  fseek(stream, 0, SEEK_SET);
  fprintf(pdf->file, "<< /Length %ld >>\nstream\n", stream_length);
  while ( (c = getc(stream)) != EOF)
      putc(c, pdf->file);
  fprintf(pdf->file, "\nendstream\nendobj\n");

  if (ferror(stream)) {
    pdf->error_flags |= PDF_ERROR_FLAG_STREAM_FILE;
    return PDF_ERROR_FLAG_STREAM_FILE;
  }

  return 0;
}

int
pdf_add_true_type_program(struct pdf_ctx *pdf, int obj, const char *ttf,
    long ttf_size)
{
  int error_flags;
  long i;
  if( (error_flags = start_obj(pdf, obj)) )
    return error_flags;
  fprintf(pdf->file, "<<\n\
  /Filter /ASCIIHexDecode\n\
  /Length %ld\n\
  /Length1 %ld\n\
  >>\nstream\n", ttf_size * 2, ttf_size);
  for (i = 0; i < ttf_size; i++)
    fprintf(pdf->file, "%02x", (unsigned char)ttf[i]);
  fprintf(pdf->file, "\nendstream\nendobj\n");
  return 0;
}

int
pdf_add_int_array(struct pdf_ctx *pdf, int obj, const int *values, int count)
{
  int error_flags, i;
  if( (error_flags = start_obj(pdf, obj)) )
    return error_flags;
  fprintf(pdf->file, "[\n ");
  for (i = 0; i < count; i++)
    fprintf(pdf->file, " %d", values[i]);
  fprintf(pdf->file, "\n]\nendobj\n");
  pdf->error_flags |= error_flags;
  return 0;
}

int
pdf_add_resources(struct pdf_ctx *pdf, int obj, int font_widths,
    int font_descriptor, const char *font_name)
{
  int error_flags;
  if( (error_flags = start_obj(pdf, obj)) )
    return error_flags;
  fprintf(pdf->file, "<</Font << /F1 <<\n\
  /Type /Font\n\
  /Subtype /TrueType\n\
  /BaseFont /%s\n\
  /FirstChar 0\n\
  /LastChar 255\n\
  /Widths %d 0 R\n\
  /FontDescriptor %d 0 R\n\
  >> >> >> endobj\n", font_name, font_widths, font_descriptor);
  return 0;
}

int
pdf_add_font_descriptor(struct pdf_ctx *pdf, int obj, int font_file,
    const char *font_name, int flags, int italic_angle, int ascent, int descent,
    int cap_height, int stem_vertical, int min_x, int min_y, int max_x,
    int max_y)
{
  int error_flags;
  if( (error_flags = start_obj(pdf, obj)) )
    return error_flags;
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
  return 0;
}

int
pdf_add_page(struct pdf_ctx *pdf, int obj, int parent, int resources,
    int content)
{
  int error_flags;
  if( (error_flags = start_obj(pdf, obj)) )
    return error_flags;
  fprintf(pdf->file, "<<\n\
  /Type /Page\n\
  /Resources %d 0 R\n\
  /Parent %d 0 R\n\
  /Contents %d 0 R\n\
  >> endobj\n", resources, parent, content);
  return 0;
}

int
pdf_add_page_list(struct pdf_ctx *pdf, int obj, const int *pages,
    int page_count)
{
  int error_flags, i;
  if( (error_flags = start_obj(pdf, obj)) )
    return error_flags;
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
  return 0;
}

int
pdf_add_catalog(struct pdf_ctx *pdf, int obj, int page_list)
{
  int error_flags;
  if( (error_flags = start_obj(pdf, obj)) )
    return error_flags;
  fprintf(pdf->file, "<<\n\
  /Type /Catalog\n\
  /Pages %d 0 R\n\
  >> endobj\n", page_list);
  return 0;
}

int
pdf_end(struct pdf_ctx *pdf, int root_obj)
{
  int error_flags = 0, xref_offset, obj;
  if (root_obj < 0 || root_obj >= pdf->obj_allocated)
    error_flags |= PDF_ERROR_FLAG_INVALID_OBJ;
  if (root_obj == 0)
    error_flags |= PDF_ERROR_FLAG_RESERVED_OBJ;
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

  if(ferror(pdf->file))
    error_flags |= PDF_ERROR_FLAG_FILE;
  pdf->error_flags |= error_flags;
  return pdf->error_flags;
}
