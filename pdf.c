/*
 * Copyright (C) 2023 Christopher Lang
 * See LICENSE for license details.
 */

/* 
 * This file implements a PDF 1.7 writer. Get a copy of the 1.7 specification
 * to understand the format.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "tw.h"

static int pdf_add_font(FILE *pdf_file, FILE *font_file,
    struct pdf_xref_table *xref, const char *name);
static int pdf_add_image(FILE *pdf_file, FILE *image_file,
    struct pdf_xref_table *xref);

/* Write the 1.7 header to a new PDF file. */
void
pdf_write_header(FILE *file)
{
  fprintf(file, "%%PDF-1.7\n");
}

/*
 * Start an 'indirect object' (see the PDf spec). _obj_ is the unique reference
 * to use for this object. Add the indirecto object to the xref table.
 */
void
pdf_start_indirect_obj(FILE *file, struct pdf_xref_table *xref, int obj)
{
  xref->obj_offsets[obj] = ftell(file);
  fprintf(file, "%d 0 obj\n", obj);
}

/* End a PDF 'indirect object' (see the PDF spec). */
void
pdf_end_indirect_obj(FILE *file)
{
  fprintf(file, "endobj\n");
}

/* Write a PDF 'stream' with contents of _data_file_ encoded as hex. */
void
pdf_write_file_stream(FILE *pdf_file, FILE *data_file)
{
  long size, i;
  /* Get the size of the file. */
  fseek(data_file, 0, SEEK_END);
  size = ftell(data_file);
  /* Return to the start of the file. */
  fseek(data_file, 0, SEEK_SET);
  /* Write the PDF dictionary and start the stream. */
  fprintf(pdf_file, "<<\n\
  /Filter /ASCIIHexDecode\n\
  /Length %ld\n\
  /Length1 %ld\n\
  >>\nstream\n", size * 2, size);
  /* Write all the stream bytes encoded as hex. */
  for (i = 0; i < size; i++)
    fprintf(pdf_file, "%02x", (unsigned char)fgetc(data_file));
  /* End the stream. */
  fprintf(pdf_file, "\nendstream\n");
}

/*
 * Write a PDF text 'stream' with the data pointed to by _data_ of length
 * _size_.
 */
void
pdf_write_text_stream(FILE *file, const char *data, long size)
{
  fprintf(file, "<< /Length %ld >> stream\n", size);
  fwrite(data, 1, size, file);
  fprintf(file, "\nendstream\n");
}

/* Write a PDF integer array to _file_ (see the PDF spec). */
void
pdf_write_int_array(FILE *file, const int *values, int count)
{
  int i;
  /* Comma separated values, trailing comma is allowed */
  fprintf(file, "[\n ");
  for (i = 0; i < count; i++)
    fprintf(file, " %d", values[i]);
  fprintf(file, "\n]\n");
}

/* Write a PDF font descriptor dictionary to _file_ (see the PDF spec). */
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

/* Write a PDF page descriptor dictionary to _file_ (see the PDF spec). */
void
pdf_write_page(FILE *file, int parent, int content)
{
  fprintf(file, "<<\n\
  /Type /Page\n\
  /Parent %d 0 R\n\
  /Contents %d 0 R\n\
>>\n", parent, content);
}

/* Write a PDF font dictionary to _file_ (see the PDF spec). */
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

/* Write PDF pages dictionary to _file_ (see the PDF spec). */
void
pdf_write_pages(FILE *file, int resources, int page_count, const int *page_objs)
{
  int i;
  fprintf(file, "<<\n\
  /Type /Pages\n\
  /Resources %d 0 R\n\
  /Kids [\n", resources);
  /* Add a reference to all page objects. */
  for (i = 0; i < page_count; i++)
    fprintf(file, "    %d 0 R\n", page_objs[i]);
  /* 595x842 is a portrait A4 page. */
  fprintf(file, "    ]\n\
  /Count %d\n\
  /MediaBox [0 0 595 842]\n\
>>\n", page_count);
}

