#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "pdf.h"
#include "ttf.h"
#include "utils.h"

static const char *content_stream = "\
0.7 0.7 1 rg\n\
0 811 30 30 re f\n\
80 700 30 30 re f\n\
0 0 0 rg\n\
BT /F1 12 Tf 2 770 Td (Hello World!)Tj ET\n\
BT /F1 16 Tf 2 754 Td (Notice how each character has its own width.)Tj ET";

static char * file_to_bytes(const char *fname, long *size);
static int read_font_file(const char *fname, struct font_info *info);
static int generate_pdf_file(const char *fname, const char *ttf, long ttf_size,
    const struct font_info *ttf_info);

static char *
file_to_bytes(const char *fname, long *size)
{
  FILE *file = NULL;
  char *bytes = NULL;

  (file = fopen(fname, "r")) OR goto file_error;
  fseek(file, 0, SEEK_END) == 0 OR goto file_error;
  (*size = ftell(file)) >= 0 OR goto file_error;
  fseek(file, 0, SEEK_SET) == 0 OR goto file_error;
  (bytes = malloc(*size)) OR goto allocation_error;
  fread(bytes, 1, *size, file) == *size OR goto file_error;
  
cleanup:
  if (file != NULL)
    fclose(file);
  return bytes;
file_error:
  fprintf(stderr, "file error in '%s': %s\n", fname, strerror(errno));
  free(bytes);
  bytes = NULL;
  goto cleanup;
allocation_error:
  perror("failed to allocate memory");
  free(bytes);
  bytes = NULL;
  goto cleanup;
}

static int
read_font_file(const char *fname, struct font_info *info)
{
  int ret = 0;
  long size;
  FILE *file = NULL;
  char *ttf = NULL;

  read_ttf(ttf, size, info) == 0 OR goto ttf_error;
cleanup:
  free(ttf);
  if (file != NULL)
    fclose(file);
  return ret;
ttf_error:
  fprintf(stderr, "failed to parse ttf '%s'\n", fname);
  ret = 1;
  goto cleanup;
}

static int
generate_pdf_file(const char *fname, const char *ttf, long ttf_size,
    const struct font_info *ttf_info)
{
  int ret = 0;
  int font_descriptor, font_widths, font_file, resources, content, page;
  int page_list, catalog;
  FILE *file = NULL;
  struct pdf_ctx pdf;

  ( file = fopen(fname, "w") ) OR goto file_open_error;
  pdf_init(&pdf, file);

  font_descriptor = pdf_allocate_obj(&pdf);
  font_widths = pdf_allocate_obj(&pdf);
  font_file = pdf_allocate_obj(&pdf);
  resources = pdf_allocate_obj(&pdf);
  content = pdf_allocate_obj(&pdf);
  page = pdf_allocate_obj(&pdf);
  page_list = pdf_allocate_obj(&pdf);
  catalog = pdf_allocate_obj(&pdf);

  /* TODO: read this info from ttf. */
  pdf_add_font_descriptor(&pdf, font_descriptor, font_file, "MyFontName", 6,
      -10, 255, 255, 255, 10, ttf_info->x_min, ttf_info->y_min,
      ttf_info->x_max, ttf_info->y_max);
  pdf_add_int_array(&pdf, font_widths, ttf_info->char_widths, 256);
  pdf_add_true_type_program(&pdf, font_file, ttf, ttf_size);
  pdf_add_resources(&pdf, resources, font_widths, font_descriptor,
      "MyFontName");
  pdf_add_stream(&pdf, content, content_stream, strlen(content_stream));
  pdf_add_page(&pdf, page, page_list, resources, content);
  pdf_add_page_list(&pdf, page_list, &page, 1);
  pdf_add_catalog(&pdf, catalog, page_list);

  pdf_end(&pdf, catalog) == 0 OR goto pdf_error;

cleanup:
  if (file != NULL)
    fclose(file);
  return ret;
file_open_error:
  fprintf(stderr, "failed to open file '%s': %s\n", fname, strerror(errno));
  ret = 1;
  goto cleanup;
pdf_error:
  if (pdf.error_flags & PDF_ERROR_FLAG_MEMORY)
    fprintf(stderr, "failed to allocate memory\n");
  if (pdf.error_flags & PDF_ERROR_FLAG_FILE)
    fprintf(stderr, "failed to write to '%s'\n", fname);
  if (pdf.error_flags & PDF_ERROR_FLAG_INVALID_OBJ)
    fprintf(stderr, "invalid pdf object number\n");
  if (pdf.error_flags & PDF_ERROR_FLAG_REPEAT_OBJ)
    fprintf(stderr, "repeated pdf object number\n");
  if (pdf.error_flags & PDF_ERROR_FLAG_RESERVED_OBJ)
    fprintf(stderr, "reserved pdf object number\n");
  ret = 1;
  goto cleanup;
}

int
main()
{
  char *ttf;
  long ttf_size;
  struct font_info font_info;

  ( ttf = file_to_bytes("fonts/cmu.serif-roman.ttf", &ttf_size) ) OR return 1;
  read_ttf(ttf, ttf_size, &font_info) == 0 OR return 1;

  printf("font x_min: %d\n", font_info.x_min);
  printf("font y_min: %d\n", font_info.y_min);
  printf("font x_max: %d\n", font_info.x_max);
  printf("font y_max: %d\n", font_info.y_max);

  ttf OR free(ttf);
  ( ttf = file_to_bytes("fonts/cmu.serif-roman.ttf", &ttf_size) ) OR return 1;
  generate_pdf_file("output.pdf", ttf, ttf_size, &font_info) == 0 OR return 1;

  printf("DONE\n");
  ttf OR free(ttf);
  return 0;
}
