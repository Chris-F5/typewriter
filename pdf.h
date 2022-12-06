#include <stdio.h>

struct pdf_ctx {
  FILE *file;
  int obj_count;
  int obj_allocated;
  long *obj_offsets;
};

int pdf_init(struct pdf_ctx *pdf, FILE *file);
int pdf_allocate_obj(struct pdf_ctx *pdf);
int pdf_add_resources(struct pdf_ctx *pdf, int obj);
int pdf_add_stream(struct pdf_ctx *pdf, int obj, const char *stream,
    long stream_length);
int pdf_add_page(struct pdf_ctx *pdf, int obj, int parent, int resources,
    int content);
int pdf_add_page_list(struct pdf_ctx *pdf, int obj, int *pages, int page_count);
int pdf_add_catalog(struct pdf_ctx *pdf, int obj, int page_list);
int pdf_end(struct pdf_ctx *pdf, int root_obj);