/* Write a PDF catalog dictionary to _file_ (see the PDF spec). */
void
pdf_write_catalog(FILE *file, int page_list)
{
  fprintf(file, "<<\n\
  /Type /Catalog\n\
  /Pages %d 0 R\n\
>>\n", page_list);
}

/* Initialise a PDF xref table. */
void
init_pdf_xref_table(struct pdf_xref_table *xref)
{
  /* The 'zero' object is the first (see the PDF spec). */
  xref->obj_count = 1;
  xref->allocated = 100;
  /* Allocate memory for objects. */
  xref->obj_offsets = xmalloc(xref->allocated * sizeof(long));
  /*
   * Set unused elements to zero so if we try to use them as references before
   * they are set then we will get an error.
   */
  memset(xref->obj_offsets, 0, xref->allocated * sizeof(long));
}

/* Allocate a PDF object reference number from the xref table. */
int
allocate_pdf_obj(struct pdf_xref_table *xref)
{
  /*
   * If more memory needs to allocated to fit the new object, then allocate 100
   * more.
   */
  if (xref->obj_count == xref->allocated) {
    xref->obj_offsets
      = xrealloc(xref->obj_offsets, (xref->allocated + 100) * sizeof(long));
    /* Set the newly allocated elements to zero. */
    memset(xref->obj_offsets + xref->allocated, 0, 100 * sizeof(long));
    xref->allocated += 100;
  }
  /* Return the old _obj_count_ and increment _object_count_. */
  return xref->obj_count++;
}

