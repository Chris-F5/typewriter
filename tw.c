#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "tw.h"

static char *file_to_bytes(const char *fname, long *size);
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
  (bytes = malloc(*size + 1)) OR goto allocation_error;
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

  pdf_file = content_file = NULL;
  ( pdf_file = fopen(fname, "w") ) OR goto file_open_error;
  ( content_file = tmpfile() ) OR goto file_open_error;

  fprintf(content_file, "\
0 0 0 rg\n\
BT /F1 16 Tf 0 Tw 0 826 Td (First Line)Tj ET\n\
BT /F1 12 Tf 5 Tw 0 814 Td (Second Line)Tj ET\n\
BT /F1 8 Tf 10 Tw 0 806 Td (Thrid Line)Tj ET");

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
  pdf_add_font_descriptor(&pdf, font_descriptor, font_file, "MyFont", 6,
      -10, 255, 255, 255, 10, ttf_info->x_min, ttf_info->y_min,
      ttf_info->x_max, ttf_info->y_max);
  pdf_add_int_array(&pdf, font_widths, ttf_info->char_widths, 256);
  pdf_add_true_type_program(&pdf, font_file, ttf, ttf_size);
  pdf_add_resources(&pdf, resources, font_widths, font_descriptor, "MyFont");
  pdf_add_stream(&pdf, content, content_file);
  pdf_add_page(&pdf, page, page_list, resources, content);
  pdf_add_page_list(&pdf, page_list, &page, 1);
  pdf_add_catalog(&pdf, catalog, page_list);

  pdf_end(&pdf, catalog);

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
}

int
main()
{
  char *ttf, *document;
  long ttf_size, document_size;
  struct font_info font_info;
  struct symbol_stack sym_stack;
  struct symbol *root_sym;

  document = file_to_bytes("input.txt", &document_size);
  document[document_size] = '\0';
  root_sym = parse_document(document, &sym_stack);
  if (root_sym)
    print_symbol_tree(root_sym, 0);

  ( ttf = file_to_bytes("fonts/cmu.serif-roman.ttf", &ttf_size) ) OR return 1;
  read_ttf(ttf, ttf_size, &font_info) == 0 OR return 1;

  printf("font x_min: %d\n", font_info.x_min);
  printf("font y_min: %d\n", font_info.y_min);
  printf("font x_max: %d\n", font_info.x_max);
  printf("font y_max: %d\n", font_info.y_max);

  generate_pdf_file("output.pdf", ttf, ttf_size, &font_info) == 0 OR return 1;

  free(document);
  free(ttf);
  printf("DONE\n");
  return 0;
}
