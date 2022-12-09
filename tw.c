#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "content.h"
#include "pdf.h"
#include "ttf.h"
#include "utils.h"

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
  FILE *pdf_file, *content_file;
  struct pdf_ctx pdf;
  struct content_section content_section;
  struct content_line line1, line2, line3;

  pdf_file = content_file = NULL;
  ( pdf_file = fopen(fname, "w") ) OR goto file_open_error;
  ( content_file = tmpfile() ) OR goto file_open_error;

  content_section_init(&content_section, 595);
  line1.font_size = 16;
  line1.word_spacing = 0;
  line1.text = "First Line () \\ \\n ((()))\\))\n...";
  line2.font_size = 12;
  line2.word_spacing = 5;
  line2.text = "Second Line";
  line3.font_size = 8;
  line3.word_spacing = 10;
  line3.text = "Thrid Line";
  content_section_add_line(&content_section, line1);
  content_section_add_line(&content_section, line2);
  content_section_add_line(&content_section, line3);
  content_section_draw(&content_section, content_file, 0, 842);
  content_section_destroy(&content_section);
  if (content_section.error_flags)
    goto content_error;

  pdf_init(&pdf, pdf_file);

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
  pdf_add_stream(&pdf, content, content_file);
  pdf_add_page(&pdf, page, page_list, resources, content);
  pdf_add_page_list(&pdf, page_list, &page, 1);
  pdf_add_catalog(&pdf, catalog, page_list);

  pdf_end(&pdf, catalog);
  if (pdf.error_flags)
    goto pdf_error;

cleanup:
  if (pdf_file) fclose(pdf_file);
  if (content_file) fclose(content_file);
  return ret;
tmp_file_error:
  perror("failed to open tempoary file");
  ret = 1;
  goto cleanup;
file_open_error:
  fprintf(stderr, "failed to open file '%s': %s\n", fname, strerror(errno));
  ret = 1;
  goto cleanup;
content_error:
  if (content_section.error_flags & CONTENT_ERROR_FLAG_MEMORY)
    fprintf(stderr, "failed to allocate memory\n");
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
