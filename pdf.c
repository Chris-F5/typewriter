#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "tw.h"

void
init_pdf_xref_table(struct pdf_xref_table *xref)
{
  xref->obj_count = 0;
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
init_pdf_page_list(struct pdf_page_list *page_list)
{
  page_list->count = 0;
  page_list->allocated = 100;
  page_list->page_objs = xmalloc(page_list->allocated * sizeof(int));
}

void
pdf_page_list_append(struct pdf_page_list *page_list, int page)
{
  if (page_list->count == page_list->allocated) {
    page_list->allocated += 100;
    page_list->page_objs = xrealloc(page_list->page_objs, page_list->allocated);
  }
  page_list->page_objs[page_list->count++] = page;
}

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
pdf_write_resources(FILE *file, int font_widths, int font_descriptor,
    const char *font_name)
{
  fprintf(file, "<</Font << /F1 <<\n\
  /Type /Font\n\
  /Subtype /TrueType\n\
  /BaseFont /%s\n\
  /FirstChar 0\n\
  /LastChar 255\n\
  /Widths %d 0 R\n\
  /FontDescriptor %d 0 R\n\
  >> >> >>\n", font_name, font_widths, font_descriptor);
}

void
pdf_write_font_descriptor(FILE *file, int font_file, const char *font_name,
    int flags, int italic_angle, int ascent, int descent, int cap_height,
    int stem_vertical, int min_x, int min_y, int max_x, int max_y)
{
  fprintf(file, "<<\n\
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
  >>\n", font_name, font_file, flags, min_x, min_y, max_x, max_y, italic_angle,
      ascent, descent, cap_height, stem_vertical);
}

void
pdf_write_page(FILE *file, int parent, int resources, int content)
{
  fprintf(file, "<<\n\
  /Type /Page\n\
  /Resources %d 0 R\n\
  /Parent %d 0 R\n\
  /Contents %d 0 R\n\
  >>\n", resources, parent, content);
}

void
pdf_write_page_list(FILE *file, const struct pdf_page_list *pages)
{
  int i;
  fprintf(file, "<<\n\
  /Type /Pages\n\
  /Kids [\n");
  for (i = 0; i < pages->count; i++)
    fprintf(file, "    %d 0 R\n", pages->page_objs[i]);
  /* 595x842 is a portrait A4 page. */
  fprintf(file, "    ]\n\
  /Count %d\n\
  /MediaBox [0 0 595 842]\n\
  >>\n", pages->count);
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
pdf_write_footer(FILE *file, struct pdf_xref_table *xref, int root_obj)
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
