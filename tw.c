#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "tw.h"

static char *file_to_bytes(const char *fname, long *size);
static int read_font_file(const char *fname, struct font_info *info);
static int generate_pdf_file(const char *fname, const char *ttf, long ttf_size,
    const struct font_info *ttf_info, const struct content *content);

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
    const struct font_info *ttf_info, const struct content *content)
{
  int ret = 0;
  int obj_font_descriptor, obj_font_widths, obj_font_file, obj_resources,
      obj_content, obj_page, obj_page_list, obj_catalog;
  FILE *pdf_file;
  struct pdf_ctx pdf;

  pdf_file = NULL;
  ( pdf_file = fopen(fname, "w") ) OR goto file_open_error;

  pdf_init(&pdf, pdf_file);

  obj_font_descriptor = pdf_allocate_obj(&pdf);
  obj_font_widths = pdf_allocate_obj(&pdf);
  obj_font_file = pdf_allocate_obj(&pdf);
  obj_resources = pdf_allocate_obj(&pdf);
  obj_content = pdf_allocate_obj(&pdf);
  obj_page = pdf_allocate_obj(&pdf);
  obj_page_list = pdf_allocate_obj(&pdf);
  obj_catalog = pdf_allocate_obj(&pdf);

  /* TODO: read this info from ttf. */
  pdf_add_font_descriptor(&pdf, obj_font_descriptor, obj_font_file, "MyFont", 6,
      -10, 255, 255, 255, 10, ttf_info->x_min, ttf_info->y_min,
      ttf_info->x_max, ttf_info->y_max);
  pdf_add_int_array(&pdf, obj_font_widths, ttf_info->char_widths, 256);
  pdf_add_true_type_program(&pdf, obj_font_file, ttf, ttf_size);
  pdf_add_resources(&pdf, obj_resources, obj_font_widths, obj_font_descriptor,
      "MyFont");
  pdf_add_content(&pdf, obj_content, content);
  pdf_add_page(&pdf, obj_page, obj_page_list, obj_resources, obj_content);
  pdf_add_page_list(&pdf, obj_page_list, &obj_page, 1);
  pdf_add_catalog(&pdf, obj_catalog, obj_page_list);

  pdf_end(&pdf, obj_catalog);

cleanup:
  if (pdf_file) fclose(pdf_file);
  return ret;
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
  struct stack sym_stack, style_stack, layout_stack;
  struct symbol *root_sym;
  struct style_node *root_style;
  struct layout_box *root_layout;
  struct content content;

  ( ttf = file_to_bytes("fonts/cmu.serif-roman.ttf", &ttf_size) ) OR return 1;
  read_ttf(ttf, ttf_size, &font_info) == 0 OR return 1;

  document = file_to_bytes("input.txt", &document_size);
  document[document_size] = '\0';

  stack_init(&sym_stack, 1024, sizeof(struct symbol));
  stack_init(&style_stack, 1024, sizeof(struct style_node));
  stack_init(&layout_stack, 1024, sizeof(struct layout_box));
  content_init(&content);

  root_sym = parse_document(document, &sym_stack);
  if (!root_sym) return 1;
  print_symbol_tree(root_sym, 0);
  root_style = create_style_tree(root_sym, &style_stack);
  if (!root_style) return 1;
  print_style_tree(root_style, 0);
  root_layout = layout_pages(root_style, &layout_stack);
  if (!root_layout) return 1;
  print_layout_tree(root_layout, 0);
  paint_content(root_layout, &content);
  generate_pdf_file("output.pdf", ttf, ttf_size, &font_info, &content);

  stack_free(&sym_stack);
  stack_free(&style_stack);
  content_free(&content);
  free(document);

  printf("DONE\n");
  return 0;

  /*

  printf("font x_min: %d\n", font_info.x_min);
  printf("font y_min: %d\n", font_info.y_min);
  printf("font x_max: %d\n", font_info.x_max);
  printf("font y_max: %d\n", font_info.y_max);

  generate_pdf_file("output.pdf", ttf, ttf_size, &font_info) == 0 OR return 1;
  free(ttf);
  */
}