/* Add the PDF footer to _file_ (see the PDF spec). */
void
pdf_add_footer(FILE *file, const struct pdf_xref_table *xref, int root_obj)
{
  int xref_offset, i;
  /* Get our offset in _file_. */
  xref_offset = ftell(file);
  /* Write the footer. */
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

/* Free heap allocated memory of the _xref_ table. */
void
free_pdf_xref_table(struct pdf_xref_table *xref)
{
  free(xref->obj_offsets);
}

/* Initialise _resources_. */
void
init_pdf_resources(struct pdf_resources *resources)
{
  /* Initialise the records that will be used to store a list of strings. */
  init_record(&resources->fonts_used);
  init_record(&resources->images);
}

/* Remember that _font_ was used in the PDF, so it can be embedded later. */
void
include_font_resource(struct pdf_resources *resources, const char *font)
{
  int i;
  /*
   * Loop through all fonts currently in the _fonts_used_ list. If _font_ is
   * found to already be in this list then reutrn.
   */
  for (i = 0; i < resources->fonts_used.field_count; i++)
    if (strcmp(resources->fonts_used.fields[i], font) == 0)
      return;
  /* Add a new field to the _fonts_used_ array. */
  begin_field(&resources->fonts_used);
  dbuffer_printf(&resources->fonts_used.string, "%s", font);
  dbuffer_putc(&resources->fonts_used.string, '\0');
}

/*
 * Remember that _fname_ image was used in the PD Fso it can be embedded later.
 */
int
include_image_resource(struct pdf_resources *resources, const char *fname)
{
  int i;
  /*
   * Loop through all images already in resources. If we find the image then
   * return.
   */
  for (i = 0; i < resources->images.field_count; i++)
    if (strcmp(resources->images.fields[i], fname) == 0)
      return i;
  /* If we did not return, the new image is added to resources. */
  begin_field(&resources->images);
  dbuffer_printf(&resources->images.string, "%s", fname);
  dbuffer_putc(&resources->images.string, '\0');
  /* Return the index of the image resource. */
  return i;
}

/*
 * Add a font resource to the PDF file and return the indirect object reference
 * to it.
 */
static int
pdf_add_font(FILE *pdf_file, FILE *font_file, struct pdf_xref_table *xref,
    const char *name)
{
  struct font_info font_info;
  int font_program, font_widths, font_descriptor, font;

  /* Go to the start of the font file, and parse _font_info_ from it. */
  fseek(font_file, 0, SEEK_SET);
  if (read_ttf(font_file, &font_info))
    return -1; /* Return -1 on error */
  /* Return to start of font file. */
  fseek(font_file, 0, SEEK_SET);

  /* Allocate pdf indirect object references we are gona use. */
  font_program = allocate_pdf_obj(xref);
  font_widths = allocate_pdf_obj(xref);
  font_descriptor = allocate_pdf_obj(xref);
  font = allocate_pdf_obj(xref);

  /* Write the font file bytes into a stream in an indirect object. */
  pdf_start_indirect_obj(pdf_file, xref, font_program);
  pdf_write_file_stream(pdf_file, font_file);
  pdf_end_indirect_obj(pdf_file);

  /* Write the character width array into a indirect object. */
  pdf_start_indirect_obj(pdf_file, xref, font_widths);
  pdf_write_int_array(pdf_file, font_info.char_widths, 256);
  pdf_end_indirect_obj(pdf_file);

  /* Write the font descriptor into an indirect object. */
  pdf_start_indirect_obj(pdf_file, xref, font_descriptor);
  pdf_write_font_descriptor(pdf_file, font_program, name, -10, 255,
      255, 255, 10, font_info.x_min, font_info.y_min, font_info.x_max,
      font_info.y_max);
  pdf_end_indirect_obj(pdf_file);

  /* Write the font dictionary into an indirect object. */
  pdf_start_indirect_obj(pdf_file, xref, font);
  pdf_write_font(pdf_file, name, font_descriptor, font_widths);
  pdf_end_indirect_obj(pdf_file);

  /* Return the indirect object reference to the font. */
  return font;
}

/* Add an image resource to a PDF file. */
static int
pdf_add_image(FILE *pdf_file, FILE *image_file, struct pdf_xref_table *xref)
{
  int image;
  long image_length, i;
  struct jpeg_info jpeg_info;
  const char *color_space;
  /* Parse the jpeg file to get _jpeg_info_. */
  if (read_jpeg(image_file, &jpeg_info))
    return -1; /* Return -1 on error */
  /*
   * Set color space to RGB or Grey depending on number of color components in
   * the jpeg file.
   */
  color_space = jpeg_info.components == 3 ? "DeviceRGB" : "DeviceGray";
  /* Get the length of the _image_file_. */
  fseek(image_file, 0, SEEK_END);
  image_length = ftell(image_file);
  /* Return to the begining of the file. */
  fseek(image_file, 0, SEEK_SET);
  /* Allocate a pdf indirect object reference for the image dictionary. */
  image = allocate_pdf_obj(xref);
  /* Write the PDF image dictionary into a indirect object. */
  pdf_start_indirect_obj(pdf_file, xref, image);
  fprintf(pdf_file, "<<\n\
  /Type /XObject\n\
  /Subtype /Image\n\
  /Width %d\n\
  /Height %d\n\
  /ColorSpace /%s\n\
  /BitsPerComponent 8\n\
  /Length %ld\n\
  /Filter /DCTDecode\n\
  >>\n", jpeg_info.width, jpeg_info.height, color_space, image_length);
  /* Write the embedded image stream at the end of the image object. */
  fprintf(pdf_file, "stream\n");
  for (i = 0; i < image_length; i++)
    fputc(fgetc(image_file), pdf_file);
  fprintf(pdf_file, "\nendstream\n");
  pdf_end_indirect_obj(pdf_file);
  /* Return the indirect object reference to the image. */
  return image;
}

/* Add all resources to the PDF file. */
void
pdf_add_resources(FILE *pdf_file, FILE *typeface_file, int resources_obj,
    const struct pdf_resources *resources, struct pdf_xref_table *xref)
{
  FILE *font_file, *image_file;
  struct record typeface_record;
  int i, parse_result;
  int *font_objs, *image_objs;
  init_record(&typeface_record);
  /* Initialise arrays for font and image indirect objects. */
  font_objs = xmalloc(resources->fonts_used.field_count * sizeof(int));
  image_objs = xmalloc(resources->images.field_count * sizeof(int));
  /*
   * Initaliaise the font indirect objects to -1 so if its not found in the
   * typeface file, we will know.
   */
  for (i = 0; i < resources->fonts_used.field_count; i++)
    font_objs[i] = -1;
  /* Loop through every font in the TYPEFACE. */
  while ( (parse_result = parse_record(typeface_file, &typeface_record))
      != EOF ) {
    if (parse_result) /* If failed to parse record then skip it. */
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
    /* Check typeface does not contain any illegal characters. */
    if (!is_font_name_valid(typeface_record.fields[0])) {
      fprintf(stderr, "Typeface file contains invalid font name '%s'.\n",
          typeface_record.fields[0]);
      continue;
    }
    /*
     * If this font name is in _resources->fonts_used_ then it is used
     * somewhere in the PDF so write the font resource to the PDF.
     */
    i = find_field(&resources->fonts_used, typeface_record.fields[0]);
    if (i == -1) /* Font not used in the PDF. */
      continue;
    /* Try to open this font file. */
    font_file = fopen(typeface_record.fields[1], "r");
    if (font_file == NULL) {
      fprintf(stderr, "Failed to open ttf file '%s': %s\n",
          typeface_record.fields[1], strerror(errno));
      continue;
    }
    /* Add the font resource. */
    font_objs[i] = pdf_add_font(pdf_file, font_file, xref,
      typeface_record.fields[0]);
    if (font_objs[i] == -1) {
      fprintf(stderr, "Failed to parse ttf file '%s'\n",
          typeface_record.fields[1]);
    }
    fclose(font_file);
  }
  free_record(&typeface_record);

  /* Add all image resources. */
  for (i = 0; i < resources->images.field_count; i++) {
    image_file = fopen(resources->images.fields[i], "r");
    if (image_file == NULL) {
      fprintf(stderr, "Failed to open image file '%s'\n",
          resources->images.fields[i]);
      continue;
    }
    image_objs[i] = pdf_add_image(pdf_file, image_file, xref);
    fclose(image_file);
  }

  /* Add the PDF resources dictionary into an indirect object. */
  pdf_start_indirect_obj(pdf_file, xref, resources_obj);
  fprintf(pdf_file, "<<\n  /Font <<\n");
  for (i = 0; i < resources->fonts_used.field_count; i++) {
    /*
     * If _font_objs[i] is still -1 then it was not found in the typeface file.
     */
    if (font_objs[i] == -1) {
      fprintf(stderr, "Typeface file does not include '%s' font.\n",
          resources->fonts_used.fields[i]);
    }
    /* Add the font indirect reference to the fonts dictionary. */
    fprintf(pdf_file, "    /%s %d 0 R\n", resources->fonts_used.fields[i],
        font_objs[i]);
  }
  fprintf(pdf_file, "  >>\n  /XObject <<\n");
  /* Add image indirect references to resources dictionary. */
  for (i = 0; i < resources->images.field_count; i++)
    fprintf(pdf_file, "    /Img%d %d 0 R\n", i, image_objs[i]);
  fprintf(pdf_file, "  >>\n>>\n");
  pdf_end_indirect_obj(pdf_file);
  /* Free local arrays. */
  free(font_objs);
  free(image_objs);
}

/* Free lists allocated on the heap by _resources_. */
void
free_pdf_resources(struct pdf_resources *resources)
{
  free_record(&resources->fonts_used);
  free_record(&resources->images);
}

/* Initialise the list of indirect page object reference. */
void
init_pdf_page_list(struct pdf_page_list *page_list)
{
  page_list->page_count = 0;
  page_list->pages_allocated = 100;
  page_list->page_objs = xmalloc(page_list->pages_allocated * sizeof(int));
}

/* Add this page reference to the list. */
void
add_pdf_page(struct pdf_page_list *page_list, int page)
{
  /* If more memory needs to be allocated for the list. */
  if (page_list->page_count == page_list->pages_allocated) {
    /* Allocate 100 more elements. */
    page_list->pages_allocated += 100;
    page_list->page_objs = xrealloc(page_list->page_objs,
        page_list->pages_allocated * sizeof(int));
  }
  /* Set the new page element and increment the page count. */
  page_list->page_objs[page_list->page_count++] = page;
}

/* Free the page list heap allocated memory. */
void
free_pdf_page_list(struct pdf_page_list *page_list)
{
  free(page_list->page_objs);
}
