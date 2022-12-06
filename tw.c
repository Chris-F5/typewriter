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

int read_font_file(const char *fname, struct font_info *info);
int generate_pdf_file(const char *fname, const struct font_info *info);

int
read_font_file(const char *fname, struct font_info *info)
{
  int ret = 0;
  long size;
  FILE *file = NULL;
  char *ttf = NULL;

  (file = fopen(fname, "r")) OR goto file_error;
  fseek(file, 0, SEEK_END) == 0 OR goto file_error;
  (size = ftell(file)) >= 0 OR goto file_error;
  fseek(file, 0, SEEK_SET) == 0 OR goto file_error;
  (ttf = malloc(size)) OR goto allocation_error;
  fread(ttf, 1, size, file) == size OR goto file_error;
  read_ttf(ttf, size, info) == 0 OR goto ttf_error;
cleanup:
  free(ttf);
  if (file != NULL)
    fclose(file);
  return ret;
file_error:
  fprintf(stderr, "file error in '%s': %s\n", fname, strerror(errno));
  ret = 1;
  goto cleanup;
allocation_error:
  perror("failed to allocate memory");
  ret = 1;
  goto cleanup;
ttf_error:
  fprintf(stderr, "failed to parse ttf '%s'\n", fname);
  ret = 1;
  goto cleanup;
}

int
generate_pdf_file(const char *fname, const struct font_info *info)
{
  int ret = 0;
  int resources, content, page, page_list, catalog;
  FILE *file = NULL;
  struct pdf_ctx pdf;

  (file = fopen(fname, "w")) OR goto file_error;
  pdf_init(&pdf, file) == 0 OR goto pdf_error;

  (resources = pdf_allocate_obj(&pdf)) >= 0 OR goto pdf_error;
  (content = pdf_allocate_obj(&pdf)) >= 0 OR goto pdf_error;
  (page = pdf_allocate_obj(&pdf)) >= 0 OR goto pdf_error;
  (page_list = pdf_allocate_obj(&pdf)) >= 0 OR goto pdf_error;
  (catalog = pdf_allocate_obj(&pdf)) >= 0 OR goto pdf_error;

  pdf_add_resources(&pdf, resources) == 0 OR goto pdf_error;
  pdf_add_stream(&pdf, content, content_stream, strlen(content_stream));
  pdf_add_page(&pdf, page, page_list, resources, content);
  pdf_add_page_list(&pdf, page_list, &page, 1);
  pdf_add_catalog(&pdf, catalog, page_list);

  pdf_end(&pdf, catalog) == 0 OR goto pdf_error;

cleanup:
  if (file != NULL)
    fclose(file);
  return ret;
file_error:
  fprintf(stderr, "file error in '%s': %s\n", fname, strerror(errno));
  ret = 1;
  goto cleanup;
pdf_error:
  fprintf(stderr, "error writing pdf '%s'\n", fname);
  ret = 1;
  goto cleanup;
}

int
main()
{
  struct font_info font_info;

  read_font_file("fonts/cmu.serif-roman.ttf", &font_info) == 0 OR return 1;

  printf("font x_min: %d\n", font_info.x_min);
  printf("font y_min: %d\n", font_info.y_min);
  printf("font x_max: %d\n", font_info.x_max);
  printf("font y_max: %d\n", font_info.y_max);

  generate_pdf_file("output.pdf", &font_info) == 0 OR return 1;

  printf("DONE\n");
  return 0;
}
