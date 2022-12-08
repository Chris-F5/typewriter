#ifndef TW_PDF
#define TW_PDF

#include <stdio.h>

#define PDF_ERROR_FLAG_MEMORY       0b1
#define PDF_ERROR_FLAG_FILE         0b10
#define PDF_ERROR_FLAG_INVALID_OBJ  0b100
#define PDF_ERROR_FLAG_REPEAT_OBJ   0b1000
#define PDF_ERROR_FLAG_RESERVED_OBJ 0b10000

struct pdf_ctx {
  int error_flags;
  FILE *file;
  int obj_count;
  int obj_allocated;
  long *obj_offsets;
};

int pdf_init(struct pdf_ctx *pdf, FILE *file);
int pdf_allocate_obj(struct pdf_ctx *pdf);
int pdf_add_stream(struct pdf_ctx *pdf, int obj, const char *stream,
    long stream_length);
int pdf_add_true_type_program(struct pdf_ctx *pdf, int obj, const char *ttf,
    long ttf_size);
int pdf_add_int_array(struct pdf_ctx *pdf, int obj, const int *valeus,
    int count);
int pdf_add_resources(struct pdf_ctx *pdf, int obj, int font_widths,
    int font_descriptor, const char *font_name);
int pdf_add_font_descriptor(struct pdf_ctx *pdf, int obj, int font_file,
    const char *font_name, int flags, int italic_angle, int ascent, int descent,
    int cap_height, int stem_vertical, int min_x, int min_y, int max_x,
    int max_y);
int pdf_add_page(struct pdf_ctx *pdf, int obj, int parent, int resources,
    int content);
int pdf_add_page_list(struct pdf_ctx *pdf, int obj, const int *pages,
    int page_count);
int pdf_add_catalog(struct pdf_ctx *pdf, int obj, int page_list);
int pdf_end(struct pdf_ctx *pdf, int root_obj);

#endif
