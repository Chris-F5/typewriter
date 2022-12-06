#include "pdf.h"

#include <stdlib.h>
#include <string.h>

#include "utils.h"

#define PDF_HEADER "%PDF-1.7\n"

static int set_offset(struct pdf_ctx *pdf, int obj);

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
pdf_add_resources(struct pdf_ctx *pdf, int obj)
{
  set_offset(pdf, obj) == 0 OR return 1;

  fprintf(pdf->file, "\
%d 0 obj << /Font << /F1 << /Type /Font /Subtype /Type1 /BaseFont /Helvetica \
>> >> >> endobj\n", obj);
  return 0;
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
pdf_add_page_list(struct pdf_ctx *pdf, int obj, int *pages, int page_count)
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
